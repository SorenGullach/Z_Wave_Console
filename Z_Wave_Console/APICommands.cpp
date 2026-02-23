
#include "APICommands.h"
#include <cassert>

// This file is intended to be generated from `commands_with_parents.json` / `commands.json`.
// It maps Z-Wave API Command ID (FUNC_ID) to the expected frame flow.

static const APICommand kUnknown{};

void TestAPICommands()
{
	for (int i = 0; i < 256; i++)
	{
		assert(sizeof(APICommands)/sizeof(APICommands[0]) == 256);
		APICommand cmd = APICommands[i];
		const auto idx = static_cast<eCommandIds>(static_cast<uint8_t>(i));
		const bool isUnknown = (cmd.CmdId == kUnknown.CmdId) && (cmd.Flow == kUnknown.Flow);
		assert(isUnknown || cmd.CmdId == idx);
#ifdef _DEBUG
		assert(isUnknown || (!cmd.Name.empty() && !cmd.Title.empty()));
#endif
	}
}

APICommand APICommands[256] =
{
	// 0x00-0x01
	kUnknown,
	kUnknown,

	// 0x02
	{ eCommandIds::FUNC_ID_GET_INIT_DATA, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "FUNC_ID_GET_INIT_DATA", "Get Init Data Command"
	#endif
	}, // Get Init Data Command
	// 0x03
	{ eCommandIds::FUNC_ID_SET_APPLICATION_NODE_INFORMATION, eFlowType::AckOnly
	#ifdef _DEBUG
	, "FUNC_ID_SET_APPLICATION_NODE_INFORMATION", "Set Application Node Information Command"
	#endif
	}, // Set Application Node Information Command
	// 0x04
	{ eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER, eFlowType::Unsolicited
	#ifdef _DEBUG
	, "ZW_API_APPLICATION_COMMAND_HANDLER", "Application Command Handler Command"
	#endif
	}, // Application Command Handler Command
	// 0x05
	{ eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "FUNC_ID_GET_CONTROLLER_CAPABILITIES", "Get Controller Capabilities Command"
	#endif
	}, // Get Controller Capabilities Command
	// 0x06
	{ eCommandIds::ZW_API_SET_TIMEOUTS, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_SET_TIMEOUTS", "Set Timeouts Command"
	#endif
	}, // Set Timeouts Command
	// 0x07
	{ eCommandIds::FUNC_ID_GET_CAPABILITIES, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "FUNC_ID_GET_CAPABILITIES", "Get Capabilities Command"
	#endif
	}, // Get Capabilities Command
	// 0x08
	{ eCommandIds::FUNC_ID_SOFT_RESET, eFlowType::AckOnly
	#ifdef _DEBUG
	, "FUNC_ID_SOFT_RESET", "Soft Reset Command"
	#endif
	}, // Soft Reset
	// 0x09
	{ eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "FUNC_ID_GET_PROTOCOL_VERSION", "Get Protocol Version Command"
	#endif
	}, // Get Protocol Version Command
	// 0x0A
	{ eCommandIds::ZW_API_STARTED, eFlowType::Unsolicited
	#ifdef _DEBUG
	, "ZW_API_STARTED", "Z-Wave API Started Command"
	#endif
	}, // Z-Wave API Started Command
	// 0x0B
	{ eCommandIds::FUNC_ID_SETUP_ZWAVE_API, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "FUNC_ID_SETUP_ZWAVE_API", "Setup Z-Wave API Command"
	#endif
	}, // Setup Z-Wave API Command
	// 0x0C
	{ eCommandIds::FUNC_ID_SET_APPLICATION_NODE_INFORMATION_COMMAND_CLASSES, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "FUNC_ID_SET_APPLICATION_NODE_INFORMATION_COMMAND_CLASSES", "Set Application Node Information Command Classes Command"
	#endif
	}, // Set Application Node Information Command Classes Command

	// 0x0D
	kUnknown,
	// 0x0E
	{ eCommandIds::ZW_API_END_NODE_SEND_DATA, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_END_NODE_SEND_DATA", "End Node Send Data Command"
	#endif
	}, // End Node Send Data Command
	// 0x0F
	{ eCommandIds::ZW_API_END_NODE_SEND_DATA_MULTICAST, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_END_NODE_SEND_DATA_MULTICAST", "End Node Send Data Multicast Command"
	#endif
	}, // End Node Send Data Multicast Command
	// 0x10
	{ eCommandIds::ZW_API_SET_RF_RECEIVE_MODE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_SET_RF_RECEIVE_MODE", "Set RF Receive Mode Command"
	#endif
	}, // Set RF Receive Mode Command
	// 0x11
	kUnknown,
	// 0x12
	{ eCommandIds::ZW_API_SEND_NODE_INFORMATION, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_SEND_NODE_INFORMATION", "Send Node Information Command"
	#endif
	}, // Send Node Information Command
	// 0x13
	{ eCommandIds::ZW_API_CONTROLLER_SEND_DATA, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_CONTROLLER_SEND_DATA", "Controller Node Send Data Command"
	#endif
	}, // Controller Node Send Data Command
	// 0x14
	{ eCommandIds::ZW_API_CONTROLLER_SEND_DATA_MULTICAST, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_CONTROLLER_SEND_DATA_MULTICAST", "Controller Node Send Data Multicast Command"
	#endif
	}, // Controller Node Send Data Multicast Command
	// 0x15
	{ eCommandIds::FUNC_ID_GET_LIBRARY_VERSION, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "FUNC_ID_GET_LIBRARY_VERSION", "Get Library Version Command"
	#endif
	}, // Get Library Version Command

	// 0x16
	{ eCommandIds::ZW_API_SEND_DATA_ABORT, eFlowType::AckOnly
	#ifdef _DEBUG
	, "ZW_API_SEND_DATA_ABORT", "Send Data Abort Command"
	#endif
	}, // Send Data Abort Command
	// 0x17
	{ eCommandIds::ZW_API_SET_RF_POWER_LEVEL, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_SET_RF_POWER_LEVEL", "Set RF Power Level Command"
	#endif
	}, // Set RF Power Level Command
	// 0x18-0x1F
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown,
	// 0x20
	{ eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_NETWORK_IDS_FROM_MEMORY", "Get Network IDs from Memory Command"
	#endif
	}, // Get Network IDs from Memory Command
	// 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown,
	// 0x2E
	{ eCommandIds::ZW_API_NVM_OPERATIONS, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_NVM_OPERATIONS", "NVM Operations Command"
	#endif
	}, // NVM Operations Command
	// 0x2F
	{ eCommandIds::ZW_API_NETWORK_RESTORE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_NETWORK_RESTORE", "Network Restore Command"
	#endif
	}, // Network Restore Command

	// 0x30-0x36
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown,
	// 0x37
	{ eCommandIds::ZW_API_CLEAR_TX_TIMERS, eFlowType::AckOnly
	#ifdef _DEBUG
	, "ZW_API_CLEAR_TX_TIMERS", "Clear Tx Timers Command"
	#endif
	}, // Clear Tx Timers Command
	// 0x38
	{ eCommandIds::ZW_API_GET_TX_TIMER, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_TX_TIMER", "Get Tx Timer Command"
	#endif
	}, // Get Tx Timer Command
	// 0x39
	{ eCommandIds::ZW_API_CLEAR_NETWORK_STATISTICS, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_CLEAR_NETWORK_STATISTICS", "Clear Network Statistics Command"
	#endif
	}, // Clear Network Statistics Command
	// 0x3A
	{ eCommandIds::ZW_API_GET_NETWORK_STATISTICS, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_NETWORK_STATISTICS", "Get Network Statistics Command"
	#endif
	}, // Get Network Statistics Command
	// 0x3B
	{ eCommandIds::ZW_API_GET_BACKGROUND_RSSI, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_BACKGROUND_RSSI", "Get Background RSSI Command"
	#endif
	}, // Get Background RSSI Command
	// 0x3C
	{ eCommandIds::ZW_API_SET_LBT_THRESHOLD, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_SET_LBT_THRESHOLD", "Set Listen Before Talk Threshold Command"
	#endif
	}, // Set Listen Before Talk Threshold Command
	// 0x3D
	{ eCommandIds::ZW_API_EXT_NVM_OPERATIONS, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_EXT_NVM_OPERATIONS", "Extended NVM Operations Command"
	#endif
	}, // Extended NVM Operations Command
	// 0x3E
	{ eCommandIds::ZW_API_FIRMWARE_UPDATE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_FIRMWARE_UPDATE", "Firmware Update Command"
	#endif
	}, // Firmware Update Command
	// 0x3F
	{ eCommandIds::ZW_API_REMOVE_SPECIFIC_NODE, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_REMOVE_SPECIFIC_NODE", "Remove Specific Node From Network Command"
	#endif
	}, // Remove Specific Node From Network Command
	// 0x40
	kUnknown,
	// 0x41
	{ eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_NODE_INFO_PROTOCOL_DATA", "Get Node Information Protocol Data Command"
	#endif
	}, // Get Node Information Protocol Data Command
	// 0x42
	{ eCommandIds::FUNC_ID_SET_DEFAULT, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "FUNC_ID_SET_DEFAULT", "Set Default Command"
	#endif
	}, // Set Default Command

	// 0x43-0x45
	kUnknown, kUnknown, kUnknown,
	// 0x46
	{ eCommandIds::ZW_API_ASSIGN_RETURN_ROUTE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_ASSIGN_RETURN_ROUTE", "Assign Return Route Command"
	#endif
	}, // Assign Return Route Command
	// 0x47
	{ eCommandIds::ZW_API_DELETE_RETURN_ROUTE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_DELETE_RETURN_ROUTE", "Delete Return Route Command"
	#endif
	}, // Delete Return Route Command
	// 0x48
	{ eCommandIds::ZW_API_REQUEST_NODE_NEIGHBOR_DISCOVERY, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_REQUEST_NODE_NEIGHBOR_DISCOVERY", "Request Node Neighbor Discovery Command"
	#endif
	}, // Request Node Neighbor Discovery Command
	// 0x49
	{ eCommandIds::ZW_API_APPLICATION_UPDATE, eFlowType::Unsolicited
	#ifdef _DEBUG
	, "ZW_API_APPLICATION_UPDATE", "Application Update Command"
	#endif
	}, // Application Update Command
	// 0x4A
	{ eCommandIds::ZW_API_ADD_NODE_TO_NETWORK, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_ADD_NODE_TO_NETWORK", "Add Node To Network Command"
	#endif
	}, // Add Node To Network Command
	// 0x4B
	{ eCommandIds::ZW_API_REMOVE_NODE_FROM_NETWORK, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_REMOVE_NODE_FROM_NETWORK", "Remove Node From Network Command"
	#endif
	}, // Remove Node From Network Command
	// 0x4C
	{ eCommandIds::ZW_API_ADD_CONTROLLER_AND_ASSIGN_PRIMARY, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_ADD_CONTROLLER_AND_ASSIGN_PRIMARY", "Add Controller And Assign Primary Controller Role Command"
	#endif
	}, // Add Controller And Assign Primary Controller Role Command
	// 0x4D
	{ eCommandIds::ZW_API_ADD_PRIMARY_CONTROLLER, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_ADD_PRIMARY_CONTROLLER", "Add Primary Controller Command"
	#endif
	}, // Add Primary Controller Command
	// 0x4E
	kUnknown,
	// 0x4F
	{ eCommandIds::ZW_API_ASSIGN_PRIORITY_RETURN_ROUTE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_ASSIGN_PRIORITY_RETURN_ROUTE", "Assign Priority Return Route Command"
	#endif
	}, // Assign Priority Return Route Command
	// 0x50
	{ eCommandIds::ZW_API_SET_LEARN_MODE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_SET_LEARN_MODE", "Set Learn Mode Command"
	#endif
	}, // Set Learn Mode Command
	// 0x51
	{ eCommandIds::ZW_API_ASSIGN_SUC_RETURN_ROUTE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_ASSIGN_SUC_RETURN_ROUTE", "Assign SUC Return Route Command"
	#endif
	}, // Assign SUC Return Route Command
	// 0x52
	kUnknown,
	// 0x53
	{ eCommandIds::ZW_API_REQUEST_NETWORK_UPDATE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_REQUEST_NETWORK_UPDATE", "Request Network Update Command"
	#endif
	}, // Request Network Update Command
	// 0x54
	{ eCommandIds::ZW_API_SET_SUC_NODE_ID, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_SET_SUC_NODE_ID", "Set SUC NodeID Command"
	#endif
	}, // Set SUC NodeID Command
	// 0x55
	{ eCommandIds::ZW_API_DELETE_SUC_RETURN_ROUTE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_DELETE_SUC_RETURN_ROUTE", "Delete SUC Return Route Command"
	#endif
	}, // Delete SUC Return Route Command
	// 0x56
	{ eCommandIds::ZW_API_GET_SUC_NODE_ID, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_SUC_NODE_ID", "Get SUC NodeID Command"
	#endif
	}, // Get SUC NodeID Command
	// 0x57
	{ eCommandIds::ZW_API_SEND_SUC_NODE_ID, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_SEND_SUC_NODE_ID", "Send SUC NodeID Command"
	#endif
	}, // Send SUC NodeID Command
	// 0x58
	{ eCommandIds::ZW_API_ASSIGN_PRIORITY_SUC_RETURN_ROUTE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_ASSIGN_PRIORITY_SUC_RETURN_ROUTE", "Assign Priority SUC Return Route Command"
	#endif
	}, // Assign Priority SUC Return Route Command

	// 0x59-0x5B
	kUnknown, kUnknown, kUnknown,
	// 0x5C
	{ eCommandIds::ZW_API_REQUEST_NEW_ROUTE_DESTINATIONS, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_REQUEST_NEW_ROUTE_DESTINATIONS", "Request New Route Destinations Command"
	#endif
	}, // Request New Route Destinations Command
	// 0x5D
	{ eCommandIds::ZW_API_IS_NODE_WITHIN_DIRECT_RANGE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_IS_NODE_WITHIN_DIRECT_RANGE", "Is Node Within Direct Range Command"
	#endif
	}, // Is Node Within Direct Range Command
	// 0x5E
	{ eCommandIds::ZW_API_EXPLORE_REQUEST_INCLUSION, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_EXPLORE_REQUEST_INCLUSION", "Explore Request Inclusion Command"
	#endif
	}, // Explore Request Inclusion Command
	// 0x5F
	{ eCommandIds::ZW_API_EXPLORE_REQUEST_EXCLUSION, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_EXPLORE_REQUEST_EXCLUSION", "Explore Request Exclusion Command"
	#endif
	}, // Explore Request Exclusion Command
	// 0x60
	{ eCommandIds::ZW_API_REQUEST_NODE_INFORMATION, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_REQUEST_NODE_INFORMATION", "Request Node Information Command"
	#endif
	}, // Request Node Information Command
	// 0x61
	{ eCommandIds::ZW_API_REMOVE_FAILED_NODE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_REMOVE_FAILED_NODE", "Remove Failed Node Command"
	#endif
	}, // Remove Failed Node Command
	// 0x62
	{ eCommandIds::ZW_API_IS_NODE_FAILED, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_IS_NODE_FAILED", "Is Node Failed Command"
	#endif
	}, // Is Node Failed Command
	// 0x63
	{ eCommandIds::ZW_API_REPLACE_FAILED_NODE, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_REPLACE_FAILED_NODE", "Replace Failed Node Command"
	#endif
	}, // Replace Failed Node Command

	// 0x64-0x66
	kUnknown, kUnknown, kUnknown,
	// 0x67
	{ eCommandIds::ZW_API_ENCRYPT_DATA_AES, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_ENCRYPT_DATA_AES", "Encrypt Data With AES Command"
	#endif
	}, // Encrypt Data With AES Command
	// 0x68
	{ eCommandIds::ZW_API_REQUEST_NODE_TYPE_NEIGHBOR_UPDATE, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_REQUEST_NODE_TYPE_NEIGHBOR_UPDATE", "Request Node Type Neighbor Update Command"
	#endif
	}, // Request Node Type Neighbor Update Command
	// 0x69
	{ eCommandIds::ZW_API_TRANSFER_PROTOCOL_CC, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_TRANSFER_PROTOCOL_CC", "Transfer Protocol Command Class Command"
	#endif
	}, // Transfer Protocol Command Class Command
	// 0x6A
	{ eCommandIds::ZW_API_ENABLE_NODE_NLS, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_ENABLE_NODE_NLS", "Enable Node NLS Command"
	#endif
	}, // Enable Node NLS Command
	// 0x6B
	{ eCommandIds::ZW_API_GET_NODE_NLS_STATE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_NODE_NLS_STATE", "Get Node NLS State Command"
	#endif
	}, // Get Node NLS State Command
	// 0x6C
	{ eCommandIds::ZW_API_REQUEST_PROTOCOL_CC_ENCRYPTION, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_REQUEST_PROTOCOL_CC_ENCRYPTION", "Request Protocol Command Class Encryption Command"
	#endif
	}, // Request Protocol Command Class Encryption Command
	// 0x6D-0x6F
	kUnknown, kUnknown, kUnknown, 
	// 0x70-0x7F
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, 
	// 0x80
	{ eCommandIds::ZW_API_GET_NEIGHBOR_TABLE_LINE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_NEIGHBOR_TABLE_LINE", "Get Neighbor Table Line Command"
	#endif
	}, // Get Neighbor Table Line Command
	// 0x81-0x84
	kUnknown, kUnknown, kUnknown, kUnknown,
	// 0x85
	{ eCommandIds::ZW_API_GET_ROUTING_TABLE_ENTRIES, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_ROUTING_TABLE_ENTRIES", "Get Routing Table Entries Command"
	#endif
	}, // Get Routing Table Entries Command
	// 0x86-0x8F
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown,
	// 0x90
	{ eCommandIds::ZW_API_LOCK_UNLOCK_LAST_ROUTE, eFlowType::AckOnly
	#ifdef _DEBUG
	, "ZW_API_LOCK_UNLOCK_LAST_ROUTE", "Lock Unlock Last Route Command"
	#endif
	}, // Lock Unlock Last Route Command
	// 0x91
	kUnknown,
	// 0x92
	{ eCommandIds::ZW_API_GET_PRIORITY_ROUTE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_PRIORITY_ROUTE", "Get Priority Route Command"
	#endif
	}, // Get Priority Route Command
	// 0x93
	{ eCommandIds::ZW_API_SET_PRIORITY_ROUTE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_SET_PRIORITY_ROUTE", "Set Priority Route Command"
	#endif
	}, // Set Priority Route Command
	// 0x94-0x9B
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown,
	// 0x9C
	{ eCommandIds::ZW_API_SECURITY_SETUP, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_SECURITY_SETUP", "Security Setup Command"
	#endif
	}, // Security Setup Command
	// 0x9D-0x9F
	kUnknown, kUnknown, kUnknown,
	// 0xA0
	{ eCommandIds::ZW_API_SET_VIRTUAL_NODE_APP_INFO, eFlowType::AckOnly
	#ifdef _DEBUG
	, "ZW_API_SET_VIRTUAL_NODE_APP_INFO", "Set Virtual Nodes Application Node Information Command"
	#endif
	}, // Set Virtual Nodes Application Node Information Command
	// 0xA1
	kUnknown,
	// 0xA2
	{ eCommandIds::ZW_API_VIRTUAL_NODE_SEND_NODE_INFO, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_VIRTUAL_NODE_SEND_NODE_INFO", "Virtual Node Send Node Information Command"
	#endif
	}, // Virtual Node Send Node Information Command
	// 0xA3
	kUnknown,
	// 0xA4
	{ eCommandIds::ZW_API_SET_VIRTUAL_NODE_LEARN_MODE, eFlowType::AckWithCallback
	#ifdef _DEBUG
	, "ZW_API_SET_VIRTUAL_NODE_LEARN_MODE", "Set Virtual Node To Learn Mode Command"
	#endif
	}, // Set Virtual Node To Learn Mode Command
	// 0xA5
	{ eCommandIds::ZW_API_GET_VIRTUAL_NODES, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_VIRTUAL_NODES", "Get Virtual Nodes Command"
	#endif
	}, // Get Virtual Nodes Command
	// 0xA6
	{ eCommandIds::ZW_API_IS_VIRTUAL_NODE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_IS_VIRTUAL_NODE", "Is Virtual Node Command"
	#endif
	}, // Is Virtual Node Command
	// 0xA7
	kUnknown,
	// 0xA8
	{ eCommandIds::ZW_API_BRIDGE_APPLICATION_COMMAND_HANDLER, eFlowType::Unsolicited
	#ifdef _DEBUG
	, "ZW_API_BRIDGE_APPLICATION_COMMAND_HANDLER", "Bridge Application Command Handler Command"
	#endif
	}, // Bridge Application Command Handler Command
	// 0xA9
	{ eCommandIds::ZW_API_BRIDGE_SEND_DATA, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_BRIDGE_SEND_DATA", "Bridge Controller Node Send Data Command"
	#endif
	}, // Bridge Controller Node Send Data Command
	// 0xAA
	kUnknown,
	// 0xAB
	{ eCommandIds::ZW_API_BRIDGE_SEND_DATA_MULTICAST, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_BRIDGE_SEND_DATA_MULTICAST", "Bridge Controller Node Send Data Multicast Command"
	#endif
	}, // Bridge Controller Node Send Data Multicast Command
	// 0xAC
	{ eCommandIds::ZW_API_CONTROLLER_SEND_PROTOCOL_DATA, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_CONTROLLER_SEND_PROTOCOL_DATA", "Controller Node Send Protocol Data Command"
	#endif
	}, // Controller Node Send Protocol Data Command
	// 0xAD-0xAF
	kUnknown, kUnknown, kUnknown, 
	// 0xB0-0xBC
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown,
	// 0xBD
	{ eCommandIds::FUNC_ID_GET_LIBRARY_TYPE, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "FUNC_ID_GET_LIBRARY_TYPE", "Get Library Type Command"
	#endif
	}, // Get Library Type Command
	// 0xBE
	{ eCommandIds::ZW_API_SEND_TEST_FRAME, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_SEND_TEST_FRAME", "Send Test Frame Command"
	#endif
	}, // Send Test Frame Command
	// 0xBF
	{ eCommandIds::ZW_API_GET_PROTOCOL_STATUS, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_GET_PROTOCOL_STATUS", "Get Z-Wave Module Protocol Status Command"
	#endif
	}, // Get Z-Wave Module Protocol Status Command
	// 0xC0
	{ eCommandIds::GetNLSNodes, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "GetNLSNodes", "Get NLS Nodes Command"
	#endif
	}, // Get NLS Nodes Command
	// 0xC1-0xCF
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown,
	// 0xD0
	kUnknown,
	// 0xD1
	{ eCommandIds::ZW_API_PROMISCUOUS_APPLICATION_COMMAND_HANDLER, eFlowType::Unsolicited
	#ifdef _DEBUG
	, "ZW_API_PROMISCUOUS_APPLICATION_COMMAND_HANDLER", "Promiscuous Application Command Handler Command"
	#endif
	}, // Promiscuous Application Command Handler Command
	// 0xD2
	{ eCommandIds::ZW_API_START_WATCHDOG, eFlowType::Unacknowledged
	#ifdef _DEBUG
	, "ZW_API_START_WATCHDOG", "Start Watchdog Command"
	#endif
	}, // Start Watchdog Command
	// 0xD3
	{ eCommandIds::ZW_API_STOP_WATCHDOG, eFlowType::Unacknowledged
	#ifdef _DEBUG
	, "ZW_API_STOP_WATCHDOG", "Stop Watchdog Command"
	#endif
	}, // Stop Watchdog Command
	// 0xD4
	{ eCommandIds::ZW_API_SET_MAX_ROUTING_ATTEMPTS, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_SET_MAX_ROUTING_ATTEMPTS", "Set Maximum Routing Attempts Command"
	#endif
	}, // Set Maximum Routing Attempts Command
	// 0xD5
	kUnknown,
	// 0xD6
	{ eCommandIds::ZW_API_SET_SMARTSTART_INCLUSION_INTERVAL, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_SET_SMARTSTART_INCLUSION_INTERVAL", "Set SmartStart Inclusion Request Maximum Interval Command"
	#endif
	}, // Set SmartStart Inclusion Request Maximum Interval Command
	// 0xD7
	{ eCommandIds::ZW_API_PM_STAY_AWAKE, eFlowType::AckOnly
	#ifdef _DEBUG
	, "ZW_API_PM_STAY_AWAKE", "Power Management Stay Awake Command"
	#endif
	}, // Power Management Stay Awake Command
	// 0xD8
	{ eCommandIds::ZW_API_PM_CANCEL, eFlowType::AckOnly
	#ifdef _DEBUG
	, "ZW_API_PM_CANCEL", "Power Management Cancel Command"
	#endif
	}, // Power Management Cancel Command
	// 0xD9
	{ eCommandIds::ZW_API_INITIATE_SHUTDOWN, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_INITIATE_SHUTDOWN", "Initiate Shutdown Command"
	#endif
	}, // Initiate Shutdown Command
	// 0xDA
	{ eCommandIds::GetLongRangeNodes, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "GetLongRangeNodes", "Get Long Range Nodes Command"
	#endif
	}, // Get Long Range Nodes Command
	// 0xDB
	{ eCommandIds::GetLongRangeChannel, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "GetLongRangeChannel", "Get Z-Wave Long Range Channel Command"
	#endif
	}, // Get Z-Wave Long Range Channel Command
	// 0xDC
	{ eCommandIds::SetLongRangeChannel, eFlowType::AckOnly
	#ifdef _DEBUG
	, "SetLongRangeChannel", "Set Z-Wave Long Range Channel Command"
	#endif
	}, // Set Z-Wave Long Range Channel Command
	// 0xDD
	{ eCommandIds::ZW_API_SET_LR_SHADOW_NODEIDS, eFlowType::AckOnly
	#ifdef _DEBUG
	, "ZW_API_SET_LR_SHADOW_NODEIDS", "Set Z-Wave Long Range Shadow NodeIDs Command"
	#endif
	}, // Set Z-Wave Long Range Shadow NodeIDs Command
	// 0xDE-0xDF
	kUnknown, kUnknown, 
	// 0xE0-0xE5
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, 
	// 0xE6
	{ eCommandIds::ZW_API_RADIO_DEBUG_GET_PROTOCOL_LIST, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_RADIO_DEBUG_GET_PROTOCOL_LIST", "Radio Debug Get Protocol List Command"
	#endif
	}, // Radio Debug Get Protocol List Command
	// 0xE7
	{ eCommandIds::ZW_API_RADIO_DEBUG_ENABLE, eFlowType::AckOnly
	#ifdef _DEBUG
	, "ZW_API_RADIO_DEBUG_ENABLE", "Radio Debug Enable Command"
	#endif
	}, // Radio Debug Enable Command
	// 0xE8
	{ eCommandIds::ZW_API_RADIO_DEBUG_STATUS, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "ZW_API_RADIO_DEBUG_STATUS", "Radio Debug Status Command"
	#endif
	}, // Radio Debug Status Command
	// 0xE9
	{ eCommandIds::ZW_API_SEND_NOP, eFlowType::AckWithResponseCallback
	#ifdef _DEBUG
	, "ZW_API_SEND_NOP", "Send NOP Command"
	#endif
	}, // Send NOP Command
	// 0xEA
	{ eCommandIds::FUNC_ID_GET_MANUFACTURER_INFO, eFlowType::AckWithResponse
	#ifdef _DEBUG
	, "FUNC_ID_GET_MANUFACTURER_INFO", "Get Manufacturer Info Command"
	#endif
	}, // Get Manufacturer Info Command
	// 0xEB
	{ eCommandIds::ZW_API_NONCE_UPDATE, eFlowType::Unsolicited
	#ifdef _DEBUG
	, "ZW_API_NONCE_UPDATE", "Nonce Update Command / Nonce Generation Set Mode"
	#endif
	}, // Nonce Update Command / Nonce Generation Set Mode
	// 0xEC-0xEF
	kUnknown, kUnknown, kUnknown,
	// 0xF0-0xFF
	kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown
};

