
#include <vector>

#include "InterviewManager.h"

#include "Nodes.h"
#include "Node.h"
#include "CommandClass.h"

void ZW_InterviewManager::RequestNodeProtocolInfo(node_t nodeid)
{
	if (nodeid == node_t{ 0 } || nodeid == node_t{ 1 }) return;

	activeNode = nodes.GetOrCreate(nodeid, enqueue);
	activeNode->SetInterviewState(ZW_Node::eInterviewState::ProtocolInfoPending);

	ZW_APIFrame frame;
	frame.Make(eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA, nodeid);

	Log.AddL(eLogTypes::DVC, MakeTag(), ">> {}: node={}", ToString(eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA), nodeid);
	enqueue(frame);
}

void ZW_InterviewManager::DecodeNodeProtocolInfo(node_t nodeid, const APIFrame::PayLoad& payload)
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

	ZW_Node* node = nodes.GetOrCreate(nodeid, enqueue);
	listening ? node->WakeUp(): node->Sleeping();
	
	ZW_NodeInfo::ProtocolInfo protocolInfo;
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

	node->SetInterviewState(ZW_Node::eInterviewState::ProtocolInfoDone); 
}

void ZW_InterviewManager::RequestNodeInformation(node_t nodeid)
{
	if (nodeid == node_t{ 0 } || nodeid == node_t{ 1 }) return;

	ZW_Node* node = nodes.GetOrCreate(nodeid, enqueue);
	node->SetInterviewState(ZW_Node::eInterviewState::NodeInfoPending);

	ZW_APIFrame frame;
	frame.Make(eCommandIds::ZW_API_REQUEST_NODE_INFORMATION, nodeid);

	Log.AddL(eLogTypes::DVC, MakeTag(), ">> {}: node={}", ToString(eCommandIds::ZW_API_REQUEST_NODE_INFORMATION), nodeid);
	enqueue(frame);
}

// Decodes Node Information Frame (NIF) from payload and updates the node if possible
void ZW_InterviewManager::DecodeNodeInfo(const APIFrame::PayLoad& payload)
{
	if (payload.size() < 3)
	{
		Log.AddL(eLogTypes::ITW, MakeTag(), "DecodeNodeInfo: payload too short (len={})", payload.size());
		return;
	}

	node_t nodeid{ payload[1] };
	ZW_Node* node = activeNode = nodes.GetOrCreate(nodeid, enqueue);
	if (!node)
	{
		Log.AddL(eLogTypes::ITW, MakeTag(), "DecodeNodeInfo: node {} not found", nodeid);
		return;
	}
	if (node->GetInterviewState() == ZW_Node::eInterviewState::InterviewDone)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "----------------------------------- DecodeNodeInfo: node {} already interviewed", nodeid);
		node->WakeUp();
		return;
	}
	node->SetInterviewState(ZW_Node::eInterviewState::NodeInfoPending);

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

	node->SetInterviewState(ZW_Node::eInterviewState::NodeInfoDone);  

	node->WakeUp();
	return;
}
