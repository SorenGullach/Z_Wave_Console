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
		Log.AddL(eLogTypes::INFO_LOW, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: empty payload (drop)");
		return;
	}

	// Serial API payload layout is controller/SDK dependent. In this project we treat:
	// payload[0]=rxStatus, payload[1]=source nodeId, payload[2]=cmdLen, payload[3..]=cmd bytes.
	// Any trailing bytes after the cmd bytes (if present) are treated as metadata (e.g., RSSI).
	const uint8_t rxStatus = payload[0];
	const uint8_t nodeId = payload.size() > 1 ? payload[1] : 0;

	if (payload.size() < 5)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: truncated payload (fromNode={} payloadLen={})", nodeId, payload.size());
		return;
	}

	if (!nodes.Exists(nodeId))
	{
		Log.AddL(eLogTypes::INFO_LOW, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: unknown node (fromNode={} payloadLen={})", nodeId, payload.size());
		return;
	}

	// In this stack we treat payload[2] as the length of the CC command bytes.
	const uint8_t cmdLen = payload[2];
	const size_t cmdStart = 3;
	const size_t cmdEnd = cmdStart + static_cast<size_t>(cmdLen);
	const bool hasTrailingMeta = payload.size() > cmdEnd;
	const uint8_t rssi = hasTrailingMeta ? payload.back() : 0;
	if (hasTrailingMeta)
		Log.AddL(eLogTypes::DBG, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: fromNode={} rxStatus=0x{:02X} payloadLen={} cmdLen={} rssi=0x{:02X}",
				 nodeId, rxStatus, payload.size(), cmdLen, rssi);
	else
		Log.AddL(eLogTypes::DBG, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: fromNode={} rxStatus=0x{:02X} payloadLen={} cmdLen={}",
				 nodeId, rxStatus, payload.size(), cmdLen);

	if (payload.size() < cmdEnd)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: truncated cmdBytes (fromNode={} cmdLen={} payloadLen={})", nodeId, cmdLen, payload.size());
		return;
	}

	const ZW_ByteVector cmd(payload.begin() + cmdStart, payload.begin() + cmdEnd);
	if (cmd.size() < 2)
		return;

	const eCommandClass cmdClass = static_cast<eCommandClass>(cmd[0]);
	const ZW_CmdId cmdId(cmd[1]);
	const ZW_ByteVector cmdParams(cmd.begin() + 2, cmd.end());
	Log.AddL(eLogTypes::DBG, MakeTag(), "CC frame: fromNode={} cc=0x{:02X} cmd=0x{:02X} paramsLen={}",
			 nodeId, (uint8_t)cmdClass, (int)cmdId.value, cmdParams.size());

	switch ((static_cast<uint16_t>(cmdClass) << 8) | cmdId.value)
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
			case (static_cast<uint16_t>(eCommandClass::MULTI_CHANNEL) << 8) | static_cast<uint8_t>(ZW_CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP) :
			case (static_cast<uint16_t>(eCommandClass::CONFIGURATION) << 8) | static_cast<uint8_t>(ZW_CC_Configuration::eConfigurationCommand::CONFIGURATION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::PROTECTION) << 8) | static_cast<uint8_t>(ZW_CC_Protection::eProtectionCommand::PROTECTION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::ASSOCIATION) << 8) | static_cast<uint8_t>(ZW_CC_Association::eAssociationCommand::ASSOCIATION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::ASSOCIATION) << 8) | static_cast<uint8_t>(ZW_CC_Association::eAssociationCommand::ASSOCIATION_GROUPINGS_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION) << 8) | static_cast<uint8_t>(ZW_CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION) << 8) | static_cast<uint8_t>(ZW_CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT) :

			// All device CCs handled here
			nodes.HandleCCDeviceReport(nodeId, cmdClass, cmdId.value, cmdParams);
			break;

			//
			// Default
			//
			default:
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "CCDispatcher.HandleCCFrame: unhandled cc=0x{:02X} cmd=0x{:02X} fromNode={} (cmdLen={}, payloadLen={})",
						 (uint8_t)cmdClass, (int)cmdId.value, nodeId, cmdLen, payload.size());
				break;
	}
}

// Called by the interface layer when a waited-for callback/response did not arrive.
void ZW_CCDispatcher::HandleCCFrameTimeout(const std::vector<uint8_t>& payload)
{
	Log.AddL(eLogTypes::INFO_LOW, MakeTag(), "CCDispatcher.HandleCCFrameTimeout: payloadLen={}", payload.size());
}
