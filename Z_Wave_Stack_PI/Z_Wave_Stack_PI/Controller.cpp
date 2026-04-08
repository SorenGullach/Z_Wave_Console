#include "Controller.h"
#include "Logging.h"
#include <chrono>
#include <iostream> 
#include <thread>
#include "NodeId_t.h"

Controller::Controller()
	: Nodes([this](const APIFrame& f) { Enqueue(f); })
	, initialize([this](const APIFrame& f) { Enqueue(f); }, static_cast<ControllerInfo&>(*this))
	, nodeInterview(static_cast<Nodes&>(*this), [this](const APIFrame& f) { Enqueue(f); })
{
}

Controller::~Controller() = default;

void Controller::Start()
{
	static constexpr auto initializeTimeout = std::chrono::seconds(10);
	static constexpr auto initializePollInterval = std::chrono::milliseconds(1);

	initialize.Start();
	auto initializeDeadline = std::chrono::steady_clock::now() + initializeTimeout;

	while (!initialize.Done())
	{
		if (std::chrono::steady_clock::now() >= initializeDeadline)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Controller initialization timed out after {} ms", initializeTimeout.count() * 1000);
			break;
		}

		std::this_thread::sleep_for(initializePollInterval);
	}

	if (initialize.Done())
	{
		std::cout << ControllerInfo::ToString() << "\n";
		for (auto id : NodeIds)
		{
			if (id == nodeid_t{ 0 } || id == nodeid_t{ 1 }) continue;

			initializeDeadline = std::chrono::steady_clock::now() + initializeTimeout;

			while (!nodeInterview.Done(id))
			{
				nodeInterview.Start(id);
				if (std::chrono::steady_clock::now() >= initializeDeadline)
				{
					Log.AddL(eLogTypes::ERR, MakeTag(), "Node {} initialization timed out after {} ms", id, initializeTimeout.count() * 1000);
					break;
				}
				std::this_thread::sleep_for(initializePollInterval);
			}

			if (nodeInterview.Done(id))
			{
				auto* node = Nodes::Get(id);
				std::cout << node->ToString() << "\n";
			}
		}
	}
}

bool Controller::OnFrameReceived(const APIFrame& frame)
{
	switch (frame.APICmd.CmdId)
	{
	case eCommandIds::FUNC_ID_GET_INIT_DATA:
	case eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES:
	case eCommandIds::FUNC_ID_GET_CAPABILITIES:
	case eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION:
	case eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY:
	case eCommandIds::FUNC_ID_GET_LIBRARY_VERSION:
	case eCommandIds::FUNC_ID_GET_LIBRARY_TYPE:
		return initialize.HandleFrame(frame);

	case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
	case eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA:
	case eCommandIds::ZW_API_APPLICATION_UPDATE:
		return nodeInterview.HandleFrame(frame);

	case eCommandIds::ZW_API_CONTROLLER_SEND_DATA:
		if (frame.Type() == APIFrame::eFrameTypes::RES)
			Log.AddL(eLogTypes::RTU, MakeTag(), "<< route=sendData type=RES txStatus=0x{:02X} len={}", frame.payload[0], frame.payload.size());
		if (frame.Type() == APIFrame::eFrameTypes::REQ)
			Log.AddL(eLogTypes::RTU, MakeTag(), "<< route=sendData type=REQ sessionId={} txStatus=0x{:02X} len={}", frame.payload[0], frame.payload[1], frame.payload.size());
		return true;

	case eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER:
		Log.AddL(eLogTypes::RTU, MakeTag(), "<< route=ccDispatcher {}", frame.Info());
		HandleCCFrame(frame.payload);
		return true;

	case eCommandIds::ZW_API_IS_NODE_FAILED:
		//		nodes.HandleNodeFailed(frame.payload[0]);
		return true;

	case eCommandIds::ZW_API_REMOVE_FAILED_NODE:
	case eCommandIds::ZW_API_REMOVE_NODE_FROM_NETWORK:
		return true;

	default:
		return false;

	}
}

bool Controller::OnFrameReceivedTimeout(const APIFrame& frame)
{
	switch (frame.APICmd.CmdId)
	{
	case eCommandIds::FUNC_ID_GET_INIT_DATA:
	case eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES:
	case eCommandIds::FUNC_ID_GET_CAPABILITIES:
	case eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION:
	case eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY:
	case eCommandIds::FUNC_ID_GET_LIBRARY_VERSION:
	case eCommandIds::FUNC_ID_GET_LIBRARY_TYPE:
		Log.AddL(eLogTypes::DBG, MakeTag(), "<< route=initializeManager TIMEOUT {}", frame.Info());
		return initialize.HandleFrameTimeout(frame);

	case eCommandIds::ZW_API_APPLICATION_UPDATE:
	case eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA:
	case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
		Log.AddL(eLogTypes::DVC, MakeTag(), "<< route=interviewManager TIMEOUT {}", frame.Info());
		return nodeInterview.HandleFrameTimeout(frame);

	case eCommandIds::ZW_API_CONTROLLER_SEND_DATA:
		return true;

	case eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER:
		Log.AddL(eLogTypes::DVC, MakeTag(), "<< route=ccDispatcher TIMEOUT {}", frame.Info());
		HandleCCFrameTimeout(frame.payload);
		return true;

	default:
		return false;
	}
}

// Parses frames delivered via Z-Wave Serial API `ZW_API_APPLICATION_COMMAND_HANDLER` and
// dispatches to command-class specific decoders/interview handlers.
void Controller::HandleCCFrame(const APIFrame::PayLoad& payload)
{
	if (payload.empty())
	{
		Log.AddL(eLogTypes::RTU, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: empty payload (drop)");
		return;
	}

	// Serial API payload layout is controller/SDK dependent. In this project we treat:
	// payload[0]=rxStatus, payload[1]=source nodeId, payload[2]=cmdLen, payload[3..]=cmd bytes.
	// Any trailing bytes after the cmd bytes (if present) are treated as metadata (e.g., RSSI).
	const uint8_t rxStatus = payload[0];
	const nodeid_t nodeId = (nodeid_t)(payload.size() > 1 ? payload[1] : 0);

	if (payload.size() < 5)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: truncated payload (fromNode={} payloadLen={})", nodeId, payload.size());
		return;
	}

	if (!Nodes::Exists(nodeId))
	{
		Log.AddL(eLogTypes::RTU, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: unknown node (fromNode={} payloadLen={})", nodeId, payload.size());
		return;
	}

	// In this stack we treat payload[2] as the length of the CC command bytes.
	const uint8_t cmdLen = payload[2];
	const size_t cmdStart = 3;
	const size_t cmdEnd = cmdStart + static_cast<size_t>(cmdLen);
	const bool hasTrailingMeta = payload.size() > cmdEnd;
	const uint8_t rssi = hasTrailingMeta ? payload.back() : 0;
	if (hasTrailingMeta)
		Log.AddL(eLogTypes::RTU, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: fromNode={} rxStatus=0x{:02X} payloadLen={} cmdLen={} rssi=0x{:02X}",
				 nodeId, rxStatus, payload.size(), cmdLen, rssi);
	else
		Log.AddL(eLogTypes::RTU, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: fromNode={} rxStatus=0x{:02X} payloadLen={} cmdLen={}",
				 nodeId, rxStatus, payload.size(), cmdLen);

	if (payload.size() < cmdEnd)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< ZW_API_APPLICATION_COMMAND_HANDLER: truncated cmdBytes (fromNode={} cmdLen={} payloadLen={})", nodeId, cmdLen, payload.size());
		return;
	}

	const ccparams_t cmd(payload.begin() + cmdStart, payload.begin() + cmdEnd);
	if (cmd.size() < 2)
		return;

	const eCommandClass cmdClass = static_cast<eCommandClass>(cmd[0]);
	const ccid_t cmdId(cmd[1]);
	const ccparams_t cmdParams(cmd.begin() + 2, cmd.end());
	Log.AddL(eLogTypes::RTU, MakeTag(), "CC frame: fromNode={} cc=0x{:02X} cmd=0x{:02X} paramsLen={}",
			 nodeId, (uint8_t)cmdClass, (int)cmdId.value, cmdParams.size());

	switch ((static_cast<uint16_t>(cmdClass) << 8) | cmdId.value)
	{
		//
		// All device CCs handled by ZW_Node / ZW_Device
		//
		case (static_cast<uint16_t>(eCommandClass::VERSION) << 8) | static_cast<uint8_t>(CC_Version::eVersionCommand::VERSION_COMMAND_CLASS_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MANUFACTURER_SPECIFIC) << 8) | static_cast<uint8_t>(CC_ManufacturerSpecific::eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MANUFACTURER_SPECIFIC) << 8) | static_cast<uint8_t>(CC_ManufacturerSpecific::eManufacturerSpecificCommand::DEVICE_SPECIFIC_REPORT_V2) :

			case (static_cast<uint16_t>(eCommandClass::BATTERY) << 8) | static_cast<uint8_t>(CC_Battery::eBatteryCommand::BATTERY_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::BASIC) << 8) | static_cast<uint8_t>(CC_Basic::eBasicCommand::BASIC_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::BASIC) << 8) | static_cast<uint8_t>(CC_Basic::eBasicCommand::BASIC_SET) :
			case (static_cast<uint16_t>(eCommandClass::BASIC) << 8) | static_cast<uint8_t>(CC_Basic::eBasicCommand::BASIC_GET) :
			case (static_cast<uint16_t>(eCommandClass::SWITCH_BINARY) << 8) | static_cast<uint8_t>(CC_SwitchBinary::eSwitchBinaryCommand::SWITCH_BINARY_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::SWITCH_MULTILEVEL) << 8) | static_cast<uint8_t>(CC_SwitchMultilevel::eSwitchMultilevelCommand::SWITCH_MULTILEVEL_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::SENSOR_BINARY) << 8) | static_cast<uint8_t>(CC_SensorBinary::eSensorBinaryCommand::SENSOR_BINARY_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::METER) << 8) | static_cast<uint8_t>(CC_Meter::eMeterCommand::METER_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MULTI_CHANNEL) << 8) | static_cast<uint8_t>(CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CAPABILITY_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MULTI_CHANNEL) << 8) | static_cast<uint8_t>(CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP) :
			case (static_cast<uint16_t>(eCommandClass::CONFIGURATION) << 8) | static_cast<uint8_t>(CC_Configuration::eConfigurationCommand::CONFIGURATION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::PROTECTION) << 8) | static_cast<uint8_t>(CC_Protection::eProtectionCommand::PROTECTION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::ASSOCIATION) << 8) | static_cast<uint8_t>(CC_Association::eAssociationCommand::ASSOCIATION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::ASSOCIATION) << 8) | static_cast<uint8_t>(CC_Association::eAssociationCommand::ASSOCIATION_GROUPINGS_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION) << 8) | static_cast<uint8_t>(CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_REPORT) :
			case (static_cast<uint16_t>(eCommandClass::MULTI_CHANNEL_ASSOCIATION) << 8) | static_cast<uint8_t>(CC_MultiChannelAssociation::eMultiChannelAssociationCommand::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT) :

			// All device CCs handled here
			Nodes::HandleCCDeviceReport(nodeId, cmdClass, cmdId.value, cmdParams);
			break;

			//
			// Default
			//
			default:
				Log.AddL(eLogTypes::ERR, MakeTag(),
						 "Controller.HandleCCFrame: unhandled cc=0x{:02X} cmd=0x{:02X} fromNode={} (cmdLen={}, payloadLen={})",
						 (uint8_t)cmdClass, (int)cmdId.value, nodeId, cmdLen, payload.size());
				break;
	}
}

// Called by the interface layer when a waited-for callback/response did not arrive.
void Controller::HandleCCFrameTimeout(const APIFrame::PayLoad& payload)
{
	Log.AddL(eLogTypes::RTU, MakeTag(), "Controller.HandleCCFrameTimeout: payloadLen={}", payload.size());
}

