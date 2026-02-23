#pragma once

#include "Module.h"
#include "Interface.h"

class ZW_InitializeManager
{
public:
	ZW_InitializeManager(EnqueueFn enqueue, ZW_Module& module)
		: enqueue(enqueue), module(module)
	{}

	void Reset()
	{
		module.InitializationState = ZW_Module::eInitializationState::NotInitialized;
		nextCommand = eCommandIds::FUNC_ID_GET_INIT_DATA;
	}

	void Restart() { Start(); } // or restart
	void Start() // or restart
	{
		if (module.InitializationState == ZW_Module::eInitializationState::NotInitialized)
		{
			nextCommand = eCommandIds::FUNC_ID_GET_INIT_DATA;
			Continue();
			return;
		}
		if (module.InitializationState == ZW_Module::eInitializationState::Paused)
		{
			Continue();
			return;
		}
	}

	bool HandleFrame(const ZW_APIFrame& frame)
	{
		switch (frame.APICmd.CmdId)
		{
		case eCommandIds::FUNC_ID_GET_INIT_DATA:
			DecodeInitData(frame.payload);
			module.InitializationState = ZW_Module::eInitializationState::InitDataDone;
			nextCommand = eCommandIds::FUNC_ID_GET_CAPABILITIES;
			Continue();
			return true;
		case eCommandIds::FUNC_ID_GET_CAPABILITIES:
			DecodeCapabilities(frame.payload);
			module.InitializationState = ZW_Module::eInitializationState::InitCapabilitiesDone;
			nextCommand = eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES;
			Continue();
			return true;
		case eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES:
			DecodeControllerCapabilities(frame.payload);
			module.InitializationState = ZW_Module::eInitializationState::InitControllerCapabilitiesDone;
			nextCommand = eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION;
			Continue();
			return true;
		case eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION:
			DecodeProtocolVersion(frame.payload);
			module.InitializationState = ZW_Module::eInitializationState::InitProtocolVersionDone;
			nextCommand = eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY;
			Continue();
			return true;
		case eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY:
			DecodeNetworkIdsFromMemory(frame.payload);
			module.InitializationState = ZW_Module::eInitializationState::InitNetworkIdsFromMemoryDone;
			nextCommand = eCommandIds::FUNC_ID_GET_LIBRARY_VERSION;
			Continue();
			return true;
		case eCommandIds::FUNC_ID_GET_LIBRARY_VERSION:
			DecodeLibraryVersion(frame.payload);
			module.InitializationState = ZW_Module::eInitializationState::InitLibraryVersionDone;
			nextCommand = eCommandIds::FUNC_ID_GET_LIBRARY_TYPE;
			Continue();
			return true;
		case eCommandIds::FUNC_ID_GET_LIBRARY_TYPE:
			DecodeLibraryType(frame.payload);
			module.InitializationState = ZW_Module::eInitializationState::InitLibraryTypeDone;
			nextCommand = static_cast<eCommandIds>(0);
			Continue();
			return true;
		default:
			Log.AddL(eLogTypes::INFO, MakeTag(), "<< Unhandled Initialization command: {}", frame.Info());
			return false;
		}
		return false;
	}

	bool HandleFrameTimeout(const ZW_APIFrame& frame)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "Initialization timed out: {}", frame.Info());
		switch (frame.APICmd.CmdId)
		{
		case eCommandIds::FUNC_ID_GET_INIT_DATA:
		case eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES:
		case eCommandIds::FUNC_ID_GET_CAPABILITIES:
		case eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION:
		case eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY:
		case eCommandIds::FUNC_ID_GET_LIBRARY_VERSION:
		case eCommandIds::FUNC_ID_GET_LIBRARY_TYPE:
			module.InitializationState = ZW_Module::eInitializationState::Paused;
			return true;
		default:
			Log.AddL(eLogTypes::INFO, MakeTag(), "<< Unhandled Initialization command: {}", frame.Info());
			return false;
		}
		return false;
	}


private:
	EnqueueFn enqueue;
	ZW_Module& module;
	eCommandIds nextCommand = eCommandIds::FUNC_ID_GET_INIT_DATA;
/*
	// ===============================================================
	// Init sequence
	// ===============================================================
	using InitAction = void (ZW_InitializeManager::*)();
	using InitDecodeAction = void (ZW_InitializeManager::*)(const std::vector<uint8_t>& payload);
	struct InitStep
	{
		eCommandIds Cmd;
		InitAction Action;
		InitDecodeAction DecodeAction;
		bool Mandatory;
	};

	static constexpr InitStep InitSequence[] =
	{
		// --- Mandatory Host API initialization ---
		{ eCommandIds::FUNC_ID_GET_INIT_DATA,                 &ZW_InitializeManager::GetInitData,                 &ZW_InitializeManager::DecodeInitData,	true },
		{ eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES,   &ZW_InitializeManager::GetControllerCapabilities,   true },
		{ eCommandIds::FUNC_ID_GET_CAPABILITIES,              &ZW_InitializeManager::GetCapabilities,             true },
		{ eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION,          &ZW_InitializeManager::GetProtocolVersion,          true },
		{ eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY,    &ZW_InitializeManager::GetNetworkIdsFromMemory,     true },
		{ eCommandIds::FUNC_ID_GET_LIBRARY_VERSION,           &ZW_InitializeManager::GetLibraryVersion,           true },
		{ eCommandIds::FUNC_ID_GET_LIBRARY_TYPE,              &ZW_InitializeManager::GetLibraryType,              true },

		// --- Optional but recommended for modern controllers ---
		{ eCommandIds::FUNC_ID_SETUP_ZWAVE_API,                    nullptr, false }, // TODO : implement &ZW_Initialization::GetApiSetupSupported,        false },
		{ eCommandIds::FUNC_ID_GET_MANUFACTURER_INFO,             nullptr, false }, // TODO : implement &ZW_Initialization::GetManufacturerInfo,         false },
		{ eCommandIds::GetNLSNodes,                 nullptr, false }, // TODO : implement &ZW_Initialization::GetNLSNodes,                 false },
		{ eCommandIds::GetLongRangeNodes,           nullptr, false }, // TODO : implement &ZW_Initialization::GetLongRangeNodes,           false },
		{ eCommandIds::GetLongRangeChannel,         nullptr, false }, // TODO : implement &ZW_Initialization::GetLongRangeChannel,         false },
		//		{ eCommandIds::GetRFRegion,                 nullptr, false }, // TODO : implement &ZW_Initialization::GetRFRegion,                 false },
	};
*/
	void Continue() // continue initialization
	{
		switch (nextCommand)
		{
		case eCommandIds::FUNC_ID_GET_INIT_DATA:
			module.InitializationState = ZW_Module::eInitializationState::InitDataPending;
			GetInitData();
			break;
		case eCommandIds::FUNC_ID_GET_CAPABILITIES:
			module.InitializationState = ZW_Module::eInitializationState::InitCapabilitiesPending;
			GetCapabilities();
			break;
		case eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES:
			if (module.HasAPICommand(eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES))
			{
				module.InitializationState = ZW_Module::eInitializationState::InitControllerCapabilitiesPending;
				GetControllerCapabilities();
			}
			else
			{
				nextCommand = eCommandIds::FUNC_ID_GET_CAPABILITIES;
				Continue();
			}
			break;
		case eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION:
			if (module.HasAPICommand(eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION))
			{
				module.InitializationState = ZW_Module::eInitializationState::InitProtocolVersionPending;
				GetProtocolVersion();
			}
			else
			{
				nextCommand = eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY;
				Continue();
			}
			break;
		case eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY:
			if (module.HasAPICommand(eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY))
			{
				module.InitializationState = ZW_Module::eInitializationState::InitNetworkIdsFromMemoryPending;
				GetNetworkIdsFromMemory();
			}
			else
			{
				nextCommand = eCommandIds::FUNC_ID_GET_LIBRARY_VERSION;
				Continue();
			}
			break;
		case eCommandIds::FUNC_ID_GET_LIBRARY_VERSION:
			if (module.HasAPICommand(eCommandIds::FUNC_ID_GET_LIBRARY_VERSION))
			{
				module.InitializationState = ZW_Module::eInitializationState::InitLibraryVersionPending;
				GetLibraryVersion();
			}
			else
			{
				nextCommand = eCommandIds::FUNC_ID_GET_LIBRARY_TYPE;
				Continue();
			}
			break;
		case eCommandIds::FUNC_ID_GET_LIBRARY_TYPE:
			if (module.HasAPICommand(eCommandIds::FUNC_ID_GET_LIBRARY_TYPE))
			{
				module.InitializationState = ZW_Module::eInitializationState::InitLibraryTypePending;
				GetLibraryType();
			}
			else
			{
				nextCommand = static_cast<eCommandIds>(0);
				Continue();
			}
			break;
		default:
			module.InitializationState = ZW_Module::eInitializationState::Initialized;
			break;
		}
	}

	// ------------------------------------------------------------
	// GET INIT DATA (0x02)
	// ------------------------------------------------------------
	void GetInitData();
	void DecodeInitData(const std::vector<uint8_t>& payload);

	// ------------------------------------------------------------
	// GET CONTROLLER CAPABILITIES (0x05)
	// ------------------------------------------------------------
	void GetControllerCapabilities();
	void DecodeControllerCapabilities(const std::vector<uint8_t>& payload);

	// ------------------------------------------------------------
	// GET PROTOCOL VERSION (0x09)
	// ------------------------------------------------------------
	void GetProtocolVersion();
	void DecodeProtocolVersion(const std::vector<uint8_t>& payload);

	// ------------------------------------------------------------
	// GET CAPABILITIES (0x07)
	// ------------------------------------------------------------
	void GetCapabilities();
	void DecodeCapabilities(const std::vector<uint8_t>& payload);

	// ------------------------------------------------------------
	// GET NETWORK IDS FROM MEMORY (0x20)
	// ------------------------------------------------------------
	void GetNetworkIdsFromMemory();
	void DecodeNetworkIdsFromMemory(const std::vector<uint8_t>& payload);

	// ------------------------------------------------------------
	// LIBRARY VERSION
	// ------------------------------------------------------------
	void GetLibraryVersion();
	void DecodeLibraryVersion(const std::vector<uint8_t>& payload);

	// ------------------------------------------------------------
	// LIBRARY TYPE
	// ------------------------------------------------------------
	void GetLibraryType();
	void DecodeLibraryType(const std::vector<uint8_t>& payload);
};

