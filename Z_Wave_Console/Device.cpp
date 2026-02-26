#include <vector>

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
	const eCommandClass reportedCc = static_cast<eCommandClass>(params[0]);
	const uint8_t version = params[1];

	Log.AddL(eLogTypes::INFO, MakeTag(), "<< VERSION_COMMAND_CLASS_REPORT: node={} cc=0x{:02X} version={}",
			 node.NodeId, (uint8_t)reportedCc, version);

	if (auto* cc = node.GetCC(reportedCc))
	{
		cc->supported = true;
		cc->version = version;
		cc->versionOk = true;
	}
}

//
// MANUFACTURER SPECIFIC
//
void ZW_CC_ManufacturerSpecific::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params)
{
	auto* mfgCc = node.GetCC(eCommandClass::MANUFACTURER_SPECIFIC);
	uint8_t version = mfgCc ? mfgCc->version : 1;

	if (version >= 1)
		frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 1,
						   { static_cast<uint8_t>(eCommandClass::MANUFACTURER_SPECIFIC),
							   static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET), // 0x04,
						   params[0] }); // DEVICE_SPECIFIC_GET (type 0)
	if (version >= 2)
		frame.MakeSendData(static_cast<uint8_t>(node.NodeId), 1,
						   { static_cast<uint8_t>(eCommandClass::MANUFACTURER_SPECIFIC),
							   static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET_V2), // 0x06,
						   params[0], params[1] }); // DEVICE_SPECIFIC_GET (type 0)
}

void ZW_CC_ManufacturerSpecific::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	if (cmdId != static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< DEVICE_SPECIFIC_REPORT: node={} cmd=0x{:02X} len={} params={}", node.NodeId, cmdId, params.size(), params[0]);
		return;
	}

	if (cmdId == static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT))
	{
		node.manufacturerInfo.mfgId = (static_cast<uint16_t>(params[0]) << 8) | params[1];
		node.manufacturerInfo.prodType = (static_cast<uint16_t>(params[2]) << 8) | params[3];
		node.manufacturerInfo.prodId = (static_cast<uint16_t>(params[4]) << 8) | params[5];
		node.manufacturerInfo.hasManufacturerData = true;

		Log.AddL(eLogTypes::INFO, MakeTag(), "<< MANUFACTURER_SPECIFIC_REPORT: node={} mfgId=0x{:04X} prodType=0x{:04X} prodId=0x{:04X}", node.NodeId, node.manufacturerInfo.mfgId, node.manufacturerInfo.prodType, node.manufacturerInfo.prodId);
	}
	else if (cmdId == static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT_V2))
	{
		uint8_t deviceIdType = params[0] & 0x07; // Device ID type
		uint8_t dataFormat = (params[1] >> 5) & 0x07; // Data format
		uint8_t dataLength = params[1] & 0x07; // Data length

		node.manufacturerInfo.deviceIdType = deviceIdType;
		node.manufacturerInfo.deviceIdFormat = dataFormat;
		node.manufacturerInfo.hasDeviceId = true;

		Log.AddL(eLogTypes::INFO, MakeTag(), "<< DEVICE_SPECIFIC_REPORT: node={} type=0x{:02X} format=0x{:02X} len={}", node.NodeId, deviceIdType, dataFormat, dataLength);
	}
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
							 params[0] });
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
				const size_t idx = static_cast<size_t>(group - 1);
				if (node.associationInfo.size() <= idx)
					node.associationInfo.resize(idx + 1);

				auto& g = node.associationInfo[group - 1];
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
			for (int i = 0; i < Groupings; i++)
				node.associationInfo[i].groupId = i + 1;
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

//
// WAKE UP
//
	//
	// MAKE FRAME
	//
void ZW_CC_WakeUp::MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params)
{
	auto* cc = node.GetCC(eCommandClass::WAKE_UP);
	uint8_t version = cc ? cc->version : 1;

	switch (cmdId)
	{
	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_INTERVAL_GET):
		Log.AddL(eLogTypes::INFO, MakeTag(),
				 ">> WAKE_UP_INTERVAL_GET (v{}): node {}", version, node.NodeId);

		frame.MakeSendData(node.NodeId, 2,
						   { uint8_t(eCommandClass::WAKE_UP),
							 uint8_t(eWakeUpCommand::WAKE_UP_INTERVAL_GET) });
		break;

	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_INTERVAL_SET):
		if (params.size() < 3)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "WAKE_UP_INTERVAL_SET missing params (v{})", version);
			return;
		}

		Log.AddL(eLogTypes::INFO, MakeTag(),
				 ">> WAKE_UP_INTERVAL_SET (v{}): node {} interval={}s",
				 version, node.NodeId,
				 (params[0] << 16) | (params[1] << 8) | params[2]);

		frame.MakeSendData(node.NodeId, 5,
						   { uint8_t(eCommandClass::WAKE_UP),
							 uint8_t(eWakeUpCommand::WAKE_UP_INTERVAL_SET),
							 params[0], params[1], params[2] });
		break;

	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_NO_MORE_INFORMATION):
		Log.AddL(eLogTypes::INFO, MakeTag(),
				 ">> WAKE_UP_NO_MORE_INFORMATION (v{}): node {}", version, node.NodeId);

		frame.MakeSendData(node.NodeId, 2,
						   { uint8_t(eCommandClass::WAKE_UP),
							 uint8_t(eWakeUpCommand::WAKE_UP_NO_MORE_INFORMATION) });
		break;

	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_INTERVAL_CAPABILITIES_GET):
		if (version >= 3)
		{
			Log.AddL(eLogTypes::INFO, MakeTag(),
					 ">> WAKE_UP_INTERVAL_CAPABILITIES_GET (v{}): node {}", version, node.NodeId);

			frame.MakeSendData(node.NodeId, 2,
							   { uint8_t(eCommandClass::WAKE_UP),
								 uint8_t(eWakeUpCommand::WAKE_UP_INTERVAL_CAPABILITIES_GET) });
		}
		break;

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 "Unknown WAKE_UP cmdId={} (v{}) for node {}", cmdId, version, node.NodeId);
		break;
	}
}

//
// HANDLE REPORT
//
void ZW_CC_WakeUp::HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params)
{
	auto* cc = node.GetCC(eCommandClass::WAKE_UP);
	uint8_t version = cc ? cc->version : 1;

	ZW_NodeInfo::WakeUpInfo wup = node.GetWakeUpInfo();
	switch (cmdId)
	{
	case uint8_t(eWakeUpCommand::WAKE_UP_NOTIFICATION):
		Log.AddL(eLogTypes::INFO, MakeTag(),
				 "<< WAKE_UP_NOTIFICATION (v{}): node {} is awake", version, node.NodeId);

		node.WakeUp();
		wup.lastWakeUp = std::chrono::system_clock::now();
		break;

	case uint8_t(eWakeUpCommand::WAKE_UP_INTERVAL_REPORT):
		if (version == 1)
		{
			if (params.size() < 3)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "<< WAKE_UP_INTERVAL_REPORT invalid v1 params: node {}", node.NodeId);
				return;
			}

			uint32_t interval = (params[0] << 16) | (params[1] << 8) | params[2];
			wup.wakeUpInterval = interval;
			wup.hasLastReport = true;
			node.SetWakeUpInfo(wup);

			Log.AddL(eLogTypes::INFO, MakeTag(),
					 "<< WAKE_UP_INTERVAL_REPORT v1: node {} interval={}s",
					 node.NodeId, interval);
		}
		else if (version >= 2)
		{
			if (params.size() < 9)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "<< WAKE_UP_INTERVAL_REPORT invalid v2 params: node {}", node.NodeId);
				return;
			}

			uint32_t interval = (params[0] << 16) | (params[1] << 8) | params[2];
			uint32_t minInt = (params[3] << 16) | (params[4] << 8) | params[5];
			uint32_t maxInt = (params[6] << 16) | (params[7] << 8) | params[8];

			wup.wakeUpInterval = interval;
			wup.wakeUpMin = minInt;
			wup.wakeUpMax = maxInt;
			wup.hasLastReport = true;
			node.SetWakeUpInfo(wup);

			Log.AddL(eLogTypes::INFO, MakeTag(),
					 "<< WAKE_UP_INTERVAL_REPORT v{}: node {} interval={}s min={}s max={}s",
					 version, node.NodeId, interval, minInt, maxInt);
		}
		break;

	case uint8_t(eWakeUpCommand::WAKE_UP_INTERVAL_CAPABILITIES_REPORT):
		if (version >= 3)
		{
			if (params.size() < 9)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "<< WAKE_UP_INTERVAL_CAPABILITIES_REPORT invalid params: node {}", node.NodeId);
				return;
			}

			uint32_t minInt = (params[0] << 16) | (params[1] << 8) | params[2];
			uint32_t maxInt = (params[3] << 16) | (params[4] << 8) | params[5];
			uint32_t defInt = (params[6] << 16) | (params[7] << 8) | params[8];

			wup.wakeUpMin = minInt;
			wup.wakeUpMax = maxInt;
			wup.wakeUpDefault = defInt;
			wup.hasLastReport = true;
			node.SetWakeUpInfo(wup);

			Log.AddL(eLogTypes::INFO, MakeTag(),
					 "<< WAKE_UP_INTERVAL_CAPABILITIES_REPORT v{}: node {} min={}s max={}s default={}s",
					 version, node.NodeId, minInt, maxInt, defInt);
		}
		break;

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 "<< WAKE_UP unknown CC (v{}): node={} cmdId={}",
				 version, node.NodeId, cmdId);
		break;
	}
};
