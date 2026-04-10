
#include "Logging.h"
#include "NodeInterview.h"
#include "Node.h"

/* ========================================================================
   Z-Wave Node Interview — Overview
   ========================================================================

   Start(node)
	   If state == NotInterviewed:
		   RequestNodeProtocolInfo
			   -> HandleFrame(ZW_API_GET_NODE_INFO_PROTOCOL_DATA)
			   -> DecodeNodeProtocolInfo

			   If node is listening:
				   RequestNodeInformation
					   -> HandleApplicationUpdate(
							  UPDATE_STATE_NODE_INFO_RECEIVED
							  or UPDATE_STATE_NODE_ADDED
						  )
					   -> DecodeNodeInfo
					   -> NodeInterview reaches NodeInfoDone
					   -> Node-specific interview/jobs continue asynchronously in Node

			   If node is NOT listening:
				   NodeInterview reaches ProtocolInfoDone
				   (interview pauses until the node wakes up)

   Later asynchronous path for non-listening/sleepy nodes:
	   HandleApplicationUpdate(
		   UPDATE_STATE_NODE_INFO_RECEIVED
		   or UPDATE_STATE_NODE_ADDED
	   )
		   -> DecodeNodeInfo
		   -> Node-specific interview/jobs continue asynchronously in Node
		   -> Node transitions itself to InterviewDone
			  and performs post-interview jobs there

   Done(node) returns true when:
	   Listening node     -> state == NodeInfoDone
	   Non-listening node -> state == ProtocolInfoDone
	   (Remaining interview work happens asynchronously inside Node)

   ======================================================================== */

   /* ========================================================================
	  Z-Wave Node Interview State Machine
	  ========================================================================

						+----------------------+
						|   NotInterviewed     |
						+----------+-----------+
								   |
								   | Start(node)
								   v
						+----------------------+
						| RequestNodeProtocol  |
						| (ZW_API_GET_...)     |
						+----------+-----------+
								   |
								   | HandleFrame(ZW_API_GET_NODE_INFO_PROTOCOL_DATA)
								   v
						+----------------------+
						| DecodeProtocolInfo   |
						+----------+-----------+
								   |
				   +---------------+----------------+
				   |                                |
				   | node is listening              | node is NOT listening
				   v                                v
	  +-----------------------------+      +-----------------------------+
	  | RequestNodeInformation      |      |  ProtocolInfoDone           |
	  | (ZW_API_REQUEST_NODE_INFO)  |      |  Done(node)==true           |
	  +--------------+--------------+      +--------------+--------------+
					 |                                |
					 |                                | Later (async)
					 |                                | ApplicationUpdate:
					 |                                |   UPDATE_STATE_NODE_INFO_RECEIVED
					 |                                |   UPDATE_STATE_NODE_ADDED
					 v                                v
	  +-----------------------------+      +-----------------------------+
	  | ApplicationUpdate           |      | ApplicationUpdate           |
	  | UPDATE_STATE_NODE_INFO_...  |      | UPDATE_STATE_NODE_INFO_...  |
	  +--------------+--------------+      +--------------+--------------+
					 |                                |
					 v                                v
			 +------------------+             +------------------+
			 |  DecodeNodeInfo  |             |  DecodeNodeInfo  |
			 +--------+---------+             +--------+---------+
					  |                                |
					  v                                v
			 +------------------+             +------------------+
			 |  NodeInfoDone    |             | Continue async   |
			 | Done(node)==true |             | in Node          |
			 +--------+---------+             +--------+---------+
					  |                                |
					  |                                | sleepy/non-listening node
					  |                                | sets InterviewDone and runs
					  |                                | post-interview jobs there
					  v                                v
			 +------------------+             +------------------+
			 | Continue async   |             |   InterviewDone  |
			 | in Node          |             +--------+---------+
			 +--------+---------+                      |
					  |                                v
					  |                       +------------------+
					  |                       |   Operational    |
					  |                       +------------------+
					  v
			 +------------------+
			 |   InterviewDone  |
			 +--------+---------+
					  |
					  v
			 +------------------+
			 |   Operational    |
			 +------------------+

	  ======================================================================== */

NodeInterview::NodeInterview(Nodes& nodes, EnqueueFn enqueue)
	: nodes(nodes)
	, enqueue(enqueue)
	, activeNode(nullptr)
{
}

// call Start and wait until Done reports true, add an timeout in calling task
void NodeInterview::Start(nodeid_t nodeid)
{
	auto node = nodes.GetOrCreate(nodeid);
	if (!node) return;

	if (node->GetInterviewState() == Node::eInterviewState::NotInterviewed)
	{
		RequestNodeProtocolInfo(nodeid); // starts the interview chain
	}
}

bool NodeInterview::Done(nodeid_t nodeid)
{
	const auto node = nodes.Get(nodeid);
	if (node)
	{
		if (!node->IsListening() && node->GetInterviewState() == Node::eInterviewState::ProtocolInfoDone)
			return true; // rest of interview is done when a UPDATE_STATE_NODE_INFO_RECEIVED or UPDATE_STATE_NODE_ADDED is received and async in Node

		if (node->IsListening() && node->GetInterviewState() >= Node::eInterviewState::NodeInfoDone)
			return true; // rest of interview is done async in Node
	}
	return false;
}

const bool NodeInterview::HandleFrame(const APIFrame& frame)
{
	Log.AddL(eLogTypes::DBG, MakeTag(), "HandleFrame called for node {}: {}", activeNode ? activeNode->nodeId : nullptr, frame.Info());

	switch (frame.APICmd.CmdId)
	{
	case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
		// ignore
		return true;

	case eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA:
		if (activeNode == nullptr)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "<< NODE_PROTOCOL_INFO: unexpected response (no current node)");
			return true;
		}
		DecodeNodeProtocolInfo(activeNode->nodeId, frame.payload);
		if (activeNode->GetInterviewState() == Node::eInterviewState::ProtocolInfoDone && activeNode->IsListening())
			RequestNodeInformation(activeNode->nodeId);
		return true;

	case eCommandIds::ZW_API_APPLICATION_UPDATE:
		HandleApplicationUpdate(frame);
		return true;

	default:
		return false;
	}
}

bool NodeInterview::HandleFrameTimeout(const APIFrame& frame)
{
	Log.AddL(eLogTypes::ITW, MakeTag(), "HandleFrameTimeout called for node {}: {}",
			 activeNode ? activeNode->nodeId : nullptr, frame.Info());
	return false;
}

void NodeInterview::HandleApplicationUpdate(const APIFrame& frame)
{
	const auto& p = frame.payload;
	if (p.size() < 2)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< payload too short: {}", frame.Info());
		return;
	}

	ApplicationUpdateEvent event = static_cast<ApplicationUpdateEvent>(p[0]);

	switch (event)
	{
	case ApplicationUpdateEvent::UPDATE_STATE_NODE_ADDED:
	case ApplicationUpdateEvent::UPDATE_STATE_NODE_INFO_RECEIVED:
		DecodeNodeInfo(p);
		break;

	case ApplicationUpdateEvent::UPDATE_STATE_NODE_INFO_REQ_FAILED:
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< {}: event={} payloadLen={}", ToString(eCommandIds::ZW_API_APPLICATION_UPDATE), ToString(event), p.size());
		if (activeNode != nullptr)
		{
			Node* node = activeNode;
			if (node && node->GetInterviewState() == Node::eInterviewState::NodeInfoPending && node->IsListening())
				node->SetInterviewState(Node::eInterviewState::NodeInfoPending);
		}
		break;

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< {}: unexpected event=0x{:02X} payloadLen={}", ToString(eCommandIds::ZW_API_APPLICATION_UPDATE), static_cast<uint8_t>(event), p.size());
		break;
	}

}

void NodeInterview::RequestNodeProtocolInfo(nodeid_t nodeid)
{
	if (nodeid == 0 || nodeid == 1) return;

	activeNode = nodes.GetOrCreate(nodeid);
	activeNode->SetInterviewState(Node::eInterviewState::ProtocolInfoPending);

	APIFrame frame;
	frame.Make(eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA, nodeid);

	Log.AddL(eLogTypes::DVC, MakeTag(), ">> {}: node={}", ToString(eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA), nodeid);
	enqueue(frame);
}

void NodeInterview::DecodeNodeProtocolInfo(nodeid_t nodeid, const payload_t& payload)
{
	// Table 4.92: Get Node Information Protocol Data response payload (excluding FUNC_ID)
	// [0]=Byte1 (Listening, Routing, Supported speed[3], Protocol version[3])
	// [1]=Byte2 (Optional Functionality,
	//				Sensor1000ms,
	//				Sensor250ms,
	//				Beam Capability,
	//				Routing end node,
	//				Specific device,
	//				Controller node,
	//				Security)
	// [2]=Byte3 Reserved Speed Extension
	// [3]=Basic Device Type
	// [4]=Generic Device Class
	// [5]=Specific Device Class

#pragma pack(push, 1)
	struct PayloadStruct
	{
		uint8_t b0, b1, b2;
		uint8_t basic;
		uint8_t generic;
		uint8_t specific;
	};
#pragma pack(pop)
	static_assert(sizeof(PayloadStruct) == 6);

	if (payload.size() < sizeof(PayloadStruct))
	{
		Log.AddL(eLogTypes::ITW, MakeTag(), "DecodeNodeProtocolInfo: node={} payload too short (len={})", nodeid, payload.size());
		return;
	}

	const auto* p = reinterpret_cast<const PayloadStruct*>(payload.data());

	NodeInfo::ProtocolInfo info;

	info.b0 = p->b0;
	info.b1 = p->b1;
	info.b2 = p->b2;
	info.basicType = static_cast<NodeInfo::ProtocolInfo::eBasicDeviceType>(p->basic);
	info.genericDeviceClass = static_cast<NodeInfo::ProtocolInfo::eGenericDeviceClass>(p->generic); 
	info.specificDeviceClass = p->specific;

	Node* node = nodes.Get(nodeid);
	node->SetProtocolInfo(info);

	info.IsListening() ? node->WakeUp() : node->Sleeping();

	Log.AddL(
		eLogTypes::DVC,
		MakeTag(),
		"<< {}: node={} listening={} routing={} speed={} proto={} basic=0x{:02X} generic=0x{:02X} specific=0x{:02X} (payloadLen={})",
		ToString(eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA),
		nodeid,
		info.IsListening() ? "yes" : "no",
		info.IsRouting() ? "yes" : "no",
		(int)info.SupportedSpeed(),
		(int)info.ProtocolVersion(),
		(int)info.BasicType(),
		(int)info.GenericDeviceClass(),
		info.specificDeviceClass,
		payload.size()
	);

	node->SetInterviewState(Node::eInterviewState::ProtocolInfoDone);
}

void NodeInterview::RequestNodeInformation(nodeid_t nodeid)
{
	if (nodeid == 0 || nodeid == 1) return;

	Node* node = nodes.GetOrCreate(nodeid);
	node->SetInterviewState(Node::eInterviewState::NodeInfoPending);

	APIFrame frame;
	frame.Make(eCommandIds::ZW_API_REQUEST_NODE_INFORMATION, nodeid);

	Log.AddL(eLogTypes::DVC, MakeTag(), ">> {}: node={}", ToString(eCommandIds::ZW_API_REQUEST_NODE_INFORMATION), nodeid);
	enqueue(frame);
}

// Decodes Node Information Frame (NIF) from payload and updates the node if possible
void NodeInterview::DecodeNodeInfo(const payload_t& payload)
{
	if (payload.size() < 3)
	{
		Log.AddL(eLogTypes::ITW, MakeTag(), "DecodeNodeInfo: payload too short (len={})", payload.size());
		return;
	}

	nodeid_t nodeid{ payload[1] };
	Node* node = activeNode = nodes.GetOrCreate(nodeid);
	if (!node)
	{
		Log.AddL(eLogTypes::ITW, MakeTag(), "DecodeNodeInfo: node {} not found", nodeid);
		return;
	}

	if (node->GetInterviewState() == Node::eInterviewState::InterviewDone)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "----------------------------------- DecodeNodeInfo: node {} already interviewed", nodeid);
		node->WakeUp();
		return;
	}

	node->SetInterviewState(Node::eInterviewState::NodeInfoPending);

	size_t index = 2;
	if (payload.size() < index + 1)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "DecodeNodeInfo: truncated payload for node {}", nodeid);
		return;
	}

	const uint8_t ccLen = payload[index++];
	if (ccLen < 3 || payload.size() < index + ccLen)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "DecodeNodeInfo: invalid ccLen={} or truncated payload for node {}", ccLen, nodeid);
		return;
	}

	const NodeInfo::ProtocolInfo::eBasicDeviceType basic = static_cast<NodeInfo::ProtocolInfo::eBasicDeviceType>(payload[index++]); 
	const NodeInfo::ProtocolInfo::eGenericDeviceClass generic = static_cast<NodeInfo::ProtocolInfo::eGenericDeviceClass>(payload[index++]);
	const uint8_t specific = payload[index++];

	std::vector<uint8_t> ccList;
	if (ccLen > 3)
	{
		ccList.assign(payload.begin() + static_cast<std::ptrdiff_t>(index), payload.begin() + static_cast<std::ptrdiff_t>(index + (ccLen - 3)));
	}

	node->SetNIF(basic, generic, specific, ccList);

	Log.AddL(eLogTypes::DVC, MakeTag(), "<< Decoded NodeInfo: node={} basic=0x{:02X} generic=0x{:02X} specific=0x{:02X} ccCount={}", nodeid, basic, generic, specific, ccList.size());

	node->SetInterviewState(Node::eInterviewState::NodeInfoDone);

	node->WakeUp();
	return;
}
