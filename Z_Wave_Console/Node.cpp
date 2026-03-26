#include "Node.h"

std::string ZW_NodeInfo::NodeStateString() const
{
	DebugLockGuard lock(stateMutex);
	switch (nodeState)
	{
	case eNodeState::New:    return "New";
	case eNodeState::Awake:  return "Awake";
	case eNodeState::Sleepy: return "Sleepy";
	}
	return "Unknown";
}

std::string ZW_NodeInfo::InterviewStateString() const
{
	DebugLockGuard lock(stateMutex);
	switch (interviewState)
	{
	case eInterviewState::NotInterviewed: return "NotInterviewed";
	case eInterviewState::ProtocolInfoPending: return "ProtocolInfoPending";
	case eInterviewState::ProtocolInfoDone: return "ProtocolInfoDone";
	case eInterviewState::NodeInfoPending: return "NodeInfoPending";
	case eInterviewState::NodeInfoDone: return "NodeInfoDone";
	case eInterviewState::CCVersionPending: return "CCVersionPending";
	case eInterviewState::CCVersionDone: return "CCVersionDone";
	case eInterviewState::CCMnfcSpecPending: return "CCMnfcSpecPending";
	case eInterviewState::CCMnfcSpecDone: return "CCMnfcSpecDone";
	case eInterviewState::CCMultiChannelPending: return "CCMultiChannelPending";
	case eInterviewState::CCMultiChannelDone: return "CCMultiChannelDone";
	case eInterviewState::InterviewDone: return "InterviewDone";
	}
	return "Unknown";
}

bool ZW_NodeInfo::supportsCC(eCommandClass cc) const
{
	// 1) Check if CC is in the NIF (authoritative)
	//if (std::find(nifCCs.begin(), nifCCs.end(), cc) == nifCCs.end())
		//return false;

	// 2) Special rule: Multi Channel CC (0x60)
	if (cc == eCommandClass::MULTI_CHANNEL)
	{
		// Multi Channel CC is only valid if the node has endpoints
		// or if the device class is one that MUST have endpoints
		bool hasEndpoints =
			//			(multiChannel.endpointCount > 0) ||
			(protocolInfo.generic == 0x11) ||   // Multi Level Switch
			(protocolInfo.generic == 0x12) ||   // Multi Level Sensor
			(protocolInfo.generic == 0x13) ||   // Multi Channel Device
			(protocolInfo.generic == 0xA1);     // Multi Endpoint Controller

		return hasEndpoints;
	}
	/*
		// 3) Security-wrapped CCs (S0/S2)
		if (isSecurelyIncluded())
		{
			if (cc == eCommandClass::SECURITY_0 || cc == eCommandClass::SECURITY_2)
				return true;

			if (std::find(secureCCs.begin(), secureCCs.end(), cc) != secureCCs.end())
				return true;
		}
	*/
	// 4) Otherwise: CC is supported
	return true;
}

std::string ZW_NodeInfo::ToString(int width) const
{
	DebugLockGuard lock(stateMutex);
	std::ostringstream out;

	auto WrapAndPrint = [&](std::ostream& out, const std::string& text)
		{
			std::istringstream words(text);
			std::string word;
			std::string line;

			while (words >> word)
			{
				if (line.size() + word.size() + 1 > width)
				{
					out << line << "\n";
					line = word;
				}
				else
				{
					if (!line.empty()) line += " ";
					line += word;
				}
			}

			if (!line.empty())
				out << line << "\n";
		};

	out << "=== Node " << std::dec << NodeId.value << " ===\n";

	//
	// Node state
	//
	out << "State: ";
	switch (nodeState)
	{
	case eNodeState::Awake:  out << "Awake "; break;
	case eNodeState::Sleepy: out << "Sleepy "; break;
	default:                 out << "Bad "; break;
	}

	//
	// Interview state
	//
	out << " Interview: ";
	switch (interviewState)
	{
	case eInterviewState::NotInterviewed:      out << "NotInterviewed "; break;
	case eInterviewState::ProtocolInfoPending: out << "ProtocolInfoPending "; break;
	case eInterviewState::ProtocolInfoDone:    out << "ProtocolInfoDone "; break;
	case eInterviewState::NodeInfoPending:     out << "NodeInfoPending "; break;
	case eInterviewState::NodeInfoDone:        out << "NodeInfoDone "; break;
	case eInterviewState::CCVersionPending:    out << "CCVersionPending "; break;
	case eInterviewState::CCVersionDone:       out << "CCVersionDone "; break;
	case eInterviewState::CCMnfcSpecPending:   out << "CCMnfcSpecPending "; break;
	case eInterviewState::CCMnfcSpecDone:      out << "CCMnfcSpecDone "; break;
	case eInterviewState::CCMultiChannelPending:    out << "CCMultiChannelPending "; break;
	case eInterviewState::CCMultiChannelDone:       out << "CCMultiChannelDone "; break;

	case eInterviewState::InterviewDone:       out << "InterviewDone "; break;
	default:                                   out << "Unknown "; break;
	}

	//
	// Manufacturer
	//
	if (manufacturerInfo.hasManufacturerData)
	{
		out << "Manufacturer     : mfg=0x" << std::hex << std::setw(4) << std::setfill('0')
			<< manufacturerInfo.mfgId
			<< " prodType=0x" << std::setw(4) << manufacturerInfo.prodType
			<< " prodId=0x" << std::setw(4) << manufacturerInfo.prodId << "\n";

		if (manufacturerInfo.hasDeviceId)
		{
			out << "Device ID        : type=0x" << std::setw(2)
				<< unsigned(manufacturerInfo.deviceIdType)
				<< " format=0x" << std::setw(2)
				<< unsigned(manufacturerInfo.deviceIdFormat)
				<< " data=";

			out << "\n";
		}
	}
	else
	{
		out << "Manufacturer     : unknown\n";
	}

	//
	// Battery
	//
	if (batteryLevel.has_value())
		out << "Battery          : " << std::dec << unsigned(*batteryLevel) << "%\n";

	//
	// Device classes
	//
	out << "Device classes   : basic=0x" << std::hex << std::setw(2)
		<< unsigned(protocolInfo.basic)
		<< " generic=0x" << std::setw(2) << unsigned(protocolInfo.generic)
		<< " specific=0x" << std::setw(2) << unsigned(protocolInfo.specific) << "\n";

	//
	// Protocol flags
	//
	out << "Protocol flags   : listening=" << (protocolInfo.isListening ? "yes" : "no")
		<< " routing=" << (protocolInfo.isRouting ? "yes" : "no")
		<< " speed=" << std::dec << unsigned(protocolInfo.supportedSpeed)
		<< " protoVer=" << unsigned(protocolInfo.protocolVersion) << "\n";

	//
	// Optional flags
	//
	{
		std::ostringstream s;
		s << "Optional flags   : optFunc=" << (protocolInfo.optionalFunctionality ? "yes" : "no")
			<< " sensor1000ms=" << (protocolInfo.sensor1000ms ? "yes" : "no")
			<< " sensor250ms=" << (protocolInfo.sensor250ms ? "yes" : "no")
			<< " beam=" << (protocolInfo.beamCapable ? "yes" : "no")
			<< " routingEndNode=" << (protocolInfo.routingEndNode ? "yes" : "no")
			<< " specificDevice=" << (protocolInfo.specificDevice ? "yes" : "no")
			<< " controllerNode=" << (protocolInfo.controllerNode ? "yes" : "no")
			<< " security=" << (protocolInfo.security ? "yes" : "no") << "\n";
		WrapAndPrint(out, s.str());
	}

	//
	// CC values
	//
	if (basicValue)            out << "Basic            : " << unsigned(*basicValue) << "\n";
	if (switchBinaryValue)     out << "Switch Binary    : " << unsigned(*switchBinaryValue) << "\n";
	if (switchMultilevelValue) out << "Switch Multilevel: " << unsigned(*switchMultilevelValue) << "\n";
	if (sensorBinaryValue)     out << "Sensor Binary    : " << unsigned(*sensorBinaryValue) << "\n";

	if (meterInfo.hasValue)
	{
		out << "Meter            : type=" << unsigned(meterInfo.meterType)
			<< " value=" << unsigned(meterInfo.value) << "\n";
	}

	if (protectionState)
		out << "Protection       : " << unsigned(*protectionState) << "\n";

	//
	// CC report structures
	//

// ----- Configuration (0x70) -----
	{
		std::ostringstream s;
		s << "Config: ";
		for (const auto& c : configurationInfo)
		{
			if (!c.valid) continue;

			s << "P" << unsigned(c.paramNumber)
				<< "(s=" << unsigned(c.size)
				<< ",v=" << c.value
				<< ") ";

			if (c.paramNumber >= 9) break;
		}
		WrapAndPrint(out, s.str());
	}

	// ----- Multi Channel Endpoints (0x60) -----
	if (multiChannel.hasEndpointReport)
	{
		std::ostringstream s;
		s << "Endpoints(" << unsigned(multiChannel.endpointCount) << "): ";
		for (const auto& ep : multiChannel.endpoints)
		{
			s << "EP" << unsigned(ep.endpointId)
				<< "(g=0x" << std::hex << unsigned(ep.generic)
				<< ",s=0x" << unsigned(ep.specific)
				<< ",CC=[";
			for (auto cc : ep.supportedCCs)
				s << "0x" << unsigned(cc) << " ";
			s << std::dec << "]) ";
		}
		WrapAndPrint(out, s.str());
	}

	// ----- Association (0x85) -----
	{
		std::ostringstream s;
		s << "Assoc[" << unsigned(associationGroups.size()) << "]:";
		for (const auto& g : associationGroups)
		{
			s << " G" << unsigned(g.groupId) << "(" << unsigned(g.maxNodes) << ")=";
			for (auto n : g.nodeList)
				if (n.valid)
					s << unsigned(n.nodeId.value) << ",";
		}
		WrapAndPrint(out, s.str());
	}

	// ----- Multi Channel Association (0x8E) -----
	{
		std::ostringstream s;
		s << "MC Assoc[" << unsigned(multiChannelAssociationGroups.size()) << "]:";
		for (const auto& g : multiChannelAssociationGroups)
		{
			s << " G" << unsigned(g.groupId) << "(" << unsigned(g.maxNodes) << ")=";
			for (const auto& m : g.members)
			{
				if (m.valid)
				{
					s << unsigned(m.nodeId.value);
					s << "." << unsigned(m.endpointId);
					s << ",";
				}
			}
		}
		WrapAndPrint(out, s.str());
	}

	//
	// Command Classes
	//
	size_t ccCount = 0;
	for (const auto& cc : ccs)
		if (cc.supported)
			ccCount++;

	{
		std::ostringstream s;
		s << "Command Classes  : " << std::dec << ccCount << "";

		if (ccCount > 0)
		{
			s << "  ";

			for (size_t i = 0; i < ccs.size(); ++i)
			{
				if (!ccs[i].supported)
					continue;

				s << "0x" << std::hex << std::setw(2) << std::setfill('0')
					<< std::uppercase << i
					<< " v=" << std::dec << unsigned(ccs[i].version);

				if (ccs[i].versionOk)
					s << "(ok)";
			}
		}
		WrapAndPrint(out, s.str());
	}

	return out.str();
}

void ZW_Node::ProcessInterviewState()
{
	switch (GetInterviewState())
	{
	case eInterviewState::NotInterviewed:
	case eInterviewState::ProtocolInfoPending:
	case eInterviewState::ProtocolInfoDone:
	case eInterviewState::NodeInfoPending:
		break;
	case eInterviewState::NodeInfoDone:
		SetInterviewState(eInterviewState::CCVersionPending);
		// fallthrough
	case eInterviewState::CCVersionPending:
		if (auto* handler = device.GetHandler(eCommandClass::VERSION))
		{
			for (const auto ccId : GetSupportedCCs())
			{
				auto* cc = GetCC(ccId);
				if (!cc || cc->versionOk)
					continue;

				int retryCount = 0;
				do
				{
					Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE VERSION_COMMAND_CLASS_GET CC [0x{:02X}] to node {}", (uint8_t)ccId, NodeId);

					ZW_APIFrame frame;
					handler->MakeFrame(frame, ZW_CC_Version::eVersionCommand::VERSION_COMMAND_CLASS_GET, { static_cast<uint8_t>(ccId) });
					enqueue(frame);
					std::this_thread::sleep_for(std::chrono::milliseconds(10));

					if (!WaitUntil(std::chrono::seconds(5), [&]() { return cc->versionOk; }))
						Log.AddL(eLogTypes::ERR, MakeTag(), "Version timeout: CC 0x{:02X} for node {}", static_cast<uint8_t>(ccId), NodeId);
				} while (!cc->versionOk && retryCount++ < 3);
			}
		}
		else
			Log.AddL(eLogTypes::ITW, MakeTag(), "No CC VERSION_COMMAND_CLASS_GET for node {}", NodeId);
		SetInterviewState(eInterviewState::CCVersionDone);
		break;

	case eInterviewState::CCVersionDone:
		SetInterviewState(eInterviewState::CCMnfcSpecPending);
		// fallthrough
	case eInterviewState::CCMnfcSpecPending:
		if (auto* cc = GetCC(eCommandClass::MANUFACTURER_SPECIFIC))
		{
			uint8_t version = cc->version;
			if (auto* handler = device.GetHandler(eCommandClass::MANUFACTURER_SPECIFIC))
			{
				Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE MANUFACTURER_SPECIFIC_COMMAND_CLASS_GET to node {}", NodeId);

				if (version >= 1)
				{
					ZW_APIFrame frame;
					handler->MakeFrame(frame, ZW_CC_ManufacturerSpecific::eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET, { 0 });
					enqueue(frame);
					if (!WaitUntil(std::chrono::seconds(5), [&]() { return manufacturerInfo.hasManufacturerData; }))
					{
						Log.AddL(eLogTypes::ERR, MakeTag(), "Manufacturer timeout: node {}", NodeId);
						break;
					}
				}
				if (version >= 2)
				{
					ZW_APIFrame frame;
					handler->MakeFrame(frame, ZW_CC_ManufacturerSpecific::eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET_V2, { 0 });
					enqueue(frame);
					if (!WaitUntil(std::chrono::seconds(5), [&]() { return manufacturerInfo.hasDeviceId; }))
					{
						Log.AddL(eLogTypes::ERR, MakeTag(), "Manufacturer timeout: node {}", NodeId);
						break;
					}
				}
			}
		}
		else
			Log.AddL(eLogTypes::ITW, MakeTag(), "No CC MANUFACTURER_SPECIFIC_COMMAND_CLASS_GET for node {}", NodeId);
		SetInterviewState(eInterviewState::CCMnfcSpecDone);
		break;

	case eInterviewState::CCMnfcSpecDone:
		SetInterviewState(eInterviewState::CCMultiChannelPending);
		// fallthrough
	case eInterviewState::CCMultiChannelPending:
		if (auto* cc = GetCC(eCommandClass::MULTI_CHANNEL))
		{
			(void)cc;
			if (auto* handler = device.GetHandler(eCommandClass::MULTI_CHANNEL))
			{
				Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE MULTI_CHANNEL_END_POINT_GET to node {}", NodeId);

				ZW_APIFrame frame;
				handler->MakeFrame(frame, ZW_CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_END_POINT_GET, {});
				enqueue(frame);

				if (!WaitUntil(std::chrono::seconds(5), [&]() { return multiChannel.hasEndpointReport; }))
				{
					Log.AddL(eLogTypes::ERR, MakeTag(), "Multi-channel end point get timeout: node {}", NodeId);
					SetInterviewState(eInterviewState::CCMultiChannelDone);
					break;
				}

				if (multiChannel.hasEndpointReport)
				{
					for (uint8_t ep = 1; ep <= multiChannel.endpointCount; ep++)
					{
						Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE MULTI_CHANNEL_CAPABILITY_GET to node {} endpoint {}", NodeId, ep);
						handler->MakeFrame(frame, ZW_CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_GET, { ep });
						enqueue(frame);

						if (!WaitUntil(std::chrono::seconds(5), [&]() { return multiChannel.endpoints[ep - 1].hasCapabilityReport; }))
						{
							Log.AddL(eLogTypes::ERR, MakeTag(), "Multi-channel end point get capability timeout: node {} endpoint {}", NodeId, ep);
							SetInterviewState(eInterviewState::CCMultiChannelDone);
							break;
						}
					}
				}
			}
		}
		else
			Log.AddL(eLogTypes::ITW, MakeTag(), "No CC MULTI_CHANNEL_END_POINT_GET for node {}", NodeId);
		SetInterviewState(eInterviewState::CCMultiChannelDone);
		break;

	case eInterviewState::CCMultiChannelDone:
		break;
	case eInterviewState::InterviewDone:
		break;
	}
}

void ZW_Node::ProcessIsDead()
{
	ZW_APIFrame frame;
	frame.MakeSendData(NodeId,
					   3, // length of payload
					   { static_cast<uint8_t>(eCommandClass::CONFIGURATION),
						 static_cast<uint8_t>(ZW_CC_Configuration::eConfigurationCommand::CONFIGURATION_GET),
						 1 } // parameter number
	);
	enqueue(frame);

	auto& cfg = configurationInfo[1];

	if (!WaitUntil(std::chrono::seconds(5), [&]() { return cfg.valid; }))
	{
		frame.Make(eCommandIds::ZW_API_IS_NODE_FAILED, { NodeId });
		enqueue(frame);
	}
}

bool ZW_Node::HandleNodeFailed(uint8_t status)
{
	return false;
}

bool ZW_Node::ExecuteBatteryCommandJob()
{
	if (!HasCC(eCommandClass::BATTERY))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support BATTERY CC node {}", NodeId);
		return true;
	}
	auto* handler = device.GetHandler(eCommandClass::BATTERY);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for BATTERY CC node {}", NodeId);
		return true;
	}
	ZW_APIFrame frame;
	handler->MakeFrame(frame, ZW_CC_Battery::eBatteryCommand::BATTERY_GET, {});
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> BATTERY_GET: node {}", NodeId);
	enqueue(frame);
	return true;
}


bool ZW_Node::ExecuteBindCommandJob(uint8_t groupId, node_t nodeid)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteBindCommandJob: groupId={}, nodeid={}", groupId, nodeid);

	if (!HasCC(eCommandClass::ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support ASSOCIATION CC node {}", NodeId);
		return true;
	}
	auto* handler = device.GetHandler(eCommandClass::ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for ASSOCIATION CC node {}", NodeId);
		return true;
	}

	ZW_APIFrame frame;
	handler->MakeFrame(frame, ZW_CC_Association::eAssociationCommand::ASSOCIATION_SET, { groupId, nodeid.value });
	enqueue(frame);

	return true;
}

bool ZW_Node::ExecuteUnBindCommandJob(uint8_t groupId, node_t nodeid)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteUnBindCommandJob: groupId={}, nodeid={}", groupId, nodeid.value);

	if (!HasCC(eCommandClass::ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support ASSOCIATION CC node {}", NodeId);
		return true;
	}
	auto* handler = device.GetHandler(eCommandClass::ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for ASSOCIATION CC node {}", NodeId);
		return true;
	}

	ZW_APIFrame frame;
	handler->MakeFrame(frame, ZW_CC_Association::eAssociationCommand::ASSOCIATION_REMOVE, { groupId, nodeid.value });
	enqueue(frame);

	return true;
}

bool ZW_Node::ExecuteMultiChannelUnBindCommandJob(uint8_t groupId, node_t nodeid, uint8_t endpoint)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteMultiChannelUnBindCommandJob: groupId={}, nodeid={}", groupId, nodeid);

	if (!HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support MULTI_CHANNEL_ASSOCIATION CC node {}", NodeId);
		return true;
	}
	auto* handler = device.GetHandler(eCommandClass::MULTI_CHANNEL_ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for MULTI_CHANNEL_ASSOCIATION CC node {}", NodeId);
		return true;
	}

	ZW_ByteVector params = {
		groupId,
		// no nodeid's
		(uint8_t)ZW_CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REMOVE_MARKER,
		nodeid.value, endpoint
	};

	ZW_APIFrame frame;
	handler->MakeFrame(frame, ZW_CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REMOVE, params);
	enqueue(frame);

	return true;
}

bool ZW_Node::ExecuteMultiChannelBindCommandJob(uint8_t groupId, node_t nodeid, uint8_t endpoint)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteMultiChannelBindCommandJob: groupId={}, nodeid={} endpoint={}", groupId, nodeid, endpoint);

	if (!HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support MULTI_CHANNEL_ASSOCIATION CC node {}", NodeId);
		return true;
	}
	auto* handler = device.GetHandler(eCommandClass::MULTI_CHANNEL_ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for MULTI_CHANNEL_ASSOCIATION CC node {}", NodeId);
		return true;
	}

	ZW_ByteVector params = {
		groupId,
		// no nodeid's
		(uint8_t)ZW_CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_SET_MARKER,
		nodeid.value, endpoint
	};

	ZW_APIFrame frame;
	handler->MakeFrame(frame, ZW_CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_SET, params);
	enqueue(frame);

	return true;
}

bool ZW_Node::ExecuteConfigurationCommandJob(uint8_t paramNumber, eConfigSize size, uint32_t value)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteConfigurationCommandJob: paramNumber={}, size={}, value={}", paramNumber, (uint8_t)size, value);

	if (!HasCC(eCommandClass::CONFIGURATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support CONFIGURATION CC node {}", NodeId);
		return false;
	}

	auto* handler = device.GetHandler(eCommandClass::CONFIGURATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for CONFIGURATION CC node {}", NodeId);
		return true;
	}

	bool setDefault = false;
	uint8_t defaultValue = setDefault ? 0x80 : 0x00;
	std::vector<uint8_t> params;
	params.push_back(paramNumber); // Parameter number
	switch (size)
	{
	case eConfigSize::OneByte:
		params.push_back(defaultValue | 0x01); // Size 8);
		params.push_back(value); // Value
		break;
	case eConfigSize::TwoBytes:
		params.push_back(defaultValue | 0x02); // Size 16);
		params.push_back(value >> 8); // Value MSB
		params.push_back(value); // Value
		break;
	case eConfigSize::FourBytes:
		params.push_back(defaultValue | 0x04); // Size 32);
		params.push_back(value >> 24); // Value MSB
		params.push_back(value >> 16); // Value
		params.push_back(value >> 8); // Value
		params.push_back(value); // Value
		break;
	}

	ZW_APIFrame frame;
	handler->MakeFrame(frame, ZW_CmdId(ZW_CC_Configuration::eConfigurationCommand::CONFIGURATION_SET), params);
	enqueue(frame);

	return true;
}

bool ZW_Node::ExecuteAssociationInterviewJob()
{
	if (!HasCC(eCommandClass::ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support ASSOCIATION CC node {}", NodeId);
		return true;
	}
	auto* handler = device.GetHandler(eCommandClass::ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for ASSOCIATION CC node {}", NodeId);
		return true;
	}

	ZW_APIFrame frame;
	if (associationGroups.size() == 0) // only ask first time
	{
		handler->MakeFrame(frame, ZW_CC_Association::eAssociationCommand::ASSOCIATION_GROUPINGS_GET, {});
		Log.AddL(eLogTypes::DVC, MakeTag(), ">> ASSOCIATION_GROUPINGS_GET: node {}", NodeId);
		enqueue(frame);

		if (!WaitUntil(std::chrono::seconds(5), [&]() { return associationGroups.size() > 0; }))
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Association groupings timeout: node {}", NodeId);
			return false;
		}
	}

	for (size_t i = 0; i < associationGroups.size(); i++)
	{
		uint8_t groupId = associationGroups[i].groupId;
		associationGroups[i].hasLastReport = false;
		handler->MakeFrame(frame, ZW_CC_Association::eAssociationCommand::ASSOCIATION_GET, { groupId });
		Log.AddL(eLogTypes::DVC, MakeTag(), ">> ASSOCIATION_GET: node {} group {}", NodeId, groupId);
		enqueue(frame);

		if (!WaitUntil(std::chrono::seconds(5), [&]() { return associationGroups[i].hasLastReport; }))
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Association get timeout: node {} group {}", NodeId, groupId);
			return false;
		}
	}
	/*
	if (!associationGroups.empty())
	{
		bool found = false;
		uint8_t controllerId = 1;
		for (auto DVC : associationGroups[0].nodeList)
		{
			if (DVC == controllerId)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			associationHandler->MakeFrame(frame, ZW_CC_Association::eAssociationCommand::ASSOCIATION_SET, { 1, controllerId });
			Log.AddL(eLogTypes::DVC, MakeTag(), ">> ASSOCIATION_SET: node {} group 1 + controllerId", NodeId);
			enqueue(frame);
			return true;
		}
	}
	*/
	return true;
}

bool ZW_Node::ExecuteMultiChannelAssociationInterviewJob()
{
	if (!HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support MULTI_CHANNEL_ASSOCIATION CC node {}", NodeId);
		return true;
	}

	auto* mcaHandler = device.GetHandler(eCommandClass::MULTI_CHANNEL_ASSOCIATION);
	if (!mcaHandler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for MULTI_CHANNEL_ASSOCIATION CC node {}", NodeId);
		return true;
	}

	ZW_APIFrame frame;
	if (multiChannelAssociationGroups.size() == 0) // only ask first time
	{
		mcaHandler->MakeFrame(frame, ZW_CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET, {});
		Log.AddL(eLogTypes::DVC, MakeTag(), ">> MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET: node {}", NodeId);
		enqueue(frame);

		if (!WaitUntil(std::chrono::seconds(5), [&]() { return multiChannelAssociationGroups.size() > 0; }))
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Association groupings timeout: node {}", NodeId);
			return false;
		}
	}

	for (auto& group : multiChannelAssociationGroups)
	{
		uint8_t groupId = group.groupId;
		group.hasLastReport = false;
		mcaHandler->MakeFrame(frame, ZW_CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GET, { groupId });
		Log.AddL(eLogTypes::DVC, MakeTag(), ">> MULTI_CHANNEL_ASSOCIATION_GET: node {} group {}", NodeId, groupId);
		enqueue(frame);

		if (!WaitUntil(std::chrono::seconds(5), [&]() { return group.hasLastReport; }))
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Multi channel association get timeout: node {} group {}", NodeId, groupId);
			return false;
		}
	}
	return true;
}

bool ZW_Node::ExecuteConfigurationInterviewJob(uint8_t startparam, uint8_t numParams)
{
	if (!HasCC(eCommandClass::CONFIGURATION))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "Node does not support CONFIGURATION CC node {}", NodeId);
		return true;
	}

	auto* cfgHandler = device.GetHandler(eCommandClass::CONFIGURATION);
	if (!cfgHandler)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "No handler for CONFIGURATION CC node {}", NodeId);
		return true;
	}

	for (size_t param = startparam; param < configurationInfo.size() && param < numParams + startparam; ++param)
	{
		auto& cfg = configurationInfo[param];
		cfg.valid = false;

		ZW_APIFrame frame;
		cfgHandler->MakeFrame(frame, ZW_CC_Configuration::eConfigurationCommand::CONFIGURATION_GET, { static_cast<uint8_t>(param) });

		Log.AddL(eLogTypes::DVC, MakeTag(), ">> CONFIGURATION_GET: node {} param {}", NodeId, param);

		enqueue(frame);

		// Wait for report
		bool ok = WaitUntil(std::chrono::seconds(3), [&]() { return cfg.valid; });

		if (!ok)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "No CONFIGURATION_REPORT for param {} on node {} — stopping interview", cfg.paramNumber, NodeId);
			NotifyUI(UINotify::NodeConfigChanged, NodeId);
			return false;
		}

		Log.AddL(eLogTypes::DVC, MakeTag(), "<< CONFIGURATION_REPORT: node {} param {} size {} value {}",
				 NodeId,
				 cfg.paramNumber,
				 cfg.size,
				 cfg.value);
	}

	Log.AddL(eLogTypes::DVC, MakeTag(), "Configuration interview completed for node {}", NodeId);
	NotifyUI(UINotify::NodeConfigChanged, NodeId);
	return true;
}
