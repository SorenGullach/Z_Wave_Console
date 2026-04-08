#include "Node.h"
#include "NodeCommandClasses.h"

// ---------- CC Dispatch ----------
void Node::HandleCCDeviceReport(eCommandClass cmdClass, ccid_t cmdId, const ccparams_t& cmdParams)
{
	ccparams_t innerParams = cmdParams;
	uint8_t destiationEndpoint = 0;
	if (cmdClass == eCommandClass::MULTI_CHANNEL &&
		cmdId.value == static_cast<uint8_t>(CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP))
	{
		// pull out the encapsulated command
		// | 0x60 | 0x06 | DE | CC | CMD | PAYLOAD...
		if (cmdParams.size() >= 3)
		{
			destiationEndpoint = cmdParams[0];
			cmdClass = static_cast<eCommandClass>(cmdParams[1]);
			cmdId = ccid_t(cmdParams[2]);
			innerParams.assign(cmdParams.begin() + 3, cmdParams.end());
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "Encapsulated command class: 0x{:02X} {} (to endpoint {})",
					 (uint8_t)cmdClass, CommandClassToString(cmdClass), destiationEndpoint);
		}
		else
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Malformed multi-channel encapsulated command");
		}
	}
	auto handler = ccHandlerFactory.GetHandler(cmdClass);
	if (handler)
	{
		handler->HandleReport(cmdId, destiationEndpoint, innerParams);
	}
	else
		Log.AddL(eLogTypes::DVC, MakeTag(), "Unknown command class handler: 0x{:02X} {}", (uint8_t)cmdClass, CommandClassToString(cmdClass));
}

void Node::ProcessInterviewState()
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
		if (auto* handler = ccHandlerFactory.GetHandler(eCommandClass::VERSION))
		{
			for (const auto ccId : GetSupportedCCs())
			{
				auto* cc = GetCC(ccId);
				if (!cc || cc->versionOk)
					continue;

				int retryCount = 0;
				do
				{
					Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE VERSION_COMMAND_CLASS_GET CC [0x{:02X}] to node {}", (uint8_t)ccId, nodeId);

					APIFrame frame;
					handler->MakeFrame(frame, CC_Version::eVersionCommand::VERSION_COMMAND_CLASS_GET, { static_cast<uint8_t>(ccId) });
					enqueue(frame);
					std::this_thread::sleep_for(std::chrono::milliseconds(10));

					if (!WaitUntil(std::chrono::seconds(5), [&]() { return cc->versionOk; }))
						Log.AddL(eLogTypes::ERR, MakeTag(), "Version timeout: CC 0x{:02X} for node {}", static_cast<uint8_t>(ccId), nodeId);
				} while (!cc->versionOk && retryCount++ < 3);
			}
		}
		else
			Log.AddL(eLogTypes::ITW, MakeTag(), "No CC VERSION_COMMAND_CLASS_GET for node {}", nodeId);
		SetInterviewState(eInterviewState::CCVersionDone);
		break;

	case eInterviewState::CCVersionDone:
		SetInterviewState(eInterviewState::CCMnfcSpecPending);
		// fallthrough
	case eInterviewState::CCMnfcSpecPending:
		if (auto* cc = GetCC(eCommandClass::MANUFACTURER_SPECIFIC))
		{
			uint8_t version = cc->version;
			if (auto* handler = ccHandlerFactory.GetHandler(eCommandClass::MANUFACTURER_SPECIFIC))
			{
				Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE MANUFACTURER_SPECIFIC_COMMAND_CLASS_GET to node {}", nodeId);

				if (version >= 1)
				{
					APIFrame frame;
					handler->MakeFrame(frame, CC_ManufacturerSpecific::eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET, { 0 });
					enqueue(frame);
					if (!WaitUntil(std::chrono::seconds(5), [&]() { return manufacturerInfo.hasManufacturerData; }))
					{
						Log.AddL(eLogTypes::ERR, MakeTag(), "Manufacturer timeout: node {}", nodeId);
						break;
					}
				}
				if (version >= 2)
				{
					APIFrame frame;
					handler->MakeFrame(frame, CC_ManufacturerSpecific::eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET_V2, { 0 });
					enqueue(frame);
					if (!WaitUntil(std::chrono::seconds(5), [&]() { return manufacturerInfo.hasDeviceId; }))
					{
						Log.AddL(eLogTypes::ERR, MakeTag(), "Manufacturer timeout: node {}", nodeId);
						break;
					}
				}
			}
		}
		else
			Log.AddL(eLogTypes::ITW, MakeTag(), "No CC MANUFACTURER_SPECIFIC_COMMAND_CLASS_GET for node {}", nodeId);
		SetInterviewState(eInterviewState::CCMnfcSpecDone);
		break;

	case eInterviewState::CCMnfcSpecDone:
		SetInterviewState(eInterviewState::CCMultiChannelPending);
		// fallthrough
	case eInterviewState::CCMultiChannelPending:
		if (auto* cc = GetCC(eCommandClass::MULTI_CHANNEL))
		{
			(void)cc;
			if (auto* handler = ccHandlerFactory.GetHandler(eCommandClass::MULTI_CHANNEL))
			{
				Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE MULTI_CHANNEL_END_POINT_GET to node {}", nodeId);

				APIFrame frame;
				handler->MakeFrame(frame, CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_END_POINT_GET, {});
				enqueue(frame);

				if (!WaitUntil(std::chrono::seconds(5), [&]() { return multiChannel.hasEndpointReport; }))
				{
					Log.AddL(eLogTypes::ERR, MakeTag(), "Multi-channel end point get timeout: node {}", nodeId);
					SetInterviewState(eInterviewState::CCMultiChannelDone);
					break;
				}

				if (multiChannel.hasEndpointReport)
				{
					for (uint8_t ep = 1; ep <= multiChannel.endpointCount; ep++)
					{
						Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE MULTI_CHANNEL_CAPABILITY_GET to node {} endpoint {}", nodeId, ep);
						handler->MakeFrame(frame, CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_GET, { ep });
						enqueue(frame);

						if (!WaitUntil(std::chrono::seconds(5), [&]() { return multiChannel.endpoints[ep - 1].hasCapabilityReport; }))
						{
							Log.AddL(eLogTypes::ERR, MakeTag(), "Multi-channel end point get capability timeout: node {} endpoint {}", nodeId, ep);
							SetInterviewState(eInterviewState::CCMultiChannelDone);
							break;
						}
					}
				}
			}
		}
		else
			Log.AddL(eLogTypes::ITW, MakeTag(), "No CC MULTI_CHANNEL_END_POINT_GET for node {}", nodeId);
		SetInterviewState(eInterviewState::CCMultiChannelDone);
		break;

	case eInterviewState::CCMultiChannelDone:
		break;
	case eInterviewState::InterviewDone:
		break;
	}
}

bool Node::ExecuteBatteryCommandJob()
{
	if (!HasCC(eCommandClass::BATTERY))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support BATTERY CC node {}", nodeId);
		return true;
	}
	auto* handler = ccHandlerFactory.GetHandler(eCommandClass::BATTERY);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for BATTERY CC node {}", nodeId);
		return true;
	}
	APIFrame frame;
	handler->MakeFrame(frame, CC_Battery::eBatteryCommand::BATTERY_GET, {});
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> BATTERY_GET: node {}", nodeId);
	enqueue(frame);
	return true;
}

bool Node::ExecuteSwitchBinaryCommandJob(uint8_t value)
{
	if (!HasCC(eCommandClass::SWITCH_BINARY))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support SWITCH_BINARY CC node {}", nodeId);
		return true;
	}

	auto* handler = ccHandlerFactory.GetHandler(eCommandClass::SWITCH_BINARY);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for SWITCH_BINARY CC node {}", nodeId);
		return true;
	}

	APIFrame frame;
	handler->MakeFrame(frame, CC_SwitchBinary::eSwitchBinaryCommand::SWITCH_BINARY_SET, { value });
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> SWITCH_BINARY_SET: node {} value=0x{:02X}", nodeId, value);
	enqueue(frame);

	return true;
}


bool Node::ExecuteBindCommandJob(uint8_t groupId, nodeid_t nodeid)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteBindCommandJob: groupId={}, nodeid={}", groupId, nodeId.Value());

	if (!HasCC(eCommandClass::ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support ASSOCIATION CC node {}", nodeId.Value());
		return true;
	}
	auto* handler = ccHandlerFactory.GetHandler(eCommandClass::ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for ASSOCIATION CC node {}", nodeId.Value());
		return true;
	}

	APIFrame frame;
	handler->MakeFrame(frame, CC_Association::eAssociationCommand::ASSOCIATION_SET, { groupId, nodeid.Value() });
	enqueue(frame);

	return true;
}

bool Node::ExecuteUnBindCommandJob(uint8_t groupId, nodeid_t nodeid)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteUnBindCommandJob: groupId={}, nodeid={}", groupId, nodeId.Value());

	if (!HasCC(eCommandClass::ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support ASSOCIATION CC node {}", nodeId.Value());
		return true;
	}
	auto* handler = ccHandlerFactory.GetHandler(eCommandClass::ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for ASSOCIATION CC node {}", nodeId.Value());
		return true;
	}

	APIFrame frame;
	handler->MakeFrame(frame, CC_Association::eAssociationCommand::ASSOCIATION_REMOVE, { groupId, nodeid.Value() });
	enqueue(frame);

	return true;
}

bool Node::ExecuteMultiChannelUnBindCommandJob(uint8_t groupId, nodeid_t nodeid, uint8_t endpoint)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteMultiChannelUnBindCommandJob: groupId={}, nodeid={}", groupId, nodeId.Value());

	if (!HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support MULTI_CHANNEL_ASSOCIATION CC node {}", nodeId.Value());
		return true;
	}
	auto* handler = ccHandlerFactory.GetHandler(eCommandClass::MULTI_CHANNEL_ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for MULTI_CHANNEL_ASSOCIATION CC node {}", nodeId.Value());
		return true;
	}

	ccparams_t params = {
		groupId,
		// no nodeid's
		(uint8_t)CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REMOVE_MARKER,
		nodeid.Value(), endpoint
	};

	APIFrame frame;
	handler->MakeFrame(frame, CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REMOVE, params);
	enqueue(frame);

	return true;
}

bool Node::ExecuteMultiChannelBindCommandJob(uint8_t groupId, nodeid_t nodeid, uint8_t endpoint)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteMultiChannelBindCommandJob: groupId={}, nodeid={} endpoint={}", groupId, nodeid, endpoint);

	if (!HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support MULTI_CHANNEL_ASSOCIATION CC node {}", nodeId);
		return true;
	}
	auto* handler = ccHandlerFactory.GetHandler(eCommandClass::MULTI_CHANNEL_ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for MULTI_CHANNEL_ASSOCIATION CC node {}", nodeId);
		return true;
	}

	ccparams_t params = {
		groupId,
		// no nodeid's
		(uint8_t)CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_SET_MARKER,
		nodeId.Value(), endpoint
	};

	APIFrame frame;
	handler->MakeFrame(frame, CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_SET, params);
	enqueue(frame);

	return true;
}

bool Node::ExecuteConfigurationCommandJob(uint8_t paramNumber, eConfigSize size, uint32_t value)
{
	Log.AddL(eLogTypes::DVC, MakeTag(), "ExecuteConfigurationCommandJob: paramNumber={}, size={}, value={}", paramNumber, (uint8_t)size, value);

	if (!HasCC(eCommandClass::CONFIGURATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support CONFIGURATION CC node {}", nodeId);
		return false;
	}

	auto* handler = ccHandlerFactory.GetHandler(eCommandClass::CONFIGURATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for CONFIGURATION CC node {}", nodeId);
		return true;
	}

	bool setDefault = false;
	uint8_t defaultValue = setDefault ? 0x80 : 0x00;
	ccparams_t params;
	params.push_back(paramNumber); // Parameter number
	switch (size)
	{
	case eConfigSize::OneByte:
		params.push_back(defaultValue | 0x01); // Size 8);
		params.push_back((uint8_t)value); // Value
		break;
	case eConfigSize::TwoBytes:
		params.push_back(defaultValue | 0x02); // Size 16);
		params.push_back((uint8_t)(value >> 8)); // Value MSB
		params.push_back((uint8_t)value); // Value
		break;
	case eConfigSize::FourBytes:
		params.push_back(defaultValue | 0x04); // Size 32);
		params.push_back((uint8_t)(value >> 24)); // Value MSB
		params.push_back((uint8_t)(value >> 16)); // Value
		params.push_back((uint8_t)(value >> 8)); // Value
		params.push_back((uint8_t)value); // Value
		break;
	}

	APIFrame frame;
	handler->MakeFrame(frame, CC_Configuration::eConfigurationCommand::CONFIGURATION_SET, params);
	enqueue(frame);

	ExecuteConfigurationInterviewJob(paramNumber, 1);

	return true;
}

bool Node::ExecuteAssociationInterviewJob()
{
	if (!HasCC(eCommandClass::ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support ASSOCIATION CC node {}", nodeId);
		return true;
	}
	auto* handler = ccHandlerFactory.GetHandler(eCommandClass::ASSOCIATION);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for ASSOCIATION CC node {}", nodeId);
		return true;
	}

	APIFrame frame;
	//if (associationGroups.size() == 0) // only ask first time
	if (multiChannelAssociationGroups.size() == 0) // only ask first time
	{
		handler->MakeFrame(frame, CC_Association::eAssociationCommand::ASSOCIATION_GROUPINGS_GET, {});
		Log.AddL(eLogTypes::DVC, MakeTag(), ">> ASSOCIATION_GROUPINGS_GET: node {}", nodeId);
		enqueue(frame);

		//if (!WaitUntil(std::chrono::seconds(5), [&]() { return associationGroups.size() > 0; }))
		if (!WaitUntil(std::chrono::seconds(5), [&]() { return multiChannelAssociationGroups.size() > 0; }))
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Association groupings timeout: node {}", nodeId);
			return false;
		}
	}

	//	for (size_t i = 0; i < associationGroups.size(); i++)
	for (size_t i = 0; i < multiChannelAssociationGroups.size(); i++)
	{
		//uint8_t groupId = associationGroups[i].groupId;
		uint8_t groupId = multiChannelAssociationGroups[i].groupId;
		//associationGroups[i].hasLastReport = false;
		multiChannelAssociationGroups[i].hasLastReport = false;
		handler->MakeFrame(frame, CC_Association::eAssociationCommand::ASSOCIATION_GET, { groupId });
		Log.AddL(eLogTypes::DVC, MakeTag(), ">> ASSOCIATION_GET: node {} group {}", nodeId, groupId);
		enqueue(frame);

		//if (!WaitUntil(std::chrono::seconds(5), [&]() { return associationGroups[i].hasLastReport; }))
		if (!WaitUntil(std::chrono::seconds(5), [&]() { return multiChannelAssociationGroups[i].hasLastReport; }))
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Association get timeout: node {} group {}", nodeId, groupId);
			return false;
		}
	}
	NotifyUI(UINotify::NodeChanged, nodeId);
	return true;
}

bool Node::ExecuteMultiChannelAssociationInterviewJob()
{
	if (!HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION))
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support MULTI_CHANNEL_ASSOCIATION CC node {}", nodeId);
		return true;
	}

	auto* mcaHandler = ccHandlerFactory.GetHandler(eCommandClass::MULTI_CHANNEL_ASSOCIATION);
	if (!mcaHandler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "No handler for MULTI_CHANNEL_ASSOCIATION CC node {}", nodeId);
		return true;
	}

	APIFrame frame;
	if (multiChannelAssociationGroups.size() == 0) // only ask first time
	{
		mcaHandler->MakeFrame(frame, CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET, {});
		Log.AddL(eLogTypes::DVC, MakeTag(), ">> MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET: node {}", nodeId);
		enqueue(frame);

		if (!WaitUntil(std::chrono::seconds(5), [&]() { return multiChannelAssociationGroups.size() > 0; }))
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Association groupings timeout: node {}", nodeId);
			return false;
		}
	}

	for (auto& group : multiChannelAssociationGroups)
	{
		uint8_t groupId = group.groupId;
		group.hasLastReport = false;
		mcaHandler->MakeFrame(frame, CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GET, { groupId });
		Log.AddL(eLogTypes::DVC, MakeTag(), ">> MULTI_CHANNEL_ASSOCIATION_GET: node {} group {}", nodeId, groupId);
		enqueue(frame);

		if (!WaitUntil(std::chrono::seconds(5), [&]() { return group.hasLastReport; }))
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Multi channel association get timeout: node {} group {}", nodeId, groupId);
			return false;
		}
	}
	NotifyUI(UINotify::NodeChanged, nodeId);
	return true;
}

bool Node::ExecuteConfigurationInterviewJob(uint8_t startparam, uint8_t numParams)
{
	if (!HasCC(eCommandClass::CONFIGURATION))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "Node does not support CONFIGURATION CC node {}", nodeId);
		return true;
	}

	auto* cfgHandler = ccHandlerFactory.GetHandler(eCommandClass::CONFIGURATION);
	if (!cfgHandler)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "No handler for CONFIGURATION CC node {}", nodeId);
		return true;
	}

	for (size_t param = startparam; param < configurationInfo.size() && param < numParams + startparam; ++param)
	{
		auto& cfg = configurationInfo[param];
		cfg.valid = false;

		APIFrame frame;
		cfgHandler->MakeFrame(frame, CC_Configuration::eConfigurationCommand::CONFIGURATION_GET, { static_cast<uint8_t>(param) });

		Log.AddL(eLogTypes::DVC, MakeTag(), ">> CONFIGURATION_GET: node {} param {}", nodeId, param);

		enqueue(frame);

		// Wait for report
		bool ok = WaitUntil(std::chrono::seconds(3), [&]() { return cfg.valid; });

		if (!ok)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "No CONFIGURATION_REPORT for param {} on node {} — stopping interview", cfg.paramNumber, nodeId);
			return false;
		}

		Log.AddL(eLogTypes::DVC, MakeTag(), "<< CONFIGURATION_REPORT: node {} param {} size {} value {}",
				 nodeId,
				 cfg.paramNumber,
				 cfg.size,
				 cfg.value);
	}

	Log.AddL(eLogTypes::DVC, MakeTag(), "Configuration interview completed for node {}", nodeId);
	NotifyUI(UINotify::NodeChanged, nodeId);
	return true;
}
