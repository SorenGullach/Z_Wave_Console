#pragma once

#include <cstdint>
#include <string>

#include "FormatCompat.h"

// ===============================================================
// Z-Wave Host API command identifiers and flow types
//
// This header defines:
//  - `eCommandIds`: Function IDs used in Serial API frames.
//  - `eFlowType`  : Expected frame flow for a command (ack, response, callback).
//  - `ZW_APICommand` + `APICommands[]` lookup table.
//
// The rest of the code uses this to:
//  - Build request frames (`ZW_APIFrame::Make`)
//  - Decide how to wait for ACK/response (`Interface`)
//  - Route decoded responses (`Z_Wave::OnFrameReceived`)
// ===============================================================

// ===============================================================
// Frame Flow Types
// ===============================================================
enum class eFlowType : uint8_t
{
	Unknown = 0,
	Unacknowledged,
	AckOnly,
	AckWithResponse,
	AckWithCallback,
	AckWithResponseCallback,
	Unsolicited
};

// ===============================================================
// Z-Wave Host API – Main Command IDs (subset)
// ===============================================================
enum class eCommandIds : uint8_t
{
	// 4.3.x Capability API
	FUNC_ID_GET_INIT_DATA = 0x02,
	FUNC_ID_SET_APPLICATION_NODE_INFORMATION = 0x03,
	FUNC_ID_SET_APPLICATION_NODE_INFORMATION_COMMAND_CLASSES = 0x0C,
	FUNC_ID_GET_CONTROLLER_CAPABILITIES = 0x05,
	FUNC_ID_GET_CAPABILITIES = 0x07,
	GetLongRangeNodes = 0xDA,
	GetLongRangeChannel = 0xDB,
	SetLongRangeChannel = 0xDC,
	GetNLSNodes = 0xC0,
	FUNC_ID_GET_PROTOCOL_VERSION = 0x09,
	FUNC_ID_GET_LIBRARY_VERSION = 0x15,
	FUNC_ID_GET_LIBRARY_TYPE = 0xBD,
	FUNC_ID_SOFT_RESET = 0x08,
	FUNC_ID_SET_DEFAULT = 0x42,
	FUNC_ID_SETUP_ZWAVE_API = 0x0B,
	FUNC_ID_GET_MANUFACTURER_INFO = 0xEA,

	// 4.4.1 Common Network Management
	ZW_API_SEND_NODE_INFORMATION = 0x12,
	ZW_API_GET_NODE_INFO_PROTOCOL_DATA = 0x41,
	ZW_API_REQUEST_NODE_INFORMATION = 0x60,
	ZW_API_SET_LEARN_MODE = 0x50,
	ZW_API_GET_SUC_NODE_ID = 0x56,
	ZW_API_EXPLORE_REQUEST_INCLUSION = 0x5E,
	ZW_API_EXPLORE_REQUEST_EXCLUSION = 0x5F,
	ZW_API_SEND_NOP = 0xE9,
	ZW_API_SET_SMARTSTART_INCLUSION_INTERVAL = 0xD6,

	// 4.4.2 End Node Network Management
	ZW_API_CLEAR_NETWORK_STATISTICS = 0x39,
	ZW_API_GET_NETWORK_STATISTICS = 0x3A,
	ZW_API_REMOVE_SPECIFIC_NODE = 0x3F,
	ZW_API_REQUEST_NEW_ROUTE_DESTINATIONS = 0x5C,
	ZW_API_IS_NODE_WITHIN_DIRECT_RANGE = 0x5D,

	// 4.4.3 Controller Network Management
	ZW_API_ADD_NODE_TO_NETWORK = 0x4A,
	ZW_API_REMOVE_NODE_FROM_NETWORK = 0x4B,
	ZW_API_ADD_CONTROLLER_AND_ASSIGN_PRIMARY = 0x4C,
	ZW_API_ADD_PRIMARY_CONTROLLER = 0x4D,
	ZW_API_IS_NODE_FAILED = 0x62,
	ZW_API_REMOVE_FAILED_NODE = 0x61,
	ZW_API_REPLACE_FAILED_NODE = 0x63,
	ZW_API_ASSIGN_RETURN_ROUTE = 0x46,
	ZW_API_DELETE_RETURN_ROUTE = 0x47,
	ZW_API_ASSIGN_SUC_RETURN_ROUTE = 0x51,
	ZW_API_REQUEST_NETWORK_UPDATE = 0x53,
	ZW_API_SET_SUC_NODE_ID = 0x54,
	ZW_API_DELETE_SUC_RETURN_ROUTE = 0x55,
	ZW_API_SEND_SUC_NODE_ID = 0x57,
	ZW_API_ASSIGN_PRIORITY_RETURN_ROUTE = 0x4F,
	ZW_API_ASSIGN_PRIORITY_SUC_RETURN_ROUTE = 0x58,
	ZW_API_GET_NEIGHBOR_TABLE_LINE = 0x80,
	ZW_API_GET_ROUTING_TABLE_ENTRIES = 0x85,
	ZW_API_LOCK_UNLOCK_LAST_ROUTE = 0x90,
	ZW_API_GET_PRIORITY_ROUTE = 0x92,
	ZW_API_SET_PRIORITY_ROUTE = 0x93,
	ZW_API_REQUEST_NODE_NEIGHBOR_DISCOVERY = 0x48,
	ZW_API_REQUEST_NODE_TYPE_NEIGHBOR_UPDATE = 0x68,
	ZW_API_TRANSFER_PROTOCOL_CC = 0x69,
	ZW_API_ENABLE_NODE_NLS = 0x6A,
	ZW_API_GET_NODE_NLS_STATE = 0x6B,
	ZW_API_SET_VIRTUAL_NODE_APP_INFO = 0xA0,
	ZW_API_VIRTUAL_NODE_SEND_NODE_INFO = 0xA2,
	ZW_API_SET_VIRTUAL_NODE_LEARN_MODE = 0xA4,
	ZW_API_SET_LR_SHADOW_NODEIDS = 0xDD,

	// 4.5 Z-Wave API Memory
	ZW_API_GET_NETWORK_IDS_FROM_MEMORY = 0x20,

	// 4.6 Z-Wave API Firmware Update
	ZW_API_FIRMWARE_UPDATE = 0x3E,

	// 4.7 Z-Wave API Backup and Restore
	ZW_API_NVM_OPERATIONS = 0x2E,
	ZW_API_NETWORK_RESTORE = 0x2F,
	ZW_API_EXT_NVM_OPERATIONS = 0x3D,

	// 4.8 Unsolicited Z-Wave API commands
	ZW_API_APPLICATION_UPDATE = 0x49,
	ZW_API_STARTED = 0x0A,
	ZW_API_BRIDGE_APPLICATION_COMMAND_HANDLER = 0xA8,
	ZW_API_PROMISCUOUS_APPLICATION_COMMAND_HANDLER = 0xD1,
	ZW_API_APPLICATION_COMMAND_HANDLER = 0x04,

	// 4.9 Other Host API commands (from Commands1.json)
	ZW_API_CLEAR_TX_TIMERS = 0x37,
	ZW_API_GET_TX_TIMER = 0x38,
	ZW_API_GET_BACKGROUND_RSSI = 0x3B,
	ZW_API_SET_LBT_THRESHOLD = 0x3C,
	ZW_API_SET_RF_RECEIVE_MODE = 0x10,
	ZW_API_SET_RF_POWER_LEVEL = 0x17,
	ZW_API_SET_MAX_ROUTING_ATTEMPTS = 0xD4,
	ZW_API_SET_RF_POWERLEVEL_REDISCOVERY = 0x1E,
	ZW_API_START_WATCHDOG = 0xD2,
	ZW_API_STOP_WATCHDOG = 0xD3,
	ZW_API_SET_TIMEOUTS = 0x06,
	ZW_API_PM_STAY_AWAKE = 0xD7,
	ZW_API_PM_CANCEL = 0xD8,
	ZW_API_INITIATE_SHUTDOWN = 0xD9,
	ZW_API_RADIO_DEBUG_GET_PROTOCOL_LIST = 0xE6,
	ZW_API_RADIO_DEBUG_ENABLE = 0xE7,
	ZW_API_RADIO_DEBUG_STATUS = 0xE8,
	ZW_API_NONCE_GENERATION_SET_MODE = 0xEB,
	ZW_API_NONCE_UPDATE = 0xEB,
	ZW_API_GET_VIRTUAL_NODES = 0xA5,
	ZW_API_GET_PROTOCOL_STATUS = 0xBF,
	ZW_API_IS_VIRTUAL_NODE = 0xA6,

	// 4.10 Send Data commands (from Commands1.json)
	ZW_API_CONTROLLER_SEND_DATA = 0x13,
	ZW_API_CONTROLLER_SEND_DATA_MULTICAST = 0x14,
	ZW_API_END_NODE_SEND_DATA = 0x0E,
	ZW_API_END_NODE_SEND_DATA_MULTICAST = 0x0F,
	ZW_API_BRIDGE_SEND_DATA = 0xA9,
	ZW_API_BRIDGE_SEND_DATA_MULTICAST = 0xAB,
	ZW_API_SEND_DATA_ABORT = 0x16,
	ZW_API_SEND_TEST_FRAME = 0xBE,
	ZW_API_CONTROLLER_SEND_PROTOCOL_DATA = 0xAC,

	// 4.11 Security related commands (from Commands1.json)
	ZW_API_SECURITY_SETUP = 0x9C,
	ZW_API_ENCRYPT_DATA_AES = 0x67,
	ZW_API_REQUEST_PROTOCOL_CC_ENCRYPTION = 0x6C,
};

// ===============================================================
// Setup Sub‑Commands (0x01 → 0x80)
// ===============================================================
enum class eSetupSubCommand : uint8_t
{
	GetSupportedCommands = 0x01,
	SetTxStatusReport = 0x02,
	SetMaxLongRangeTxPower = 0x03,
	SetPowerlevel = 0x04,
	GetMaxLongRangeTxPower = 0x05,
	SetLongRangeMaxNodeId = 0x06,
	GetLongRangeMaxNodeId = 0x07,
	GetPowerlevel = 0x08,
	GetMaxPayloadSize = 0x10,
	GetLongRangeMaxPayloadSize = 0x11,
	SetPowerlevel16 = 0x12,
	GetPowerlevel16 = 0x13,
	GetSupportedRegions = 0x15,
	GetRegionInfo = 0x16,
	GetRFRegion = 0x20,
	SetRFRegion = 0x40,
	SetNodeIdBaseType = 0x80,
};

// ===============================================================
// Legacy command descriptor used by existing code
// ===============================================================
struct APICommand
{
	eCommandIds CmdId{};
	eFlowType Flow{ eFlowType::Unknown };
#ifndef NDEBUG
	std::string Name{};
	std::string Title{};
#endif // DEBUG

};

void TestAPICommands();

// Legacy lookup table indexed by function id.
extern APICommand APICommands[256];

enum class ApplicationUpdateEvent : uint8_t
{
	UPDATE_STATE_NODE_INFO_RECEIVED = 0x81, // Legacy NIF: [81][NodeID][Len][Info...]
	UPDATE_STATE_NODE_INFO_REQ_DONE = 0x82, // RequestNodeInfo ACK
	UPDATE_STATE_NODE_INFO_REQ_FAILED = 0x83, // RequestNodeInfo NAK
	UPDATE_STATE_NODE_ADDED = 0x84, // Node added (legacy format)
	UPDATE_STATE_NODE_REMOVED = 0x85  // Node removed
};

enum class ApplicationUpdateEvent500 : uint8_t
{
	UPDATE_STATE_SUC_ID = 0x10, // SIS Node ID updated
	UPDATE_STATE_DELETE_DONE = 0x20, // Node deleted from network
	UPDATE_STATE_NEW_ID_ASSIGNED = 0x40, // New node added to network
	UPDATE_STATE_ROUTING_PENDING = 0x80, // Neighbor discovery requested
	UPDATE_STATE_NODE_INFO_REQ_FAILED = 0x81, // RequestNodeInformation not acknowledged
	UPDATE_STATE_NODE_INFO_REQ_DONE = 0x82, // RequestNodeInformation acknowledged
	UPDATE_STATE_NOP_POWER_RECEIVED = 0x83, // NOP Power received
	UPDATE_STATE_NODE_INFO_RECEIVED = 0x84, // Full NIF (Basic/Generic/Specific/CCs)
	UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED = 0x85, // SmartStart Prime (Z-Wave)
	UPDATE_STATE_INCLUDED_NODE_INFO_RECEIVED = 0x86, // SmartStart Included Node Info (ZW or ZWLR)
	UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED_LR = 0x87 // SmartStart Prime (Z-Wave LR)
};

inline std::string ToString(ApplicationUpdateEvent event)
{
	switch (event)
	{
	case ApplicationUpdateEvent::UPDATE_STATE_NODE_INFO_RECEIVED:
		return "UPDATE_STATE_NODE_INFO_RECEIVED"; // 0x81

	case ApplicationUpdateEvent::UPDATE_STATE_NODE_INFO_REQ_DONE:
		return "UPDATE_STATE_NODE_INFO_REQ_DONE"; // 0x82

	case ApplicationUpdateEvent::UPDATE_STATE_NODE_INFO_REQ_FAILED:
		return "UPDATE_STATE_NODE_INFO_REQ_FAILED"; // 0x83

	case ApplicationUpdateEvent::UPDATE_STATE_NODE_ADDED:
		return "UPDATE_STATE_NODE_ADDED"; // 0x84

	case ApplicationUpdateEvent::UPDATE_STATE_NODE_REMOVED:
		return "UPDATE_STATE_NODE_REMOVED"; // 0x85

	default:
		return "UNKNOWN_300_EVENT";
	}
}

inline std::string ToString(ApplicationUpdateEvent500 event)
{
	switch (event)
	{
	case ApplicationUpdateEvent500::UPDATE_STATE_SUC_ID:
		return "UPDATE_STATE_SUC_ID"; // 0x10

	case ApplicationUpdateEvent500::UPDATE_STATE_DELETE_DONE:
		return "UPDATE_STATE_DELETE_DONE"; // 0x20

	case ApplicationUpdateEvent500::UPDATE_STATE_NEW_ID_ASSIGNED:
		return "UPDATE_STATE_NEW_ID_ASSIGNED"; // 0x40

	case ApplicationUpdateEvent500::UPDATE_STATE_ROUTING_PENDING:
		return "UPDATE_STATE_ROUTING_PENDING"; // 0x80

	case ApplicationUpdateEvent500::UPDATE_STATE_NODE_INFO_REQ_FAILED:
		return "UPDATE_STATE_NODE_INFO_REQ_FAILED"; // 0x81

	case ApplicationUpdateEvent500::UPDATE_STATE_NODE_INFO_REQ_DONE:
		return "UPDATE_STATE_NODE_INFO_REQ_DONE"; // 0x82

	case ApplicationUpdateEvent500::UPDATE_STATE_NOP_POWER_RECEIVED:
		return "UPDATE_STATE_NOP_POWER_RECEIVED"; // 0x83

	case ApplicationUpdateEvent500::UPDATE_STATE_NODE_INFO_RECEIVED:
		return "UPDATE_STATE_NODE_INFO_RECEIVED"; // 0x84

	case ApplicationUpdateEvent500::UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED:
		return "UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED"; // 0x85

	case ApplicationUpdateEvent500::UPDATE_STATE_INCLUDED_NODE_INFO_RECEIVED:
		return "UPDATE_STATE_INCLUDED_NODE_INFO_RECEIVED"; // 0x86

	case ApplicationUpdateEvent500::UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED_LR:
		return "UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED_LR"; // 0x87

	default:
		return "UNKNOWN_500_EVENT";
	}
}

inline std::string ToString(eCommandIds command)
{
#ifndef NDEBUG
	const auto& api = APICommands[static_cast<uint8_t>(command)];
	if (!api.Name.empty())
		return api.Name;
#endif
	return "0x" + FormatCompat::HexValue(static_cast<uint8_t>(command));
}
