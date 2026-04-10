#include "Node.h"
#include "NodeCommandClasses.h"

// ---------- CC Dispatch ----------
void Node::HandleCCDeviceReport(eCommandClass cmdClass, ccid_t cmdId, const ccparams_t& cmdParams)
{
	// Multi Channel Encapsulation?
	if (cmdClass == eCommandClass::MULTI_CHANNEL &&
		//cmdId.value == static_cast<uint8_t>(CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP))
		cmdId == CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP)
	{
		// | 0x60 | 0x06 | destEP | CC | CMD | PAYLOAD...
		if (cmdParams.size() < 3)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Malformed multi-channel encapsulated command");
			return;
		}

		uint8_t destinationEndpoint = cmdParams[0];
		eCommandClass innerCC = static_cast<eCommandClass>(cmdParams[1]);
		ccid_t innerCCId = cmdParams[2];

		ccparams_t innerParams(cmdParams.begin() + 3, cmdParams.end());

		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "Encapsulated command class: 0x{:02X} {} (to endpoint {})",
				 (uint8_t)innerCC, CommandClassToString(innerCC), destinationEndpoint);

		// Look up handler for the *inner* CC
		auto innerHandler = ccHandlerFactory.GetHandler(innerCC);
		if (!innerHandler)
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "Unknown encapsulated command class handler: 0x{:02X} {}",
					 (uint8_t)innerCC, CommandClassToString(innerCC));
			return;
		}

		innerHandler->HandleReport(innerCCId, innerParams, destinationEndpoint);
		return;
	}

	// Normal (non-encapsulated) CC
	auto handler = ccHandlerFactory.GetHandler(cmdClass);
	if (!handler)
	{
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "Unknown command class handler: 0x{:02X} {}",
				 (uint8_t)cmdClass, CommandClassToString(cmdClass));
		return;
	}

	handler->HandleReport(cmdId, cmdParams, 0);
}

/* ========================================================================
   Z-Wave Node Interview — Overview
   ========================================================================

	ProcessInterviewState()
		NodeInfoDone ->
			CCVersionPending ->
				query VERSION for each supported Command Class ->
			CCVersionDone ->
			CCMnfcSpecPending ->
				query MANUFACTURER_SPECIFIC data/device id when supported ->
			CCMnfcSpecDone ->
			CCMultiChannelPending ->
				query MULTI_CHANNEL endpoint report and endpoint capabilities when supported ->
			CCMultiChannelDone ->
				enqueue post-interview jobs:
					ASSOCIATION_INTERVIEW
					MULTI_CHANNEL_ASSOCIATION_INTERVIEW
					CONFIGURATION_INTERVIEW
				set InterviewDone

		If a required Command Class is not supported, the state machine logs it and
		continues to the next interview phase.

		Listening nodes typically enter this flow after NodeInterview reaches
		NodeInfoDone.

		Non-listening/sleepy nodes continue asynchronously and should transition to
		InterviewDone themselves, then perform post-interview jobs there.

   ======================================================================== */

void Node::ProcessInterviewState()
{
	switch (GetInterviewState())
	{
	case eInterviewState::NotInterviewed:
	case eInterviewState::ProtocolInfoPending: // handled on Controller::Interview
	case eInterviewState::ProtocolInfoDone:  // handled on Controller::Interview
	case eInterviewState::NodeInfoPending:  // handled on Controller::Interview
		break;
	case eInterviewState::NodeInfoDone:  // handled on Controller::Interview, set to NodeInfoDone if is listening or after node wakeup
		SetInterviewState(eInterviewState::CCVersionPending);
		// fallthrough
	case eInterviewState::CCVersionPending:
		if (auto* handler = ccHandlerFactory.GetHandler(eCommandClass::VERSION))
		{
			for (const auto supportedCCId : GetSupportedCCs())
			{
				auto* cc = GetCC(supportedCCId);
				if (!cc || cc->versionOk)
					continue;

				int retryCount = 0;
				do
				{
					Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE VERSION_COMMAND_CLASS_GET CC [0x{:02X}] to node {}", supportedCCId, nodeId);

					APIFrame frame;
					handler->MakeFrame(frame, CC_Version::eVersionCommand::VERSION_COMMAND_CLASS_GET, { supportedCCId });
					enqueue(frame);
					std::this_thread::sleep_for(std::chrono::milliseconds(10));

					if (!WaitUntil(std::chrono::seconds(5), [&]() { return cc->versionOk; }))
						Log.AddL(eLogTypes::ERR, MakeTag(), "Version timeout: CC 0x{:02X} for node {}", supportedCCId, nodeId);
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
			if (auto* handler = ccHandlerFactory.GetHandler(eCommandClass::MANUFACTURER_SPECIFIC))
			{
				(void)cc;
				uint8_t version = handler->Version();
				Log.AddL(eLogTypes::DVC, MakeTag(), ">> NODE MANUFACTURER_SPECIFIC_COMMAND_CLASS_GET to node {}", nodeId);

				if (version >= 1)
				{
					APIFrame frame;
					handler->MakeFrame(frame, CC_ManufacturerSpecific::eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET, { });
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
		{
			Node::Job job;

			// the merten 506004
			if (manufacturerInfo.mfgId == 0x007A && manufacturerInfo.prodType == 0x0003 && manufacturerInfo.prodId == 0x0004)
			{
				for (int param = 0; param <= 7; param++)
				{
					job.job = Node::eJobs::CONFIGURATION_COMMAND;
					job.group = param;
					job.value = 22; // toggle on both buttons(top/down)
					job.cfgSize = eConfigSize::OneByte;
					EnqueueJob(job);
				}
				for (int groupid = 1; groupid <= 4; groupid++)
				{
					job.job = Node::eJobs::MULTI_CHANNEL_BIND_COMMAND;
					job.group = groupid;
					job.nodeId = nodeid_t(1); // controller
					job.endpoint = groupid;
					EnqueueJob(job);
				}
			}

			job.job = Node::eJobs::ASSOCIATION_INTERVIEW;
			EnqueueJob(job);
			job.job = Node::eJobs::MULTI_CHANNEL_ASSOCIATION_INTERVIEW;
			EnqueueJob(job);
			job.job = Node::eJobs::CONFIGURATION_INTERVIEW;
			job.group = 0; // start with param 0
			job.value = 10; // get first 10 params
			EnqueueJob(job);
			SetInterviewState(Node::eInterviewState::InterviewDone);

			std::cout << ToString();
		}
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
		nodeid.Value(), endpoint
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
	std::cout << ToString(); // TODO remove when debug done
	return true;
}
