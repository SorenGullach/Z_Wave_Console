#include <vector>
#include <cstdint>  // Add this for uint8_t

#include "NodeCommandClasses.h"
#include "Logging.h"
#include "NodeInfo.h"

int CCHandler::Version(eCommandClass cc) { return node.GetCC(cc)->version; };

//
// VERSION
//
void CC_Version::MakeFrame(APIFrame& frame, const ccid_t ccid, const ccparams_t& params)
{
	switch (static_cast<eVersionCommand>(ccid.Value()))
	{
	case eVersionCommand::VERSION_COMMAND_CLASS_GET:
		if (params.size() != 1)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "MF VERSION_COMMAND_CLASS_GET: node={} invalid param count={}",
					 node.nodeId, params.size());
			return;
		}

		frame.MakeSendData(node.nodeId, 1,
						   { static_cast<uint8_t>(CC),
							 ccid,
							 params[0] });
		return;

	case eVersionCommand::VERSION_GET:
	case eVersionCommand::VERSION_REPORT:
	case eVersionCommand::VERSION_CAPABILITIES_GET:
	case eVersionCommand::VERSION_CAPABILITIES_REPORT:
	case eVersionCommand::VERSION_ZWAVE_SOFTWARE_GET:
	case eVersionCommand::VERSION_ZWAVE_SOFTWARE_REPORT:
	case eVersionCommand::VERSION_COMMAND_CLASS_REPORT:
		break;
	}

	Log.AddL(eLogTypes::ERR, MakeTag(),
			 "MF VERSION unknown command: node={} ccid=0x{:02X}",
			 node.nodeId, ccid);
}

void CC_Version::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t)
{
	switch (static_cast<eVersionCommand>(ccid.Value()))
	{
	case eVersionCommand::VERSION_GET:
	case eVersionCommand::VERSION_REPORT:
	case eVersionCommand::VERSION_CAPABILITIES_GET:
	case eVersionCommand::VERSION_CAPABILITIES_REPORT:
	case eVersionCommand::VERSION_ZWAVE_SOFTWARE_GET:
	case eVersionCommand::VERSION_ZWAVE_SOFTWARE_REPORT:
	case eVersionCommand::VERSION_COMMAND_CLASS_GET:
		break;

	case eVersionCommand::VERSION_COMMAND_CLASS_REPORT:
		{
			if (params.size() < 2)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "<< VERSION_COMMAND_CLASS_REPORT: invalid payload size={} node={}",
						 params.size(), node.nodeId);
				return;
			}

			struct PayLoad
			{
				eCommandClass reportedCc;
				uint8_t version;
			};

			static_assert(sizeof(PayLoad) == 2, "VERSION_COMMAND_CLASS_REPORT payload mismatch");

			const auto* p = reinterpret_cast<const PayLoad*>(params.data());

			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "<< VERSION_COMMAND_CLASS_REPORT: node={} cc=0x{:02X} {} version={}",
					 node.nodeId, static_cast<uint8_t>(p->reportedCc),
					 CommandClassToString(p->reportedCc), p->version);

			if (auto* cc = node.GetCC(p->reportedCc))
			{
				cc->version = p->version;
				cc->versionOk = true;
			}
		}
		return;
	}

	Log.AddL(eLogTypes::ERR, MakeTag(),
			 "<< VERSION unknown CC: node={} ccid={}",
			 node.nodeId, ccid);
}

//
// MANUFACTURER SPECIFIC
//
void CC_ManufacturerSpecific::MakeFrame(APIFrame& frame, const ccid_t ccid, const ccparams_t& params)
{
	switch (static_cast<eManufacturerSpecificCommand>(ccid.Value()))
	{
	case eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET:
		if (Version() < 1 || !params.empty())
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "MF DEVICE_SPECIFIC_GET: node={} invalid param count={} for version={}",
					 node.nodeId, params.size(), Version());
			return;
		}

		frame.MakeSendData(node.nodeId, 1,
						   { static_cast<uint8_t>(CC),
							 static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET),
						   });
		return;
	case eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET_V2:
		if (Version() < 2 || params.size() != 1)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "MF DEVICE_SPECIFIC_GET: node={} invalid param count={} for version={}",
					 node.nodeId, params.size(), Version());
			return;
		}

		frame.MakeSendData(node.nodeId, 1,
						   { static_cast<uint8_t>(CC),
							 static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET_V2),
							 params[0] });
		/* param[0]
			Device ID Type								Value
			Return OEM factory default Device ID Type	0
			Serial Number								1
			Pseudo Random								2
			Reserved									3 - 7
		*/
		return;

	case eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT:
	case eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT_V2:
		break;
	}

	Log.AddL(eLogTypes::ERR, MakeTag(),
			 "MF MANUFACTURER_SPECIFIC unknown command: node={} ccid=0x{:02X}",
			 node.nodeId, ccid);
}

void CC_ManufacturerSpecific::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t)
{
	if (ccid != eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT && ccid != eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT_V2)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< DEVICE_SPECIFIC_REPORT: node={} cmd=0x{:02X} len={} params={}", node.nodeId, ccid, params.size(), params[0]);
		return;
	}

	if (ccid == static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT))
	{
		node.manufacturerInfo.mfgId = (static_cast<uint16_t>(params[0]) << 8) | params[1];
		node.manufacturerInfo.prodType = (static_cast<uint16_t>(params[2]) << 8) | params[3];
		node.manufacturerInfo.prodId = (static_cast<uint16_t>(params[4]) << 8) | params[5];
		node.manufacturerInfo.hasManufacturerData = true;
		NotifyUI(UINotify::NodeChanged, node.nodeId);

		Log.AddL(eLogTypes::DVC, MakeTag(), "<< MANUFACTURER_SPECIFIC_REPORT: node={} mfgId=0x{:04X} prodType=0x{:04X} prodId=0x{:04X}", node.nodeId, node.manufacturerInfo.mfgId, node.manufacturerInfo.prodType, node.manufacturerInfo.prodId);
	}
	else if (ccid == eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT_V2)
	{
		uint8_t deviceIdType = params[0] & 0x07; // Device ID type
		uint8_t dataFormat = (params[1] >> 5) & 0x07; // Data format
		uint8_t dataLength = params[1] & 0x07; // Data length

		node.manufacturerInfo.deviceIdType = deviceIdType;
		node.manufacturerInfo.deviceIdFormat = dataFormat;
		node.manufacturerInfo.hasDeviceId = true;
		NotifyUI(UINotify::NodeChanged, node.nodeId);

		Log.AddL(eLogTypes::DVC, MakeTag(), "<< DEVICE_SPECIFIC_REPORT: node={} type=0x{:02X} format=0x{:02X} len={}", node.nodeId, deviceIdType, dataFormat, dataLength);
	}
}

//
// BATTERY
//
void CC_Battery::MakeFrame(APIFrame& frame, const ccid_t ccid, const ccparams_t& params)
{
	switch (static_cast<eBatteryCommand>(ccid.Value()))
	{
	case eBatteryCommand::BATTERY_GET:
		if (!params.empty())
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "MF BATTERY_GET: node={} invalid param count={}", node.nodeId, params.size());
		}
		frame.MakeSendData(node.nodeId, 3,
						   { static_cast<uint8_t>(CC),
							 ccid });
		return;
	case eBatteryCommand::BATTERY_REPORT:
		break;
	}

	Log.AddL(eLogTypes::ERR, MakeTag(), "MF BATTERY unknown command: node={} ccid=0x{:02X}", node.nodeId, ccid);
}

void CC_Battery::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t)
{
	const uint8_t cmdId = ccid.Value();
	if (cmdId != static_cast<uint8_t>(eBatteryCommand::BATTERY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< BATTERY unknowen CC: node={} cmdId={}", node.nodeId, cmdId);
		return;
	}
	if (params.empty())
		return;

	const uint8_t level = params[0];
	node.batteryLevel = level;   // from your existing code
	NotifyUI(UINotify::NodeChanged, node.nodeId);
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< BATTERY_REPORT: node={} level={}", node.nodeId, level);
}

//
// SWITCH BINARY
//
void CC_SwitchBinary::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	switch (static_cast<eSwitchBinaryCommand>(ccid.Value()))
	{
	case eSwitchBinaryCommand::SWITCH_BINARY_GET:
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF SWITCH_BINARY_GET: node {}", node.nodeId);
		frame.MakeSendData(node.nodeId, 3, { static_cast<uint8_t>(CC), ccid });
		return;
	case eSwitchBinaryCommand::SWITCH_BINARY_SET:
		if (params.size() != 1)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "MF SWITCH_BINARY_SET: node={} invalid param count={}", node.nodeId, params.size());
			return;
		}
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF SWITCH_BINARY_SET: node {} {}", node.nodeId, params[0]);
		frame.MakeSendData(node.nodeId, 3, { static_cast<uint8_t>(CC), ccid, params[0] });
		return;

	case eSwitchBinaryCommand::SWITCH_BINARY_REPORT:
		break;
	}

	Log.AddL(eLogTypes::ERR, MakeTag(), "MF SWITCH_BINARY unknown command: node={} ccid=0x{:02X}", node.nodeId, ccid);
}

void CC_SwitchBinary::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t)
{
	const uint8_t cmdId = ccid.Value();
	if (cmdId != static_cast<uint8_t>(eSwitchBinaryCommand::SWITCH_BINARY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SWITCH_BINARY unknowen CC: node={} cmdId={}", node.nodeId, cmdId);
		return;
	}
	if (params.empty())
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SWITCH_BINARY_REPORT: node={} invalid param count={}", node.nodeId, params.size());
		return;
	}

	const uint8_t value = params[0];
	node.switchBinaryValue = value;
	NotifyUI(UINotify::NodeChanged, node.nodeId);
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< SWITCH_BINARY_REPORT: node={} value=0x{:02X}",
			 node.nodeId, value);
}

//
// BASIC (0x20)
//
void CC_Basic::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	switch (static_cast<eBasicCommand>(ccid.Value()))
	{
	case eBasicCommand::BASIC_SET:
		if (!params.empty())
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "MF BASIC_SET: node={} invalid param count={}", node.nodeId, params.size());
			return;
		}
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF BASIC_SET: node {}", node.nodeId);
		frame.MakeSendData(node.nodeId, 3,
						   { static_cast<uint8_t>(CC),
							 static_cast<uint8_t>(eBasicCommand::BASIC_SET), params[0] });
		break;

	case eBasicCommand::BASIC_GET:
		if (!params.empty())
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "MF BASIC_GET: node={} invalid param count={}", node.nodeId, params.size());
			return;
		}
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF BASIC_GET: node {}", node.nodeId);
		frame.MakeSendData(node.nodeId, 3,
						   { static_cast<uint8_t>(CC),
							 static_cast<uint8_t>(eBasicCommand::BASIC_GET) });
		break;

	case eBasicCommand::BASIC_REPORT:
		if (params.empty())
		{
			Log.AddL(eLogTypes::DVC, MakeTag(), "MF BASIC_REPORT: node {} no value", node.nodeId);
			return;
		}
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF BASIC_REPORT: node {} value=0x{:02X}", node.nodeId, params[0]);
		frame.MakeSendData(node.nodeId, 3,
						   { static_cast<uint8_t>(CC),
							 static_cast<uint8_t>(eBasicCommand::BASIC_REPORT), params[0] });
		break;

	}
	Log.AddL(eLogTypes::ERR, MakeTag(), "<< BASIC unknowen CC: node={} cmdId={}", node.nodeId, ccid.Value());
}

void CC_Basic::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t destinationEP)
{
	const uint8_t cmdId = ccid.Value();

	switch (static_cast<eBasicCommand>(ccid.Value()))
	{
	case eBasicCommand::BASIC_SET:
		{
			// device send a set
			if (params.empty())
			{
				Log.AddL(eLogTypes::ERR, MakeTag(), "<< BASIC_SET: node={} no value", node.nodeId);
				return;
			}

			// unsolicited state change from device
			const uint8_t value = params[0];
			node.basicValue[destinationEP] = value;
			NotifyUI(UINotify::NodeChanged, node.nodeId);
			std::string sBV;
			for (auto v : node.basicValue)
			{
				sBV += std::to_string(v.second);
				sBV += ' ';
			}
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "<< BASIC_SET: From node={} value={} destinationEP={}", node.nodeId, sBV, destinationEP);
		}
		return;

	case eBasicCommand::BASIC_GET:
		{
			// Device send a request 
			std::string sBV;
			for (auto v : node.basicValue)
			{
				sBV += std::to_string(v.second);
				sBV += ' ';
			}
			Log.AddL(eLogTypes::DVC, MakeTag(), "<< BASIC_GET From node={} value={} destinationEP={}", node.nodeId, sBV, destinationEP);

			// Send report to device
			APIFrame apiFrame;
			if (destinationEP == 0)
			{	// standalone
				MakeFrame(apiFrame, eBasicCommand::BASIC_REPORT, { node.basicValue[0] });
				node.SendFrame(apiFrame); // send report
			}
			else
			{ // make a encapsulated report
				MakeFrame(apiFrame, eBasicCommand::BASIC_REPORT, { node.basicValue[destinationEP] });
				CC_MultiChannel mc(node);
				mc.MakeFrame(apiFrame, CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP,
							 { destinationEP,
							 static_cast<uint8_t>(eCommandClass::BASIC),
							 static_cast<uint8_t>(eBasicCommand::BASIC_REPORT),
							 node.basicValue[destinationEP]
							 });
				node.SendFrame(apiFrame);
			}
		}
		return;

	case eBasicCommand::BASIC_REPORT:
		if (params.empty())
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "<< BASIC_REPORT: node={} no value", node.nodeId);
			return;
		}
		node.basicValue[destinationEP] = params[0];
		NotifyUI(UINotify::NodeChanged, node.nodeId);
		std::string sBV;
		for (auto v : node.basicValue)
		{
			sBV += std::to_string(v.second);
			sBV += ' ';
		}
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< BASIC_REPORT: To node={} value={} destinationEP={}", node.nodeId, sBV, destinationEP);
		return;

	}
	Log.AddL(eLogTypes::ERR, MakeTag(), "<< BASIC unknown cmd: node={} cmdId=0x{:02X}", node.nodeId, (int)cmdId);
}

//
// SWITCH MULTILEVEL (0x26)
//
void CC_SwitchMultilevel::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	(void)params;
	(void)ccid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF SWITCH_MULTILEVEL_GET: node {}", node.nodeId);
	frame.MakeSendData(node.nodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::SWITCH_MULTILEVEL),
						 static_cast<uint8_t>(eSwitchMultilevelCommand::SWITCH_MULTILEVEL_GET) });
}

void CC_SwitchMultilevel::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t destinationEP)
{
	(void)destinationEP;
	const uint8_t cmdId = ccid.Value();
	if (cmdId != static_cast<uint8_t>(eSwitchMultilevelCommand::SWITCH_MULTILEVEL_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SWITCH_MULTILEVEL unknowen CC: node={} cmdId={}", node.nodeId, cmdId);
		return;
	}

	if (params.empty())
		return;

	const uint8_t level = params[0];
	node.switchMultilevelValue = level;
	NotifyUI(UINotify::NodeChanged, node.nodeId);
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< SWITCH_MULTILEVEL_REPORT: node={} level={}",
			 node.nodeId, level);
}

//
// SENSOR BINARY (0x30)
//
void CC_SensorBinary::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	(void)params;
	(void)ccid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF SENSOR_BINARY_GET: node {}", node.nodeId);
	frame.MakeSendData(node.nodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::SENSOR_BINARY),
						 static_cast<uint8_t>(eSensorBinaryCommand::SENSOR_BINARY_GET) });
}

void CC_SensorBinary::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t destinationEP)
{
	(void)destinationEP;
	const uint8_t cmdId = ccid.Value();
	if (cmdId != static_cast<uint8_t>(eSensorBinaryCommand::SENSOR_BINARY_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< SENSOR_BINARY unknowen CC: node={} cmdId={}", node.nodeId, cmdId);
		return;
	}
	if (params.empty())
		return;

	const uint8_t value = params[0];
	node.sensorBinaryValue = value;
	NotifyUI(UINotify::NodeChanged, node.nodeId);
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< SENSOR_BINARY_REPORT: node={} value={}",
			 node.nodeId, value);
}

//
// METER (0x32 / 0x50)
//
void CC_Meter::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	(void)params;
	(void)ccid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF METER_GET: node {}", node.nodeId);
	frame.MakeSendData(node.nodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::METER),
						 static_cast<uint8_t>(eMeterCommand::METER_GET) });
}

void CC_Meter::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t destinationEP)
{
	(void)destinationEP;
	const uint8_t cmdId = ccid.Value();
	if (cmdId != static_cast<uint8_t>(eMeterCommand::METER_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< METER unknowen CC: node={} cmdId={}", node.nodeId, cmdId);
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
			 node.nodeId, meterType, value);
}

//
// MULTI CHANNEL (0x60)
//
void CC_MultiChannel::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	const uint8_t cmdId = ccid.Value();
	switch (cmdId)
	{
	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP):
	{
		std::string oss;
		for (auto& param : params) oss += std::format("{:02X} ", (param));
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF MULTI_CHANNEL_CMD_ENCAP: node {} params {}", node.nodeId, oss.c_str());

		ccparams_t payload;
		payload.push_back(static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL));
		payload.push_back(static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP));
		payload.insert(payload.end(), params.begin(), params.end()); // DE | CC | CMD | PAYLOAD...
		frame.MakeSendData(node.nodeId, 4, payload);
	}
	break;

	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_END_POINT_GET):
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF MULTI_CHANNEL_END_POINT_GET: node {}", node.nodeId);
		frame.MakeSendData(node.nodeId, 4,
						   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL),
							 static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_END_POINT_GET)
						   });
		break;

	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_GET):
		Log.AddL(eLogTypes::DVC, MakeTag(), "MF MULTI_CHANNEL_CAPABILITY_GET: node {}", node.nodeId);
		frame.MakeSendData(node.nodeId, 4,
						   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL),
							 static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_GET),
							 params[0] });
		break;
	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 ">> MULTI_CHANNEL unknown CC: node={} CC={}",
				 node.nodeId, cmdId);
		break;
	}
}

void CC_MultiChannel::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t destinationEP)
{
	(void)destinationEP;
	const uint8_t cmdId = ccid.Value();

	switch (cmdId)
	{
	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP):
	{
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 "<< MULTI_CHANNEL_CMD_ENCAP: node={} params={}, it shoud be handled in Node::HandleCCDeviceReport",
				 node.nodeId, params);
	}
	break;
	//
	// MULTI_CHANNEL_END_POINT_REPORT (0x08)
	//
	case static_cast<uint8_t>(eMultiChannelCommand::MULTI_CHANNEL_END_POINT_REPORT):
	{
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< MULTI_CHANNEL_END_POINT_REPORT: node={} params={}",
				 node.nodeId, params);

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
				 node.nodeId, params);

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

		NodeInfo::EndpointInfo& DVC = node.multiChannel.endpoints[ep - 1];

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
					 node.nodeId, cmdId);
			break;
		}
	}
}

//
// CONFIGURATION (0x70)
//
void CC_Configuration::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	const uint8_t cc = static_cast<uint8_t>(eCommandClass::CONFIGURATION);
	const uint8_t cmd = ccid.Value();

	switch (cmd)
	{
	case (uint8_t)eConfigurationCommand::CONFIGURATION_GET:
		{
			// params[0] = parameter number
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 ">> CONFIGURATION_GET: node {} param={}",
					 node.nodeId, params[0]);

			frame.MakeSendData(
				node.nodeId,
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
						 "CONFIGURATION_SET missing size/value: node {}", node.nodeId);
				return;
			}

			uint8_t paramNumber = params[0];
			uint8_t size = params[1];

			if (params.size() < static_cast<size_t>(2 + size))
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "CONFIGURATION_SET wrong value size: node {} param={} size={}",
						 node.nodeId, paramNumber, size);
				return;
			}

			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF CONFIGURATION_SET: node {} param={} size={}",
					 node.nodeId, paramNumber, size);

			ccparams_t payload;
			payload.reserve(3 + size);

			payload.push_back(cc);
			payload.push_back(static_cast<uint8_t>(eConfigurationCommand::CONFIGURATION_SET));
			payload.push_back(paramNumber);
			payload.push_back(size);

			for (uint8_t i = 0; i < size; i++)
				payload.push_back(params[2 + i]);

			frame.MakeSendData(
				node.nodeId,
				static_cast<uint8_t>(payload.size()),
				payload
			);
			break;
		}

	default:
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "Unknown CONFIGURATION command: node {} cmd={}",
					 node.nodeId, cmd);
			break;
		}
	}
}

void CC_Configuration::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t)
{
	if (ccid != eConfigurationCommand::CONFIGURATION_REPORT)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< CONFIGURATION unknown CC: node={} cmdId={}", node.nodeId, ccid.Value());
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
	for (uint8_t i = 0; i < size && static_cast<size_t>(2 + i) < params.size(); i++)
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
				 node.nodeId, paramNumber, (int)node.configurationInfo.size());
		return;
	}

	if (paramNumber % 10 == 0)
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< CONFIGURATION_REPORT: node={} param={} size={} value={}",
				 node.nodeId, paramNumber, size, value);
}

//
// PROTECTION (0x75)
//
void CC_Protection::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	(void)params;
	(void)ccid;
	Log.AddL(eLogTypes::DVC, MakeTag(), "MF PROTECTION_GET: node {}", node.nodeId);
	frame.MakeSendData(node.nodeId, 3,
					   { static_cast<uint8_t>(eCommandClass::PROTECTION),
						 static_cast<uint8_t>(eProtectionCommand::PROTECTION_GET) });
}

void CC_Protection::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t destinationEP)
{
	(void)destinationEP;
	const uint8_t cmdId = ccid.Value();
	if (cmdId != static_cast<uint8_t>(eProtectionCommand::PROTECTION_REPORT))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< PROTECTION unknowen CC: node={} cmdId={}", node.nodeId, cmdId);
		return;
	}

	if (params.empty())
		return;

	node.protectionState = params[0];
	NotifyUI(UINotify::NodeChanged, node.nodeId);
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< PROTECTION_REPORT: node={} state={}",
			 node.nodeId, params[0]);
}

//
// ASSOCIATION (0x85)
//
void CC_Association::MakeFrame(APIFrame& frame, const ccid_t ccid, const ccparams_t& params)
{
	std::vector<uint8_t> payload;
	payload.reserve(2 + params.size());
	payload.push_back(static_cast<uint8_t>(eCommandClass::ASSOCIATION));
	payload.push_back(ccid.Value());
	payload.insert(payload.end(), params.begin(), params.end());

	uint8_t cb = 5; // node.GetNextCallbackId();

	switch (static_cast<eAssociationCommand>(ccid.Value()))
	{
	case eAssociationCommand::ASSOCIATION_SET:
		{
			std::string oss;
			for (size_t i = 1; i < params.size(); i++)
				oss += std::to_string(params[i]) + " ";

			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF ASSOCIATION_SET: node {} group {} = {}",
					 node.nodeId, params[0], oss);

			frame.MakeSendData(node.nodeId, cb, payload);
			break;
		}

	case eAssociationCommand::ASSOCIATION_REMOVE:
		{
			std::string oss;
			for (size_t i = 1; i < params.size(); i++)
				oss += std::to_string(params[i]) + " ";

			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF ASSOCIATION_REMOVE: node {} group {} - {}",
					 node.nodeId, params[0], oss);

			frame.MakeSendData(node.nodeId, cb, payload);
			break;
		}

	case eAssociationCommand::ASSOCIATION_GET:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF ASSOCIATION_GET: node {} group {}",
					 node.nodeId, params[0]);

			frame.MakeSendData(node.nodeId, cb, payload);
			break;
		}

	case eAssociationCommand::ASSOCIATION_GROUPINGS_GET:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF ASSOCIATION_GROUPINGS_GET: node {}",
					 node.nodeId);

			frame.MakeSendData(node.nodeId, cb, payload);
			break;
		}

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 ">> ASSOCIATION unknown CC: node={} cmdId={}",
				 node.nodeId, ccid);
		break;
	}
}

void CC_Association::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t destinationEP)
{
	(void)destinationEP;
	const uint8_t cmdId = ccid.Value();
	switch (cmdId)
	{
	case (uint8_t)eAssociationCommand::ASSOCIATION_REPORT:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "<< ASSOCIATION_REPORT: node={} {}",
					 node.nodeId, params);

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
				g.members[nodeId].nodeId = (nodeid_t)nodeId;
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
					 node.nodeId, params);

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
				 node.nodeId, cmdId);
		break;
	}
}

//
// MULTI CHANNEL ASSOCIATION (0x8E)
//
void CC_MultiChannelAssociation::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	const uint8_t cmdId = ccid.Value();
	uint8_t cb = 6; // node.GetNextCallbackId();

	switch (cmdId)
	{
	case (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET: node {}",
					 node.nodeId);

			frame.MakeSendData(node.nodeId, cb,
							   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION),cmdId });
			break;
		}

	case (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GET:
		{
			if (params.size() < 1)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "MULTI_CHANNEL_ASSOCIATION_GET missing group: node {}", node.nodeId);
				return;
			}
			auto group = params[0];
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF MULTI_CHANNEL_ASSOCIATION_GET: node {} group {}",
					 node.nodeId, group);

			frame.MakeSendData(node.nodeId, cb,
							   { static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION),cmdId, (uint8_t)group });
			break;
		}

	case (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REMOVE:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF MULTI_CHANNEL_ASSOCIATION_REMOVE: node {} group {} params={}",
					 node.nodeId, params[0], params);

			ccparams_t payLoad;
			payLoad.push_back(static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION));
			payLoad.push_back(cmdId);
			payLoad.insert(payLoad.end(), params.begin(), params.end());

			frame.MakeSendData(node.nodeId, cb, payLoad);
			break;
		}

	case (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_SET:
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF MULTI_CHANNEL_ASSOCIATION_SET: node {} group {} params={}",
					 node.nodeId, params[0], params);

			ccparams_t payLoad;
			payLoad.push_back(static_cast<uint8_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION));
			payLoad.push_back(cmdId);
			payLoad.insert(payLoad.end(), params.begin(), params.end());

			frame.MakeSendData(node.nodeId, cb, payLoad);
			break;
		}

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 "<< MULTI_CHANNEL_ASSOCIATION unknown CC: node={} cmdId={}",
				 node.nodeId, cmdId);
		break;
	}
}

void CC_MultiChannelAssociation::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t destinationEP)
{
	(void)destinationEP;
	const uint8_t cmdId = ccid.Value();

	//
	// --- MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT (0x06) ---
	//
	if (cmdId == (uint8_t)eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT)
	{
		if (params.size() < 1)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(),
					 "<< MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT too short: node={}",
					 node.nodeId);
			return;
		}

		const uint8_t supportedGroups = params[0];

		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT: node={} supportedGroups={}",
				 node.nodeId, supportedGroups);

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
					 node.nodeId);
			return;
		}

		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< MULTI_CHANNEL_ASSOCIATION_REPORT: node={} {}",
				 node.nodeId, params);

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

			g.members[nodeId].nodeId = (nodeid_t)nodeId;
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

			g.members[nodeId].nodeId = (nodeid_t)nodeId;
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
void CC_WakeUp::MakeFrame(APIFrame& frame, ccid_t ccid, const ccparams_t& params)
{
	const uint8_t cmdId = ccid.Value();
	auto* cc = node.GetCC(eCommandClass::WAKE_UP);
	uint8_t version = cc ? cc->version : 1;

	switch (cmdId)
	{
	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_INTERVAL_GET):
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 ">> WAKE_UP_INTERVAL_GET (v{}): node {}", version, node.nodeId);

		frame.MakeSendData(node.nodeId, 2,
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
				 version, node.nodeId,
				 (params[0] << 16) | (params[1] << 8) | params[2]);

		frame.MakeSendData(node.nodeId, 5,
						   { uint8_t(eCommandClass::WAKE_UP),
							 uint8_t(eWakeUpCommand::WAKE_UP_INTERVAL_SET),
							 params[0], params[1], params[2] });
		break;

	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_NO_MORE_INFORMATION):
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "MF WAKE_UP_NO_MORE_INFORMATION (v{}): node {}", version, node.nodeId);

		frame.MakeSendData(node.nodeId, 2,
						   { uint8_t(eCommandClass::WAKE_UP),
							 uint8_t(eWakeUpCommand::WAKE_UP_NO_MORE_INFORMATION) });
		break;

	case static_cast<uint8_t>(eWakeUpCommand::WAKE_UP_INTERVAL_CAPABILITIES_GET):
		if (version >= 3)
		{
			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "MF WAKE_UP_INTERVAL_CAPABILITIES_GET (v{}): node {}", version, node.nodeId);

			frame.MakeSendData(node.nodeId, 2,
							   { uint8_t(eCommandClass::WAKE_UP),
								 uint8_t(eWakeUpCommand::WAKE_UP_INTERVAL_CAPABILITIES_GET) });
		}
		break;

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 "Unknown WAKE_UP cmdId={} (v{}) for node {}", cmdId, version, node.nodeId);
		break;
	}
}

//
// HANDLE REPORT
//
void CC_WakeUp::HandleReport(const ccid_t ccid, const ccparams_t& params, const uint8_t destinationEP)
{
	(void)destinationEP;
	const uint8_t cmdId = ccid.Value();
	auto* cc = node.GetCC(eCommandClass::WAKE_UP);
	uint8_t version = cc ? cc->version : 1;

	NodeInfo::WakeUpInfo& wup = node.wakeUpInfo;
	switch (cmdId)
	{
	case (uint8_t)eWakeUpCommand::WAKE_UP_NOTIFICATION:
		Log.AddL(eLogTypes::DVC, MakeTag(),
				 "<< WAKE_UP_NOTIFICATION (v{}): node {} is awake", version, node.nodeId);

		node.WakeUp();
		wup.lastWakeUp = std::chrono::system_clock::now();
		break;

	case (uint8_t)eWakeUpCommand::WAKE_UP_INTERVAL_REPORT:
		if (version == 1)
		{
			if (params.size() < 3)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "<< WAKE_UP_INTERVAL_REPORT invalid v1 params: node {}", node.nodeId);
				return;
			}

			uint32_t interval = (static_cast<uint32_t>(params[0]) << 16) | (static_cast<uint32_t>(params[1]) << 8) | static_cast<uint32_t>(params[2]);
			wup.wakeUpInterval = interval;
			wup.hasLastReport = true;
			node.wakeUpInfo = wup;

			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "<< WAKE_UP_INTERVAL_REPORT v1: node {} interval={}s",
					 node.nodeId, interval);
		}
		else if (version >= 2)
		{
			if (params.size() < 9)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "<< WAKE_UP_INTERVAL_REPORT invalid v2 params: node {}", node.nodeId);
				return;
			}

			uint32_t interval = (static_cast<uint32_t>(params[0]) << 16) | (static_cast<uint32_t>(params[1]) << 8) | static_cast<uint32_t>(params[2]);
			uint32_t minInt = (static_cast<uint32_t>(params[3]) << 16) | (static_cast<uint32_t>(params[4]) << 8) | static_cast<uint32_t>(params[5]);
			uint32_t maxInt = (static_cast<uint32_t>(params[6]) << 16) | (static_cast<uint32_t>(params[7]) << 8) | static_cast<uint32_t>(params[8]);

			wup.wakeUpInterval = interval;
			wup.wakeUpMin = minInt;
			wup.wakeUpMax = maxInt;
			wup.hasLastReport = true;
			node.wakeUpInfo = wup;

			Log.AddL(eLogTypes::DVC, MakeTag(),
					 "<< WAKE_UP_INTERVAL_REPORT v{}: node {} interval={}s min={}s max={}s",
					 version, node.nodeId, interval, minInt, maxInt);
		}
		break;

	case (uint8_t)eWakeUpCommand::WAKE_UP_INTERVAL_CAPABILITIES_REPORT:
		if (version >= 3)
		{
			if (params.size() < 9)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "<< WAKE_UP_INTERVAL_CAPABILITIES_REPORT invalid params: node {}", node.nodeId);
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
					 version, node.nodeId, minInt, maxInt, defInt);
		}
		break;

	default:
		Log.AddL(eLogTypes::ERR, MakeTag(),
				 "<< WAKE_UP unknown CC (v{}): node={} cmdId={}",
				 version, node.nodeId, cmdId);
		break;
	}
};

