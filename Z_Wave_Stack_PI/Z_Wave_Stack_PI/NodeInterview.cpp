
#include "Logging.h"
#include "NodeInterview.h"

NodeInterview::NodeInterview(Nodes& nodes, EnqueueFn enqueue)
	: nodes(nodes)
	, enqueue(enqueue)
	, activeNode(nullptr)
{
}

void NodeInterview::Start(nodeid_t nodeid)
{
	auto node = nodes.GetOrCreate(nodeid);
	if (!node) return;

	switch (node->GetInterviewState())
	{
	case Node::eInterviewState::NotInterviewed:
		RequestNodeProtocolInfo(nodeid);
		break;

		//case Node::eInterviewState::ProtocolInfoDone:	// handled in HandleFrame
		//case Node::eInterviewState::NodeInfoDone:		// handled in InteviewManager
		//case Node::eInterviewState::CCVersionDone:		// handled in Device
		//case Node::eInterviewState::CCMnfcSpecDone:	// handled in Device
		// CCMultiChannelPending, CCMultiChannelDone // handled in 

	case Node::eInterviewState::CCMultiChannelDone:
		{
			node->SetInterviewState(Node::eInterviewState::InterviewDone);
			Node::Job job;
			job.job = Node::eJobs::ASSOCIATION_INTERVIEW;
			node->EnqueueJob(job);
			job.job = Node::eJobs::MULTI_CHANNEL_ASSOCIATION_INTERVIEW;
			node->EnqueueJob(job);
			job.job = Node::eJobs::CONFIGURATION_INTERVIEW;
			job.group = 0; // start with param 0
			job.value = 10; // get 10 params
			node->EnqueueJob(job);
		}
		break;

	default:
		break;
	}
}

bool NodeInterview::Done(nodeid_t nodeid)
{
	const auto node = nodes.Get(nodeid);
	if (node)
	{
		if (!node->IsListening() && node->GetInterviewState() == Node::eInterviewState::ProtocolInfoDone)
			return true;

		if (node->IsListening() && node->GetInterviewState() == Node::eInterviewState::InterviewDone)
			return true;
	}
	return false;
}

const bool NodeInterview::HandleFrame(const APIFrame& frame)
{
	Log.AddL(eLogTypes::DBG, MakeTag(), "HandleFrame called for node {}: {}", activeNode ? activeNode->nodeId : (nodeid_t)0, frame.Info());

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
			 activeNode ? activeNode->nodeId : (nodeid_t)0, frame.Info());
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
		Log.AddL(eLogTypes::DBG, MakeTag(), "<< {}: unexpected event=0x{:02X} payloadLen={}", ToString(eCommandIds::ZW_API_APPLICATION_UPDATE), static_cast<uint8_t>(event), p.size());
		break;
	}

}

void NodeInterview::RequestNodeProtocolInfo(nodeid_t nodeid)
{
	if (nodeid == nodeid_t{ 0 } || nodeid == nodeid_t{ 1 }) return;

	activeNode = nodes.GetOrCreate(nodeid);
	activeNode->SetInterviewState(Node::eInterviewState::ProtocolInfoPending);

	APIFrame frame;
	frame.Make(eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA, nodeid);

	Log.AddL(eLogTypes::DVC, MakeTag(), ">> {}: node={}", ToString(eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA), nodeid);
	enqueue(frame);
}

void NodeInterview::DecodeNodeProtocolInfo(nodeid_t nodeid, const APIFrame::PayLoad& payload)
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
	if (payload.size() < 6)
	{
		Log.AddL(eLogTypes::ITW, MakeTag(), "DecodeNodeProtocolInfo: node={} payload too short (len={})", nodeid, payload.size());
		return;
	}

	const uint8_t b1 = payload[0];
	const uint8_t b2 = payload[1];
	const uint8_t b3 = payload[2];
	const uint8_t basic = payload[3];
	const uint8_t generic = payload[4];
	const uint8_t specific = payload[5];

	const bool listening = (b1 & 0x80) != 0;
	const bool routing = (b1 & 0x40) != 0;
	const uint8_t supportedSpeed = static_cast<uint8_t>((b1 >> 3) & 0x07);
	const uint8_t protocolVersion = static_cast<uint8_t>(b1 & 0x07);

	const bool optionalFunctionality = (b2 & 0x80) != 0;
	const bool sensor1000ms = (b2 & 0x40) != 0;
	const bool sensor250ms = (b2 & 0x20) != 0;
	const bool beamCapability = (b2 & 0x10) != 0;
	const bool routingEndNode = (b2 & 0x08) != 0;
	const bool specificDevice = (b2 & 0x04) != 0;
	const bool controllerNode = (b2 & 0x02) != 0;
	const bool security = (b2 & 0x01) != 0;

	Log.AddL(
		eLogTypes::DVC,
		MakeTag(),
		"<< {}: node={} listening={} routing={} speed={} proto={} basic=0x{:02X} generic=0x{:02X} specific=0x{:02X} opt=0x{:02X} speedExt=0x{:02X} (payloadLen={})",
		ToString(eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA),
		nodeid, listening, routing, supportedSpeed, protocolVersion, basic, generic, specific, b2, b3
		, payload.size());

	Node* node = nodes.GetOrCreate(nodeid);
	listening ? node->WakeUp() : node->Sleeping();

	NodeInfo::ProtocolInfo protocolInfo;
	protocolInfo.basic = basic;
	protocolInfo.generic = generic;
	protocolInfo.specific = specific;

	protocolInfo.isListening = listening;
	protocolInfo.isRouting = routing;
	protocolInfo.supportedSpeed = supportedSpeed;
	protocolInfo.protocolVersion = protocolVersion;

	protocolInfo.optionalFunctionality = optionalFunctionality;
	protocolInfo.sensor1000ms = sensor1000ms;
	protocolInfo.sensor250ms = sensor250ms;
	protocolInfo.beamCapable = beamCapability;
	protocolInfo.routingEndNode = routingEndNode;
	protocolInfo.specificDevice = specificDevice;
	protocolInfo.controllerNode = controllerNode;
	protocolInfo.security = security;
	node->SetProtocolInfo(protocolInfo);

	node->SetInterviewState(Node::eInterviewState::ProtocolInfoDone);
}

void NodeInterview::RequestNodeInformation(nodeid_t nodeid)
{
	if (nodeid == nodeid_t{ 0 } || nodeid == nodeid_t{ 1 }) return;

	Node* node = nodes.GetOrCreate(nodeid);
	node->SetInterviewState(Node::eInterviewState::NodeInfoPending);

	APIFrame frame;
	frame.Make(eCommandIds::ZW_API_REQUEST_NODE_INFORMATION, nodeid);

	Log.AddL(eLogTypes::DVC, MakeTag(), ">> {}: node={}", ToString(eCommandIds::ZW_API_REQUEST_NODE_INFORMATION), nodeid);
	enqueue(frame);
}

// Decodes Node Information Frame (NIF) from payload and updates the node if possible
void NodeInterview::DecodeNodeInfo(const APIFrame::PayLoad& payload)
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

	const uint8_t basic = payload[index++];
	const uint8_t generic = payload[index++];
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
