

#include "Device.h"
#include "Node.h"
#include "Logging.h"

//
// VERSION
//
void ZW_CC_Version::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params)
{
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 1,
					   { static_cast<uint8_t>(eCommandClass::VERSION),
						 static_cast<uint8_t>(eVersionCommand::VERSION_COMMAND_CLASS_GET),
						 params[0] });
}

void ZW_CC_Version::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eVersionCommand::VERSION_COMMAND_CLASS_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< VERSION unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	Log.AddL(eLogTypes::INFO, MakeTag(), "<< VERSION_COMMAND_CLASS_REPORT: node={} cc=0x{:02X} version={}",
			 node.NodeId,
			 params.size() > 0 ? params[0] : 0,
			 params.size() > 1 ? params[1] : 0);
}

//
// BATTERY
//
void ZW_CC_Battery::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>&)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> BATTERY_GET: node {}", node.NodeId);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 3,
					   { static_cast<uint8_t>(eCommandClass::BATTERY),
						 static_cast<uint8_t>(eBatteryCommand::BATTERY_GET) });
}

void ZW_CC_Battery::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eBatteryCommand::BATTERY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< BATTERY unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	if (params.empty())
		return;

	const uint8_t level = params[0];
	node.batteryLevel = level;   // from your existing code
	Log.AddL(eLogTypes::INFO, MakeTag(), "<< BATTERY_REPORT: node={} level={}", node.NodeId, level);
}

//
// SWITCH BINARY
//
void ZW_CC_SwitchBinary::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>&)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> SWITCH_BINARY_GET: node {}", node.NodeId);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 3,
					   { static_cast<uint8_t>(eCommandClass::SWITCH_BINARY),
						 static_cast<uint8_t>(eSwitchBinaryCommand::SWITCH_BINARY_GET) });
}

void ZW_CC_SwitchBinary::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eSwitchBinaryCommand::SWITCH_BINARY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SWITCH_BINARY unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	if (params.empty())
		return;

	const uint8_t value = params[0];
	node.switchBinaryValue = value;
	Log.AddL(eLogTypes::INFO, MakeTag(), "<< SWITCH_BINARY_REPORT: node={} value=0x{:02X}",
			 node.NodeId, value);
}

//
// BASIC (0x20)
//
void ZW_CC_Basic::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>&)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> BASIC_GET: node {}", node.NodeId);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 3,
					   { static_cast<uint8_t>(eCommandClass::BASIC),
						 static_cast<uint8_t>(eBasicCommand::BASIC_GET) });
}

void ZW_CC_Basic::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eBasicCommand::BASIC_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< BASIC unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	if (params.empty())
		return;

	const uint8_t value = params[0];
	node.basicValue = value;
	Log.AddL(eLogTypes::INFO, MakeTag(), "<< BASIC_REPORT: node={} value=0x{:02X}",
			 node.NodeId, value);
}

//
// SWITCH MULTILEVEL (0x26)
//
void ZW_CC_SwitchMultilevel::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>&)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> SWITCH_MULTILEVEL_GET: node {}", node.NodeId);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 3,
					   { static_cast<uint8_t>(eCommandClass::SWITCH_MULTILEVEL),
						 static_cast<uint8_t>(eSwitchMultilevelCommand::SWITCH_MULTILEVEL_GET) });
}

void ZW_CC_SwitchMultilevel::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eSwitchMultilevelCommand::SWITCH_MULTILEVEL_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SWITCH_MULTILEVEL unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}

	if (params.empty())
		return;

	const uint8_t level = params[0];
	node.switchMultilevelValue = level;
	Log.AddL(eLogTypes::INFO, MakeTag(), "<< SWITCH_MULTILEVEL_REPORT: node={} level={}",
			 node.NodeId, level);
}

//
// SENSOR BINARY (0x30)
//
void ZW_CC_SensorBinary::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>&)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> SENSOR_BINARY_GET: node {}", node.NodeId);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 3,
					   { static_cast<uint8_t>(eCommandClass::SENSOR_BINARY),
						 static_cast<uint8_t>(eSensorBinaryCommand::SENSOR_BINARY_GET) });
}

void ZW_CC_SensorBinary::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eSensorBinaryCommand::SENSOR_BINARY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SENSOR_BINARY unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	if (params.empty())
		return;

	const uint8_t value = params[0];
	node.sensorBinaryValue = value;
	Log.AddL(eLogTypes::INFO, MakeTag(), "<< SENSOR_BINARY_REPORT: node={} value={}",
			 node.NodeId, value);
}

//
// METER (0x32 / 0x50)
//
void ZW_CC_Meter::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>&)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> METER_GET: node {}", node.NodeId);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 3,
					   { static_cast<uint8_t>(eCommandClass::METER),
						 static_cast<uint8_t>(eMeterCommand::METER_GET) });
}

void ZW_CC_Meter::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eMeterCommand::METER_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< METER unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	if (params.size() < 2)
		return;

	const uint8_t meterType = params[0];
	const uint8_t value = params[1];

	node.meterInfo.hasValue = true;
	node.meterInfo.meterType = meterType;
	node.meterInfo.value = value;

	Log.AddL(eLogTypes::INFO, MakeTag(), "<< METER_REPORT: node={} type={} value={}",
			 node.NodeId, meterType, value);
}

//
// MULTI CHANNEL (0x60)
//
void ZW_CC_MultiChannel::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>&)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> MULTI_CHANNEL_CAPABILITY_GET: node {}", node.NodeId);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 4,
					   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL),
						 static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_GET),
						 1 }); // endpoint 1 for now
}

void ZW_CC_MultiChannel::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< MULTI_CHANNEL unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	Log.AddL(eLogTypes::INFO, MakeTag(), "<< MULTI_CHANNEL_REPORT: node={} paramsLen={}",
			 node.NodeId, params.size());

	node.multiChannelInfo.hasLastReport = true;
	node.multiChannelInfo.raw = params;
}

//
// CONFIGURATION (0x70)
//
void ZW_CC_Configuration::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> CONFIGURATION_GET: node {}", node.NodeId);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 4,
					   { static_cast<uint8_t>(eCommandClass::CONFIGURATION),
						 static_cast<uint8_t>(eConfigurationCommand::CONFIGURATION_GET),
						 params[0] }); // parameter number
}

void ZW_CC_Configuration::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eConfigurationCommand::CONFIGURATION_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< CONFIGURATION_REPORT: node={} cmd=0x{:02X} len={}",
				 node.NodeId, cmdId, params.size());
		return;
	}

	Log.AddL(eLogTypes::INFO, MakeTag(), "<< CONFIGURATION_REPORT: node={} paramsLen={}",
			 node.NodeId, params.size());

	node.configurationInfo.hasLastReport = true;
	node.configurationInfo.raw = params;
	if (!params.empty())
		node.configurationInfo.paramNumber = params[0];
}

//
// PROTECTION (0x75)
//
void ZW_CC_Protection::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>&)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> PROTECTION_GET: node {}", node.NodeId);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 3,
					   { static_cast<uint8_t>(eCommandClass::PROTECTION),
						 static_cast<uint8_t>(eProtectionCommand::PROTECTION_GET) });
}

void ZW_CC_Protection::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eProtectionCommand::PROTECTION_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< PROTECTION unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}

	if (params.empty())
		return;

	node.protectionState = params[0];

	Log.AddL(eLogTypes::INFO, MakeTag(), "<< PROTECTION_REPORT: node={} state={}",
			 node.NodeId, params[0]);
}

//
// ASSOCIATION (0x85)
//
void ZW_CC_Association::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params)
{
	switch (cmdId)
	{
	case (uint8_t)eAssociationCommand::ASSOCIATION_GET:
		Log.AddL(eLogTypes::INFO, MakeTag(), ">> ASSOCIATION_GET: node {} group {}", node.NodeId, params[0]);
		frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 4,
						   { static_cast<uint8_t>(eCommandClass::ASSOCIATION),
							 static_cast<uint8_t>(eAssociationCommand::ASSOCIATION_GET),
							 params[1] });
		break;
	case (uint8_t)eAssociationCommand::ASSOCIATION_GROUPINGS_GET:
		Log.AddL(eLogTypes::INFO, MakeTag(), ">> ASSOCIATION_GROUPINGS_GET: node {}", node.NodeId);
		frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 3,
						   { static_cast<uint8_t>(eCommandClass::ASSOCIATION),
							 static_cast<uint8_t>(eAssociationCommand::ASSOCIATION_GROUPINGS_GET) });
		break;
	default:
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ASSOCIATION unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		break;
	}
}
void ZW_CC_Association::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	switch (cmdId)
	{
	case (uint8_t)eAssociationCommand::ASSOCIATION_REPORT:

		Log.AddL(eLogTypes::INFO, MakeTag(), "<< ASSOCIATION_REPORT: node={} {}",
				 node.NodeId, ParamsToString(params));

		if (!params.empty())
		{
			const uint8_t group = params[0];
			const uint8_t maxNodesCount = (params.size() > 1) ? params[1] : 0;
			const uint8_t reports = (params.size() > 2) ? params[2] : 0;

			if (group > 0)
			{
				const size_t idx = static_cast<size_t>(group-1);
				if (node.associationInfo.size() <= idx)
					node.associationInfo.resize(idx + 1);

				auto& g = node.associationInfo[group-1];
				g.groupId = group;
				g.nodes.clear();
				g.nodes.reserve(maxNodesCount);
				for (size_t i = 0; i < maxNodesCount; ++i)
				{
					const size_t p = 3 + i;
					if (p >= params.size())
						break;
					g.nodes.push_back(params[p]);
				}
			}
		}
		break;

	case (uint8_t)eAssociationCommand::ASSOCIATION_GROUPINGS_REPORT:

		Log.AddL(eLogTypes::INFO, MakeTag(), "<< ASSOCIATION_GROUPINGS_REPORT: node={} {}",
				 node.NodeId, ParamsToString(params));

		if (!params.empty())
		{
			uint8_t Groupings = params[0];
			node.associationInfo.resize(Groupings);
			for( int i = 0; i < Groupings; i++)
				node.associationInfo[i].groupId = i+1;
		}
		break;
	default:
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ASSOCIATION unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		break;
	}
}

//
// MULTI CHANNEL ASSOCIATION (0x8E)
//
void ZW_CC_MultiChannelAssociation::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params)
{
	Log.AddL(eLogTypes::INFO, MakeTag(), ">> MULTI_CHANNEL_ASSOCIATION_GET: node {} group {}", node.NodeId, params[0]);
	frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 4,
					   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION),
						 static_cast<uint8_t>(eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GET),
						 params[0] });
}

void ZW_CC_MultiChannelAssociation::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REPORT)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< MULTI_CHANNEL_ASSOCIATION unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
	}
	return;

	Log.AddL(eLogTypes::INFO, MakeTag(), "<< MULTI_CHANNEL_ASSOCIATION_REPORT: node={} paramsLen={}",
			 node.NodeId, params.size());

	node.multiChannelAssociationInfo.hasLastReport = true;
	node.multiChannelAssociationInfo.raw = params;
	if (!params.empty())
		node.multiChannelAssociationInfo.groupId = params[0];
}
