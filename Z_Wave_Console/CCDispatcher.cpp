#include <vector>
#include "CCDispatcher.h"

#include "Logging.h"
#include "Nodes.h"
#include "CommandClass.h"

// Parses frames delivered via Z-Wave Serial API `ZW_API_APPLICATION_COMMAND_HANDLER` and
// dispatches to command-class specific decoders/interview handlers.
void ZW_CCDispatcher::HandleCCFrame(const std::vector<uint8_t>& payload)
{
	if (payload.empty())
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: empty payload (drop)");
		return;
	}

	// Serial API payload layout is controller/SDK dependent, but typically starts with
	// rxStatus + source nodeId and ends with an RSSI byte.
	const uint8_t rxStatus = payload[0];
	const uint8_t nodeId = payload.size() > 1 ? payload[1] : 0;
	const uint8_t rssi = payload.back();
	Log.AddL(eLogTypes::DBG, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: fromNode={} rxStatus=0x{:02X} payloadLen={} rssi=0x{:02X}",
			 nodeId, rxStatus, payload.size(), rssi);

	if (payload.size() < 5)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: truncated payload (fromNode={} payloadLen={})", nodeId, payload.size());
		return;
	}

	if (!nodes.Exists(nodeId))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: unknown node (fromNode={} payloadLen={})", nodeId, payload.size());
		return;
	}

	// In this stack we treat payload[2] as the length of the CC command bytes.
	const uint8_t cmdLen = payload[2];
	if (payload.size() < static_cast<size_t>(3 + cmdLen))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: truncated cmdBytes (fromNode={} cmdLen={} payloadLen={})", nodeId, cmdLen, payload.size());
		return;
	}

	const std::vector<uint8_t> cmd(payload.begin() + 3, payload.begin() + 3 + cmdLen);
	if (cmd.size() < 2)
		return;

	const uint8_t cmdClass = cmd[0];
	const uint8_t cmdId = cmd[1];
	const std::vector<uint8_t> cmdParams(cmd.begin() + 2, cmd.end());
	Log.AddL(eLogTypes::DBG, MakeTag(), "CC frame: fromNode={} cc=0x{:02X} cmd=0x{:02X} paramsLen={}", nodeId, cmdClass, cmdId, cmdParams.size());

	switch ((static_cast<uint16_t>(cmdClass) << 8) | cmdId)
	{
		//
		// All device CCs handled by ZW_Node / ZW_Device
		//
		case (static_cast<uint16_t>(eCommandClass::VERSION) << 8) | static_cast<uint8_t>(ZW_CC_Version::eVersionCommand::VERSION_COMMAND_CLASS_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MANUFACTURER_SPECIFIC) << 8) | static_cast<uint8_t>(ZW_CC_ManufacturerSpecific::eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MANUFACTURER_SPECIFIC) << 8) | static_cast<uint8_t>(ZW_CC_ManufacturerSpecific::eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT_V2) :

			case (static_cast<uint16_t>(eCommandClass::BATTERY) << 8) | static_cast<uint8_t>(ZW_CC_Battery::eBatteryCommand::BATTERY_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::BASIC) << 8) | static_cast<uint8_t>(ZW_CC_Basic::eBasicCommand::BASIC_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::BASIC) << 8) | static_cast<uint8_t>(ZW_CC_Basic::eBasicCommand::BASIC_SET) :
			case (static_cast<uint16_t>(eCommandClass::BASIC) << 8) | static_cast<uint8_t>(ZW_CC_Basic::eBasicCommand::BASIC_GET) :
			case (static_cast<uint16_t>(eCommandClass::SWITCH_BINARY) << 8) | static_cast<uint8_t>(ZW_CC_SwitchBinary::eSwitchBinaryCommand::SWITCH_BINARY_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::SWITCH_MULTILEVEL) << 8) | static_cast<uint8_t>(ZW_CC_SwitchMultilevel::eSwitchMultilevelCommand::SWITCH_MULTILEVEL_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::SENSOR_BINARY) << 8) | static_cast<uint8_t>(ZW_CC_SensorBinary::eSensorBinaryCommand::SENSOR_BINARY_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::METER) << 8) | static_cast<uint8_t>(ZW_CC_Meter::eMeterCommand::METER_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MULTI_CHANNEL) << 8) | static_cast<uint8_t>(ZW_CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::CONFIGURATION) << 8) | static_cast<uint8_t>(ZW_CC_Configuration::eConfigurationCommand::CONFIGURATION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::PROTECTION) << 8) | static_cast<uint8_t>(ZW_CC_Protection::eProtectionCommand::PROTECTION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::ASSOCIATION) << 8) | static_cast<uint8_t>(ZW_CC_Association::eAssociationCommand::ASSOCIATION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::ASSOCIATION) << 8) | static_cast<uint8_t>(ZW_CC_Association::eAssociationCommand::ASSOCIATION_GROUPINGS_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION) << 8) | static_cast<uint8_t>(ZW_CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REPORT) :

			// All device CCs handled here
			nodes.HandleCCDeviceReport(nodeId, cmdClass, cmdId, cmdParams);
			break;

			//
			// Default
			//
			default:
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "CCDispatcher.HandleCCFrame: unhandled cc=0x{:02X} cmd=0x{:02X} fromNode={} (cmdLen={}, payloadLen={})",
						 cmdClass, cmdId, nodeId, cmdLen, payload.size());
				break;
	}
}

// Called by the interface layer when a waited-for callback/response did not arrive.
void ZW_CCDispatcher::HandleCCFrameTimeout(const std::vector<uint8_t>& payload)
{
	Log.AddL(eLogTypes::ERR, MakeTag(), "CCDispatcher.HandleCCFrameTimeout: payloadLen={}", payload.size());
}
/*
// Manufacturer Specific CC / Device Specific report handler.
void ZW_CCDispatcher::HandleManufacturerSpecificCCReport(uint8_t cmdId, const std::vector<uint8_t>& cmdParams, uint16_t nodeId)
{
	ZW_Node* node = nodes.Get(nodeId);
	if (!node)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "MANUFACTURER_SPECIFIC_REPORT: node={} not found", nodeId);
		return;
	}

	if (cmdId == 0x05)
	{
		// MANUFACTURER_SPECIFIC_REPORT: [mfgId MSB, mfgId LSB, prodType MSB, prodType LSB, prodId MSB, prodId LSB]
		if (cmdParams.size() < 6)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "MANUFACTURER_SPECIFIC_REPORT: node={} malformed (len={})", nodeId, cmdParams.size());
			return;
		}

		node->manufacturerInfo.mfgId = (static_cast<uint16_t>(cmdParams[0]) << 8) | cmdParams[1];
		node->manufacturerInfo.prodType = (static_cast<uint16_t>(cmdParams[2]) << 8) | cmdParams[3];
		node->manufacturerInfo.prodId = (static_cast<uint16_t>(cmdParams[4]) << 8) | cmdParams[5];
		node->manufacturerInfo.hasManufacturerData = true;

		Log.AddL(eLogTypes::INFO, MakeTag(), "<< MANUFACTURER_SPECIFIC_REPORT: node={} mfgId=0x{:04X} prodType=0x{:04X} prodId=0x{:04X}", nodeId, node->manufacturerInfo.mfgId, node->manufacturerInfo.prodType, node->manufacturerInfo.prodId);
	}
	else if (cmdId == 0x07)
	{
		// DEVICE_SPECIFIC_REPORT format depends on SDK; we read type/format/len and then copy the id bytes.
		if (cmdParams.size() < 3)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "DEVICE_SPECIFIC_REPORT: node={} malformed (len={})", nodeId, cmdParams.size());
			return;
		}

		uint8_t deviceIdType = cmdParams[0] & 0x07; // Device ID type
		uint8_t dataFormat = (cmdParams[1] >> 5) & 0x07; // Data format
		uint8_t dataLength = cmdParams[1] & 0x07; // Data length

		if (cmdParams.size() < 3 + dataLength)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "DEVICE_SPECIFIC_REPORT: node={} truncated (len={}, expected={})", nodeId, cmdParams.size(), 3 + dataLength);
			return;
		}
		std::vector<uint8_t> deviceIdData(cmdParams.begin() + 3, cmdParams.begin() + 3 + dataLength);
		node->manufacturerInfo.deviceIdType = deviceIdType;
		node->manufacturerInfo.deviceIdFormat = dataFormat;
		node->manufacturerInfo.hasDeviceId = true;

		Log.AddL(eLogTypes::INFO, MakeTag(), "<< DEVICE_SPECIFIC_REPORT: node={} type=0x{:02X} format=0x{:02X} len={} dataSize={}", nodeId, deviceIdType, dataFormat, dataLength, deviceIdData.size());
	}
	else
	{
		Log.AddL(eLogTypes::DBG, MakeTag(), "<< MANUFACTURER_SPECIFIC_REPORT: node={} cmd=0x{:02X} len={}", nodeId, cmdId, cmdParams.size());
	}
}

*/