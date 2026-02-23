#pragma once

#include "Module.h"
#include "Interface.h"

#include <cstddef>
#include <array>

class ZW_InitializeManager
{
public:
	ZW_InitializeManager(EnqueueFn enqueue, ZW_Module& module)
		: enqueue(enqueue), module(module)
	{}

	void Reset()
	{
		module.InitializationState = ZW_Module::eInitializationState::NotInitialized;
		currentStep = 0;
	}

	void StartOrResume() // or restart
	{
		if (module.InitializationState == ZW_Module::eInitializationState::NotInitialized)
		{
			Reset();
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
		const size_t count = std::size(InitSequence);
		if (currentStep >= count)
			return false; 

		const auto& expectedStep = InitSequence[currentStep];
		if (frame.APICmd.CmdId != expectedStep.CmdId)
		{
			// Late/duplicate/out-of-order frames shouldn't rewind initialization.
			Log.AddL(eLogTypes::ERR, MakeTag(), "Initialization out of order: {}", frame.Info());
			return true;
		}

		if (!expectedStep.DecodeAction)
			return false;

		(this->*expectedStep.DecodeAction)(frame.payload);
		module.InitializationState = expectedStep.DoneState;
		currentStep++;
		Continue();
		return true;
	}

	bool HandleFrameTimeout(const ZW_APIFrame& frame)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "Initialization timed out: {}", frame.Info());
		const size_t count = std::size(InitSequence);
		if (currentStep >= count || frame.APICmd.CmdId != InitSequence[currentStep].CmdId)
		{
			Log.AddL(eLogTypes::INFO, MakeTag(), "<< Unhandled Initialization command: {}", frame.Info());
			return false;
		}

		module.InitializationState = ZW_Module::eInitializationState::Paused;
		return true;
	}


private:
	EnqueueFn enqueue;
	ZW_Module& module;

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

	// ===============================================================
	// Init sequence (table-driven)
	// ===============================================================
	using InitAction = void (ZW_InitializeManager::*)();
	using InitDecodeAction = void (ZW_InitializeManager::*)(const std::vector<uint8_t>& payload);

	struct InitStep
	{
		eCommandIds CmdId;
		ZW_Module::eInitializationState PendingState;
		ZW_Module::eInitializationState DoneState;
		InitAction Action;
		InitDecodeAction DecodeAction;
		bool Mandatory;
	};

	static constexpr InitStep InitSequence[] =
	{
		{ eCommandIds::FUNC_ID_GET_INIT_DATA,
			ZW_Module::eInitializationState::InitDataPending,                  ZW_Module::eInitializationState::InitDataDone,
			&ZW_InitializeManager::GetInitData,               &ZW_InitializeManager::DecodeInitData,               true },
		{ eCommandIds::FUNC_ID_GET_CAPABILITIES,
			ZW_Module::eInitializationState::InitCapabilitiesPending,           ZW_Module::eInitializationState::InitCapabilitiesDone,
			&ZW_InitializeManager::GetCapabilities,            &ZW_InitializeManager::DecodeCapabilities,            true },
		{ eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES, ZW_Module::eInitializationState::InitControllerCapabilitiesPending,
			ZW_Module::eInitializationState::InitControllerCapabilitiesDone, &ZW_InitializeManager::GetControllerCapabilities,
			&ZW_InitializeManager::DecodeControllerCapabilities, true },
		{ eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION,        ZW_Module::eInitializationState::InitProtocolVersionPending,
			ZW_Module::eInitializationState::InitProtocolVersionDone,        &ZW_InitializeManager::GetProtocolVersion,
			&ZW_InitializeManager::DecodeProtocolVersion,        false },
		{ eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY,  ZW_Module::eInitializationState::InitNetworkIdsFromMemoryPending,
			ZW_Module::eInitializationState::InitNetworkIdsFromMemoryDone,   &ZW_InitializeManager::GetNetworkIdsFromMemory,
			&ZW_InitializeManager::DecodeNetworkIdsFromMemory,   true },
		{ eCommandIds::FUNC_ID_GET_LIBRARY_VERSION,         ZW_Module::eInitializationState::InitLibraryVersionPending,
			ZW_Module::eInitializationState::InitLibraryVersionDone,         &ZW_InitializeManager::GetLibraryVersion,
			&ZW_InitializeManager::DecodeLibraryVersion,         true },
		{ eCommandIds::FUNC_ID_GET_LIBRARY_TYPE,            ZW_Module::eInitializationState::InitLibraryTypePending,
			ZW_Module::eInitializationState::InitLibraryTypeDone,            &ZW_InitializeManager::GetLibraryType,
			&ZW_InitializeManager::DecodeLibraryType,            false },
	};

	void Continue() // continue initialization
	{
		const size_t count = std::size(InitSequence);
		while (currentStep < count)
		{
			const auto& step = InitSequence[currentStep];
			if (module.HasAPICommand(step.CmdId))
			{
				module.InitializationState = step.PendingState;
				(this->*step.Action)();
				return;
			}
			else
			{
				if (step.Mandatory)
					Log.AddL(eLogTypes::ERR, MakeTag(), "Initialization failed Mandatory: cmdId={}", APICommands[static_cast<uint8_t>(step.CmdId)].Name);
				currentStep++;
				continue;
			}
		}
		module.InitializationState = ZW_Module::eInitializationState::Initialized;
	}

	size_t currentStep = 0;
};

