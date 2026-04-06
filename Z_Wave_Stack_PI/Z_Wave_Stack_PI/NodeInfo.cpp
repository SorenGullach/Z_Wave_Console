
#include "NodeInfo.h"
#include "Notify.h"

NodeInfo::NodeInfo(nodeid_t nodeid)
	: nodeId(nodeid),
	ccHandlerFactory(*this)
{
}

void NodeInfo::SetProtocolInfo(const ProtocolInfo& DVC)
{
    {
		DebugLockGuard lock(stateMutex);
		protocolInfo = DVC;
	}
	NotifyUI(UINotify::NodeChanged, nodeId);
}

bool NodeInfo::IsListening() const
{
    DebugLockGuard lock(stateMutex);
	return protocolInfo.isListening;
}

void NodeInfo::SetWakeUpInfo(const WakeUpInfo& wakeup)
{
    {
		DebugLockGuard lock(stateMutex);
		wakeUpInfo = wakeup;
	}
	NotifyUI(UINotify::NodeChanged, nodeId);
}

NodeInfo::WakeUpInfo NodeInfo::GetWakeUpInfo() const
{
    DebugLockGuard lock(stateMutex);
	return wakeUpInfo;
}

void NodeInfo::SetNIF(uint8_t basicType, uint8_t genericClass, uint8_t specificClass,
	const std::vector<uint8_t>& commandClasses)
{
	{
        DebugLockGuard lock(stateMutex);
		protocolInfo.basic = basicType;
		protocolInfo.generic = genericClass;
		protocolInfo.specific = specificClass;
		ccs.fill({});
		for (auto cc : commandClasses)
		{
			ccs[cc].id = static_cast<eCommandClass>(cc);
			ccs[cc].supported = supportsCC(ccs[cc].id);
		}

		if (ccs[static_cast<uint8_t>(eCommandClass::BASIC)].supported) ccHandlerFactory.AddHandler<CC_Basic>();
		if (ccs[static_cast<uint8_t>(eCommandClass::VERSION)].supported) ccHandlerFactory.AddHandler<CC_Version>();
		if (ccs[static_cast<uint8_t>(eCommandClass::MANUFACTURER_SPECIFIC)].supported) ccHandlerFactory.AddHandler<CC_ManufacturerSpecific>();
		if (ccs[static_cast<uint8_t>(eCommandClass::BATTERY)].supported) ccHandlerFactory.AddHandler<CC_Battery>();
		if (ccs[static_cast<uint8_t>(eCommandClass::SWITCH_BINARY)].supported) ccHandlerFactory.AddHandler<CC_SwitchBinary>();
		if (ccs[static_cast<uint8_t>(eCommandClass::BASIC)].supported) ccHandlerFactory.AddHandler<CC_Basic>();
		if (ccs[static_cast<uint8_t>(eCommandClass::SWITCH_MULTILEVEL)].supported) ccHandlerFactory.AddHandler<CC_SwitchMultilevel>();
		if (ccs[static_cast<uint8_t>(eCommandClass::SENSOR_BINARY)].supported) ccHandlerFactory.AddHandler<CC_SensorBinary>();
		if (ccs[static_cast<uint8_t>(eCommandClass::METER)].supported) ccHandlerFactory.AddHandler<CC_Meter>();
		if (ccs[static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL)].supported) ccHandlerFactory.AddHandler<CC_MultiChannel>();
		if (ccs[static_cast<uint8_t>(eCommandClass::CONFIGURATION)].supported) ccHandlerFactory.AddHandler<CC_Configuration>();
		if (ccs[static_cast<uint8_t>(eCommandClass::PROTECTION)].supported) ccHandlerFactory.AddHandler<CC_Protection>();
		if (ccs[static_cast<uint8_t>(eCommandClass::ASSOCIATION)].supported) ccHandlerFactory.AddHandler<CC_Association>();
		if (ccs[static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION)].supported) ccHandlerFactory.AddHandler<CC_MultiChannelAssociation>();
		if (ccs[static_cast<uint8_t>(eCommandClass::WAKE_UP)].supported) ccHandlerFactory.AddHandler<CC_WakeUp>();
	}
	NotifyUI(UINotify::NodeChanged, nodeId);
}

NodeInfo::CommandClassTag* NodeInfo::GetCC(eCommandClass ccId)
{
 DebugLockGuard lock(stateMutex);
	auto& cc = ccs[static_cast<uint8_t>(ccId)];
	return cc.supported ? &cc : nullptr;
}

const NodeInfo::CommandClassTag* NodeInfo::GetCC(eCommandClass ccId) const
{
   DebugLockGuard lock(stateMutex);
	const auto& cc = ccs[static_cast<uint8_t>(ccId)];
	return cc.supported ? &cc : nullptr;
}

bool NodeInfo::HasCC(eCommandClass ccId) const
{
   DebugLockGuard lock(stateMutex);
	return ccs[static_cast<uint8_t>(ccId)].supported;
}

std::vector<eCommandClass> NodeInfo::GetSupportedCCs() const
{
  DebugLockGuard lock(stateMutex);
	std::vector<eCommandClass> result;
	for (const auto& cc : ccs)
		if (cc.supported)
			result.push_back(cc.id);
	return result;
}

void NodeInfo::SetInterviewState(eInterviewState state)
{
 {
		DebugLockGuard lock(stateMutex);
		interviewState = state;
	}
	NotifyUI(UINotify::NodeChanged, nodeId);
}

NodeInfo::eInterviewState NodeInfo::GetInterviewState() const
{
  DebugLockGuard lock(stateMutex);
	return interviewState;
}

NodeInfo::eNodeState NodeInfo::GetState() const
{
   DebugLockGuard lock(stateMutex);
	return nodeState;
}

void NodeInfo::WakeUp()
{
    {
		DebugLockGuard lock(stateMutex);
		nodeState = eNodeState::Awake;
	}
	NotifyUI(UINotify::NodeChanged, nodeId);
}

void NodeInfo::Sleeping()
{
    {
		DebugLockGuard lock(stateMutex);
		nodeState = eNodeState::Sleepy;
	}
	NotifyUI(UINotify::NodeChanged, nodeId);
}

bool NodeInfo::supportsCC(eCommandClass cc) const
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

