#include <vector>

#include "Device.h"
#include "Node.h"
#include "Logging.h"

//
// VERSION
//
void ZW_CC_Version::MakeFrame(ZW_APIFrame& frame, const ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	(void)cmdid;
	frame.MakeSendData(node.NodeId, 1,
					   { static_cast<uint8_t>(eCommandClass::VERSION),
						 static_cast<uint8_t>(eVersionCommand::VERSION_COMMAND_CLASS_GET),
						 params[0] });
}

void ZW_CC_Version::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	if (cmdId != static_cast<uint8_t>(eVersionCommand::VERSION_COMMAND_CLASS_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< VERSION unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	const eCommandClass reportedCc = static_cast<eCommandClass>(params[0]);
	const uint8_t version = params[1];

	Log.AddL(eLogTypes::DVC, MakeTag(), "<< VERSION_COMMAND_CLASS_REPORT: node={} cc=0x{:02X} version={}",
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
void ZW_CC_ManufacturerSpecific::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	auto* mfgCc = node.GetCC(eCommandClass::MANUFACTURER_SPECIFIC);
	uint8_t version = mfgCc ? mfgCc->version : 1;

	if (version >= 1)
		frame.MakeSendData(node.NodeId, 1,
						   { static_cast<uint8_t>(eCommandClass::MANUFACTURER_SPECIFIC),
							   static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET), // 0x04,
						   params[0] }); // DEVICE_SPECIFIC_GET (type 0)
	if (version >= 2)
		frame.MakeSendData(node.NodeId, 1,
						   { static_cast<uint8_t>(eCommandClass::MANUFACTURER_SPECIFIC),
							   static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET_V2), // 0x06,
						   params[0], params[1] }); // DEVICE_SPECIFIC_GET (type 0)
}

void ZW_CC_ManufacturerSpecific::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
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

		Log.AddL(eLogTypes::DVC, MakeTag(), "<< MANUFACTURER_SPECIFIC_REPORT: node={} mfgId=0x{:04X} prodType=0x{:04X} prodId=0x{:04X}", node.NodeId, node.manufacturerInfo.mfgId, node.manufacturerInfo.prodType, node.manufacturerInfo.prodId);
	}
	else if (cmdId == static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT_V2))
	{
		uint8_t deviceIdType = params[0] & 0x07; // Device ID type
		uint8_t dataFormat = (params[1] >> 5) & 0x07; // Data format
		uint8_t dataLength = params[1] & 0x07; // Data length

		node.manufacturerInfo.deviceIdType = deviceIdType;
		node.manufacturerInfo.deviceIdFormat = dataFormat;
		node.manufacturerInfo.hasDeviceId = true;

		Log.AddL(eLogTypes::DVC, MakeTag(), "<< DEVICE_SPECIFIC_REPORT: node={} type=0x{:02X} format=0x{:02X} len={}", node.NodeId, deviceIdType, dataFormat, dataLength);
	}
}

//
// BATTERY
//
void ZW_CC_Battery::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	(void)cmdid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF BATTERY_GET: node {}", node.NodeId);
	frame.MakeSendData(node.NodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::BATTERY),
						 static_cast<uint8_t>(eBatteryCommand::BATTERY_GET) });
}

void ZW_CC_Battery::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	if (cmdId != static_cast<uint8_t>(eBatteryCommand::BATTERY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< BATTERY unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	if (params.empty())
		return;

	const uint8_t level = params[0];
	node.batteryLevel = level;   // from your existing code
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< BATTERY_REPORT: node={} level={}", node.NodeId, level);
}

//
// SWITCH BINARY
//
void ZW_CC_SwitchBinary::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	(void)cmdid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF SWITCH_BINARY_GET: node {}", node.NodeId);
	frame.MakeSendData(node.NodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::SWITCH_BINARY),
						 static_cast<uint8_t>(eSwitchBinaryCommand::SWITCH_BINARY_GET) });
}

void ZW_CC_SwitchBinary::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	if (cmdId != static_cast<uint8_t>(eSwitchBinaryCommand::SWITCH_BINARY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SWITCH_BINARY unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	if (params.empty())
		return;

	const uint8_t value = params[0];
	node.switchBinaryValue = value;
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< SWITCH_BINARY_REPORT: node={} value=0x{:02X}",
			 node.NodeId, value);
}

//
// BASIC (0x20)
//
void ZW_CC_Basic::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	switch (cmdid.value)
	{
	case static_cast<uint8_t>(eBasicCommand::BASIC_SET):
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF BASIC_SET: node {}", node.NodeId);
		frame.MakeSendData(node.NodeId, 3,
						   { static_cast<uint8_t>(eCommandClass::BASIC),
							 static_cast<uint8_t>(eBasicCommand::BASIC_SET), params[0] });
		break;

	case static_cast<uint8_t>(eBasicCommand::BASIC_GET):
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF BASIC_GET: node {}", node.NodeId);
		frame.MakeSendData(node.NodeId, 3,
						   { static_cast<uint8_t>(eCommandClass::BASIC),
							 static_cast<uint8_t>(eBasicCommand::BASIC_GET) });
		break;

	case static_cast<uint8_t>(eBasicCommand::BASIC_REPORT):
		if (params.empty())
		{
			Log.AddL(eLogTypes::DVC, MakeTag(), "MF BASIC_REPORT: node {} no value", node.NodeId);
			return;
		}
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF BASIC_REPORT: node {} value=0x{:02X}", node.NodeId, params[0]);
		frame.MakeSendData(node.NodeId, 3,
						   { static_cast<uint8_t>(eCommandClass::BASIC),
							 static_cast<uint8_t>(eBasicCommand::BASIC_REPORT), params[0] });
		break;

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< BASIC unknowen CC: node={} cmdId={}", node.NodeId, cmdid.value);
		return;
	}
}

void ZW_CC_Basic::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;

	switch (cmdId)
	{
	case static_cast<uint8_t>(eBasicCommand::BASIC_SET):
	{
		if (params.empty())
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "<< BASIC_SET: node={} no value", node.NodeId);
			return;
		}

		const uint8_t value = params[0];
		// unsolicited state change from device
		node.basicValue = value;
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< BASIC_SET: node={} value=0x{:02X} destinationEP={}", node.NodeId, value, destinationEP);
	}
	break;

	case static_cast<uint8_t>(eBasicCommand::BASIC_GET):
		// Device send a request answer
		Log.AddL(eLogTypes::DVC, MakeTag(), "<< BASIC_GET from node={} destinationEP={}", node.NodeId, destinationEP);

		// Send report to device
		{
			uint8_t value = static_cast<uint8_t>(node.basicValue.has_value() ? node.basicValue.value() : 0);

			ZW_APIFrame apiFrame;
			if (destinationEP == 0)
			{	// standalone
				MakeFrame(apiFrame, eBasicCommand::BASIC_REPORT, { value });
				node.SendFrame(apiFrame); // send report
			}
			else
			{ // make a encapsulated report
				MakeFrame(apiFrame, eBasicCommand::BASIC_REPORT, { value });
				ZW_CC_MultiChannel mc(node);
				mc.MakeFrame(apiFrame, ZW_CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP,
							 { destinationEP,
							 static_cast<uint8_t>(eCommandClass::BASIC),
							 static_cast<uint8_t>(eBasicCommand::BASIC_REPORT),
							 value
							 });
				node.SendFrame(apiFrame);

			}
		}

		break;

	case static_cast<uint8_t>(eBasicCommand::BASIC_REPORT):
	{
		if (params.empty())
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "<< BASIC_REPORT: node={} no value", node.NodeId);
			return;
		}
		node.basicValue = params[0];
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< BASIC_REPORT: node={} value=0x{:02X} destinationEP={}", node.NodeId, node.basicValue.value(), destinationEP);
	}
	break;

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< BASIC unknown cmd: node={} cmdId=0x{:02X}", node.NodeId, (int)cmdId);
		break;
	}
}

//
// SWITCH MULTILEVEL (0x26)
//
void ZW_CC_SwitchMultilevel::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	(void)cmdid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF SWITCH_MULTILEVEL_GET: node {}", node.NodeId);
	frame.MakeSendData(node.NodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::SWITCH_MULTILEVEL),
						 static_cast<uint8_t>(eSwitchMultilevelCommand::SWITCH_MULTILEVEL_GET) });
}

void ZW_CC_SwitchMultilevel::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	if (cmdId != static_cast<uint8_t>(eSwitchMultilevelCommand::SWITCH_MULTILEVEL_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SWITCH_MULTILEVEL unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}

	if (params.empty())
		return;

	const uint8_t level = params[0];
	node.switchMultilevelValue = level;
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< SWITCH_MULTILEVEL_REPORT: node={} level={}",
			 node.NodeId, level);
}

//
// SENSOR BINARY (0x30)
//
void ZW_CC_SensorBinary::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	(void)cmdid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF SENSOR_BINARY_GET: node {}", node.NodeId);
	frame.MakeSendData(node.NodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::SENSOR_BINARY),
						 static_cast<uint8_t>(eSensorBinaryCommand::SENSOR_BINARY_GET) });
}

void ZW_CC_SensorBinary::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	if (cmdId != static_cast<uint8_t>(eSensorBinaryCommand::SENSOR_BINARY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SENSOR_BINARY unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}
	if (params.empty())
		return;

	const uint8_t value = params[0];
	node.sensorBinaryValue = value;
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< SENSOR_BINARY_REPORT: node={} value={}",
			 node.NodeId, value);
}

//
// METER (0x32 / 0x50)
//
void ZW_CC_Meter::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	(void)cmdid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF METER_GET: node {}", node.NodeId);
	frame.MakeSendData(node.NodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::METER),
						 static_cast<uint8_t>(eMeterCommand::METER_GET) });
}

void ZW_CC_Meter::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
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

	Log.AddL(eLogTypes::DVC, MakeTag(), "<< METER_REPORT: node={} type={} value={}",
			 node.NodeId, meterType, value);
}

//
// MULTI CHANNEL (0x60)
//
void ZW_CC_MultiChannel::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	switch (cmdId)
	{
	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP):
	{
		std::string oss;
		for (auto& param : params) oss += std::format("{:02X} ", (param));
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF MULTI_CHANNEL_CMD_ENCAP: node {} params {}", node.NodeId, oss.c_str());

		ZW_ByteVector payload;
		payload.push_back(static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL));
		payload.push_back(static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP));
		payload.insert(payload.end(), params.begin(), params.end()); // DE | CC | CMD | PAYLOAD...
		frame.MakeSendData(node.NodeId, 4, payload);
	}
	break;

	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_END_POINT_GET):
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF MULTI_CHANNEL_END_POINT_GET: node {}", node.NodeId);
		frame.MakeSendData(node.NodeId, 4,
						   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL),
							 static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_END_POINT_GET)
						   });
		break;

	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_GET):
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF MULTI_CHANNEL_CAPABILITY_GET: node {}", node.NodeId);
		frame.MakeSendData(node.NodeId, 4,
						   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL),
							 static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_GET),
							 params[0] });
		break;
	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 ">> MULTI_CHANNEL unknown CC: node={} CC={}",
				 node.NodeId, cmdId);
		break;
	}
}

void ZW_CC_MultiChannel::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;

	switch (cmdId)
	{
	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP):
	{
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< MULTI_CHANNEL_CMD_ENCAP: node={} params={}",
				 node.NodeId, ParamsToString(params));

		// | 0x60 | 0x06 | DE | CC | CMD | PAYLOAD...
		uint8_t destinationEndpoint = params[0];
		eCommandClass commandClass = static_cast<eCommandClass>(params[1]);
		uint8_t command = params[2];
		ZW_ByteVector payload(params.begin() + 3, params.end());

		node.HandleCCDeviceReport(commandClass, command, payload);
	}
	break;
	//
	// MULTI_CHANNEL_END_POINT_REPORT (0x08)
	//
	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_END_POINT_REPORT):
	{
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< MULTI_CHANNEL_END_POINT_REPORT: node={} params={}",
				 node.NodeId, ParamsToString(params));

		if (params.size() < 2)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "Invalid END_POINT_REPORT length");
			return;
		}

		// params[0] = aggregated bitmask (ignored for now)
		// params[1] = number of endpoints
		uint8_t endpointCount = params[1];

		node.multiChannel.endpointCount = endpointCount;
		node.multiChannel.hasEndpointReport = true;

		// Prepare endpoint vector
		//node.multiChannel.endpoints.clear();
		node.multiChannel.endpoints.resize(endpointCount);

		for (uint8_t ep = 1; ep <= endpointCount; ep++)
			node.multiChannel.endpoints[ep - 1].endpointId = ep;

		break;
	}

	//
	// MULTI_CHANNEL_CAPABILITY_REPORT (0x0A)
	//
	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_REPORT):
	{
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< MULTI_CHANNEL_CAPABILITY_REPORT: node={} params={}",
				 node.NodeId, ParamsToString(params));

		if (params.size() < 3)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "Invalid CAPABILITY_REPORT length");
			return;
		}

		uint8_t ep = params[0];
		if (ep == 0 || ep > node.multiChannel.endpointCount)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "Invalid endpoint {} in CAPABILITY_REPORT", ep);
			return;
		}

		ZW_Node::EndpointInfo& DVC = node.multiChannel.endpoints[ep - 1];

		DVC.endpointId = ep;
		DVC.generic = params[1];
		DVC.specific = params[2];

		// Remaining bytes are CC list
		DVC.supportedCCs.clear();
		for (size_t i = 3; i < params.size(); i++)
			DVC.supportedCCs.push_back(params[i]);

		DVC.hasCapabilityReport = true;
		break;
	}

	default:
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "<< MULTI_CHANNEL unknown CC: node={} cmdId={}",
					 node.NodeId, cmdId);
			break;
		}
	}
}

//
// CONFIGURATION (0x70)
//
void ZW_CC_Configuration::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	const uint8_t cc = static_cast<uint8_t>(eCommandClass::CONFIGURATION);
	const uint8_t cmd = cmdid.value;

	switch (cmd)
	{
	case (uint8_t)eConfigurationCommand::CONFIGURATION_GET:
		{
			// params[0] = parameter number
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 ">> CONFIGURATION_GET: node {} param={}",
					 node.NodeId, params[0]);

			frame.MakeSendData(
				node.NodeId,
				3, // length of payload
				{ cc,
				  static_cast<uint8_t>(eConfigurationCommand::CONFIGURATION_GET),
				  params[0] } // parameter number
			);
			break;
		}

	case (uint8_t)eConfigurationCommand::CONFIGURATION_SET:
		{
			// params = [paramNumber, size, valueBytes...]
			if (params.size() < 2)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "CONFIGURATION_SET missing size/value: node {}", node.NodeId);
				return;
			}

			uint8_t paramNumber = params[0];
			uint8_t size = params[1];

			if (params.size() < 2 + size)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "CONFIGURATION_SET wrong value size: node {} param={} size={}",
						 node.NodeId, paramNumber, size);
				return;
			}

			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF CONFIGURATION_SET: node {} param={} size={}",
					 node.NodeId, paramNumber, size);

			ZW_ByteVector payload;
			payload.reserve(3 + size);

			payload.push_back(cc);
			payload.push_back(static_cast<uint8_t>(eConfigurationCommand::CONFIGURATION_SET));
			payload.push_back(paramNumber);
			payload.push_back(size);

			for (uint8_t i = 0; i < size; i++)
				payload.push_back(params[2 + i]);

			frame.MakeSendData(
				node.NodeId,
				static_cast<uint8_t>(payload.size()),
				payload
			);
			break;
		}

	default:
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "Unknown CONFIGURATION command: node {} cmd={}",
					 node.NodeId, cmd);
			break;
		}
	}
}

void ZW_CC_Configuration::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	if (cmdid.value != 0x06) // CONFIGURATION_REPORT
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< CONFIGURATION unknown CC: node={} cmdId={}", node.NodeId, cmdid.value);
		return;
	}

	if (params.size() < 3)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< CONFIGURATION_REPORT: invalid length {}", params.size());
		return;
	}

	uint8_t paramNumber = params[0];
	uint8_t size = params[1];

	int32_t value = 0;

	// Parse signed big-endian value
	for (uint8_t i = 0; i < size && 2 + i < params.size(); i++)
		value = (value << 8) | params[2 + i];

	// Sign extend
	int shift = (4 - size) * 8;
	value = (value << shift) >> shift;

	if (paramNumber >= 0 && paramNumber < node.configurationInfo.size())
	{
		auto& cfg = node.configurationInfo[paramNumber];
		cfg.paramNumber = paramNumber;
		cfg.size = size;
		cfg.value = value;
		cfg.valid = true;
	}
	else
	{
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 "<< CONFIGURATION_REPORT: node={} invalid param={} (len={})",
				 node.NodeId, paramNumber, (int)node.configurationInfo.size());
		return;
	}

	if (paramNumber % 10 == 0)
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< CONFIGURATION_REPORT: node={} param={} size={} value={}",
				 node.NodeId, paramNumber, size, value);
}

//
// PROTECTION (0x75)
//
void ZW_CC_Protection::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	(void)cmdid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF PROTECTION_GET: node {}", node.NodeId);
	frame.MakeSendData(node.NodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::PROTECTION),
						 static_cast<uint8_t>(eProtectionCommand::PROTECTION_GET) });
}

void ZW_CC_Protection::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	if (cmdId != static_cast<uint8_t>(eProtectionCommand::PROTECTION_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< PROTECTION unknowen CC: node={} cmdId={}", node.NodeId, cmdId);
		return;
	}

	if (params.empty())
		return;

	node.protectionState = params[0];

	Log.AddL(eLogTypes::DVC, MakeTag(), "<< PROTECTION_REPORT: node={} state={}",
			 node.NodeId, params[0]);
}

//
// ASSOCIATION (0x85)
//
void ZW_CC_Association::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	std::vector<uint8_t> payload;
	payload.reserve(2 + params.size());
	payload.push_back(static_cast<uint8_t>(eCommandClass::ASSOCIATION));
	payload.push_back(cmdId);
	payload.insert(payload.end(), params.begin(), params.end());

	uint8_t cb = 5; // node.GetNextCallbackId();

	switch (cmdId)
	{
	case (uint8_t)eAssociationCommand::ASSOCIATION_SET:
		{
			std::string oss;
			for (size_t i = 1; i < params.size(); i++)
				oss += std::to_string(params[i]) + " ";

			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF ASSOCIATION_SET: node {} group {} = {}",
					 node.NodeId, params[0], oss);

			frame.MakeSendData(node.NodeId, cb, payload);
			break;
		}

	case (uint8_t)eAssociationCommand::ASSOCIATION_REMOVE:
		{
			std::string oss;
			for (size_t i = 1; i < params.size(); i++)
				oss += std::to_string(params[i]) + " ";

			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF ASSOCIATION_REMOVE: node {} group {} - {}",
					 node.NodeId, params[0], oss);

			frame.MakeSendData(node.NodeId, cb, payload);
			break;
		}

	case (uint8_t)eAssociationCommand::ASSOCIATION_GET:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF ASSOCIATION_GET: node {} group {}",
					 node.NodeId, params[0]);

			frame.MakeSendData(node.NodeId, cb, payload);
			break;
		}

	case (uint8_t)eAssociationCommand::ASSOCIATION_GROUPINGS_GET:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF ASSOCIATION_GROUPINGS_GET: node {}",
					 node.NodeId);

			frame.MakeSendData(node.NodeId, cb, payload);
			break;
		}

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 ">> ASSOCIATION unknown CC: node={} cmdId={}",
				 node.NodeId, cmdId);
		break;
	}
}

void ZW_CC_Association::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	switch (cmdId)
	{
	case (uint8_t)eAssociationCommand::ASSOCIATION_REPORT:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "<< ASSOCIATION_REPORT: node={} {}",
					 node.NodeId, this->ParamsToString(params));

			if (params.size() < 3)
				return;

			const uint8_t groupId = params[0];
			const uint8_t maxNodes = params[1];
			const uint8_t reportsToFollow = params[2];

			if (groupId == 0)
				return;

			// Ensure vector size
//			if (node.associationGroups.size() < groupId)
	//			node.associationGroups.resize(groupId);
			if (node.multiChannelAssociationGroups.size() < groupId)
				node.multiChannelAssociationGroups.resize(groupId);

			//auto& g = node.associationGroups[groupId - 1];
			auto& g = node.multiChannelAssociationGroups[groupId - 1];

			// If this is the first report for this group, clear old data
			if (!g.hasLastReport)
				//for (auto& m : g.nodeList) m.valid = false;
				for (auto& m : g.members) m.valid = false;

			g.groupId = groupId;
			g.maxNodes = maxNodes;

			// Parse node list
			for (size_t i = 3; i < params.size(); i++)
			{
				uint8_t nodeId = params[i];
//				g.nodeList[nodeId].valid = true;
	//			g.nodeList[nodeId].nodeId = (node_t)nodeId;
				g.members[nodeId].nodeId = (node_t)nodeId;
				g.members[nodeId].endpointId = 0;
				g.members[nodeId].valid = true;
			}

			g.hasLastReport = (reportsToFollow == 0);
			break;
		}

	case (uint8_t)eAssociationCommand::ASSOCIATION_GROUPINGS_REPORT:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "<< ASSOCIATION_GROUPINGS_REPORT: node={} {}",
					 node.NodeId, this->ParamsToString(params));

			if (params.empty())
				return;

			uint8_t groupCount = params[0];
			node.multiChannelAssociationGroups.clear();
			node.multiChannelAssociationGroups.resize(groupCount);
//			node.associationGroups.clear();
	//		node.associationGroups.resize(groupCount);

			for (uint8_t g = 0; g < groupCount; g++)
			{
				//	node.associationGroups[i].groupId = g + 1;
				node.multiChannelAssociationGroups[g].groupId = g + 1;
				node.multiChannelAssociationGroups[g].hasLastReport = false;
			}

			break;
		}

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 "<< ASSOCIATION unknown CC: node={} cmdId={}",
				 node.NodeId, cmdId);
		break;
	}
}

//
// MULTI CHANNEL ASSOCIATION (0x8E)
//
void ZW_CC_MultiChannelAssociation::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	uint8_t cb = 6; // node.GetNextCallbackId();

	switch (cmdId)
	{
	case (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET: node {}",
					 node.NodeId);

			frame.MakeSendData(node.NodeId, cb,
							   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION),cmdId });
			break;
		}

	case (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GET:
		{
			if (params.size() < 1)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "MULTI_CHANNEL_ASSOCIATION_GET missing group: node {}", node.NodeId);
				return;
			}
			auto group = params[0];
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF MULTI_CHANNEL_ASSOCIATION_GET: node {} group {}",
					 node.NodeId, group);

			frame.MakeSendData(node.NodeId, cb,
							   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION),cmdId, (uint8_t)group });
			break;
		}

	case (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REMOVE:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF MULTI_CHANNEL_ASSOCIATION_REMOVE: node {} group {} params={}",
					 node.NodeId, params[0], ParamsToString(params));

			ZW_ByteVector payLoad;
			payLoad.push_back(static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION));
			payLoad.push_back(cmdId);
			payLoad.insert(payLoad.end(), params.begin(), params.end());

			frame.MakeSendData(node.NodeId, cb, payLoad);
			break;
		}

	case (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_SET:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF MULTI_CHANNEL_ASSOCIATION_SET: node {} group {} params={}",
					 node.NodeId, params[0], ParamsToString(params));

			ZW_ByteVector payLoad;
			payLoad.push_back(static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION));
			payLoad.push_back(cmdId);
			payLoad.insert(payLoad.end(), params.begin(), params.end());

			frame.MakeSendData(node.NodeId, cb, payLoad);
			break;
		}

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 "<< MULTI_CHANNEL_ASSOCIATION unknown CC: node={} cmdId={}",
				 node.NodeId, cmdId);
		break;
	}
}

void ZW_CC_MultiChannelAssociation::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;

	//
	// --- MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT (0x06) ---
	//
	if (cmdId == (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT)
	{
		if (params.size() < 1)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "<< MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT too short: node={}",
					 node.NodeId);
			return;
		}

		const uint8_t supportedGroups = params[0];

		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT: node={} supportedGroups={}",
				 node.NodeId, supportedGroups);

		// Resize group vector
		node.multiChannelAssociationGroups.clear();
		node.multiChannelAssociationGroups.resize(supportedGroups);

		// Initialize group IDs
		for (uint8_t g = 0; g < supportedGroups; g++)
		{
			node.multiChannelAssociationGroups[g].groupId = g + 1;
			node.multiChannelAssociationGroups[g].hasLastReport = false;
		}

		return;
	}

	//
	// --- MULTI_CHANNEL_ASSOCIATION_REPORT (0x03) ---
	//
	if (cmdId == (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REPORT)
	{
		if (params.size() < 3)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "<< MULTI_CHANNEL_ASSOCIATION_REPORT too short: node={}",
					 node.NodeId);
			return;
		}

		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< MULTI_CHANNEL_ASSOCIATION_REPORT: node={} {}",
				 node.NodeId, ParamsToString(params));

		const uint8_t groupId = params[0];
		const uint8_t maxNodes = params[1];
		const uint8_t reportsToFollow = params[2];

		// Ensure vector size
		if (node.multiChannelAssociationGroups.size() < groupId)
			node.multiChannelAssociationGroups.resize(groupId);

		auto& g = node.multiChannelAssociationGroups[groupId - 1];
		g.groupId = groupId;
		g.maxNodes = maxNodes; // Number of node ID entries

		// If this is the first report for this group, clear old data
		if (!g.hasLastReport)
			for (auto& m : g.members) m.valid = false;

		size_t i = 3;

		//
		// 1) Parse NodeID entries
		//
		for (uint8_t n = 0; n < maxNodes && i < params.size(); n++)
		{
			uint8_t nodeId = params[i++];
			if (nodeId == (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REPORT_MARKER)
				break; // start of endpoint entries

			g.members[nodeId].nodeId = (node_t)nodeId;
			g.members[nodeId].endpointId = 0;
			g.members[nodeId].valid = true;
		}

		//
		// 2) Parse Multi Channel destinations
		//
		while (i + 1 < params.size())
		{
			uint8_t nodeId = params[i++];
			uint8_t endpointId = params[i++];

			g.members[nodeId].nodeId = (node_t)nodeId;
			g.members[nodeId].endpointId = endpointId;
			g.members[nodeId].valid = true;
		}

		//
		// 3) Track last report
		//
		g.hasLastReport = (reportsToFollow == 0);
	}
}

//
// WAKE UP
//
void ZW_CC_WakeUp::MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	auto* cc = node.GetCC(eCommandClass::WAKE_UP);
	uint8_t version = cc ? cc->version : 1;

	switch (cmdId)
	{
	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_INTERVAL_GET):
		Log.AddL(eLogTypes::DVC, MakeTag(),
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

		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "MF WAKE_UP_INTERVAL_SET (v{}): node {} interval={}s",
				 version, node.NodeId,
				 (params[0] << 16) | (params[1] << 8) | params[2]);

		frame.MakeSendData(node.NodeId, 5,
						   { uint8_t(eCommandClass::WAKE_UP),
							 uint8_t(eWakeUpCommand::WAKE_UP_INTERVAL_SET),
							 params[0], params[1], params[2] });
		break;

	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_NO_MORE_INFORMATION):
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "MF WAKE_UP_NO_MORE_INFORMATION (v{}): node {}", version, node.NodeId);

		frame.MakeSendData(node.NodeId, 2,
						   { uint8_t(eCommandClass::WAKE_UP),
							 uint8_t(eWakeUpCommand::WAKE_UP_NO_MORE_INFORMATION) });
		break;

	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_INTERVAL_CAPABILITIES_GET):
		if (version >= 3)
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF WAKE_UP_INTERVAL_CAPABILITIES_GET (v{}): node {}", version, node.NodeId);

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
void ZW_CC_WakeUp::HandleReport(const ZW_CmdId cmdid, const uint8_t destinationEP, const ZW_ByteVector& params)
{
	const uint8_t cmdId = cmdid.value;
	auto* cc = node.GetCC(eCommandClass::WAKE_UP);
	uint8_t version = cc ? cc->version : 1;

	ZW_NodeInfo::WakeUpInfo wup = node.GetWakeUpInfo();
	switch (cmdId)
	{
	case uint8_t(eWakeUpCommand::WAKE_UP_NOTIFICATION):
		Log.AddL(eLogTypes::DVC, MakeTag(),
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

			uint32_t interval = (static_cast<uint32_t>(params[0]) << 16) | (static_cast<uint32_t>(params[1]) << 8) | static_cast<uint32_t>(params[2]);
			wup.wakeUpInterval = interval;
			wup.hasLastReport = true;
			node.SetWakeUpInfo(wup);

			Log.AddL(eLogTypes::DVC, MakeTag(),
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

			uint32_t interval = (static_cast<uint32_t>(params[0]) << 16) | (static_cast<uint32_t>(params[1]) << 8) | static_cast<uint32_t>(params[2]);
			uint32_t minInt = (static_cast<uint32_t>(params[3]) << 16) | (static_cast<uint32_t>(params[4]) << 8) | static_cast<uint32_t>(params[5]);
			uint32_t maxInt = (static_cast<uint32_t>(params[6]) << 16) | (static_cast<uint32_t>(params[7]) << 8) | static_cast<uint32_t>(params[8]);

			wup.wakeUpInterval = interval;
			wup.wakeUpMin = minInt;
			wup.wakeUpMax = maxInt;
			wup.hasLastReport = true;
			node.SetWakeUpInfo(wup);

			Log.AddL(eLogTypes::DVC, MakeTag(),
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

			uint32_t minInt = (static_cast<uint32_t>(params[0]) << 16) | (static_cast<uint32_t>(params[1]) << 8) | static_cast<uint32_t>(params[2]);
			uint32_t maxInt = (static_cast<uint32_t>(params[3]) << 16) | (static_cast<uint32_t>(params[4]) << 8) | static_cast<uint32_t>(params[5]);
			uint32_t defInt = (static_cast<uint32_t>(params[6]) << 16) | (static_cast<uint32_t>(params[7]) << 8) | static_cast<uint32_t>(params[8]);

			wup.wakeUpMin = minInt;
			wup.wakeUpMax = maxInt;
			wup.wakeUpDefault = defInt;
			wup.hasLastReport = true;
			node.SetWakeUpInfo(wup);

			Log.AddL(eLogTypes::DVC, MakeTag(),
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
