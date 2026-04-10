#pragma once

#include "APIFrame.h"
#include "ControllerInfo.h"

class Initialize
{
public:
  Initialize(EnqueueFn enqueue, ControllerInfo& controllerinfo);

	void Start();
	bool Done();
	
	bool HandleFrame(const APIFrame& frame);
	bool HandleFrameTimeout(const APIFrame& frame);

    private:
	EnqueueFn enqueue;
	ControllerInfo& controllerInfo;
    size_t currentStep;

	void Continue(); // continue initialization

	// ===============================================================
	// Init sequence (table-driven)
	// ===============================================================
	using InitAction = void (Initialize::*)();
	using InitDecodeAction = void (Initialize::*)(const payload_t& payload);

	struct InitStep
	{
		eCommandIds CmdId;
		ControllerInfo::eInitializationState PendingState;
		ControllerInfo::eInitializationState DoneState;
		InitAction Action;
		InitDecodeAction DecodeAction;
		bool Mandatory;
	};

	// ------------------------------------------------------------
// GET INIT DATA (0x02)
// ------------------------------------------------------------
	void GetInitData();
	void DecodeInitData(const payload_t& payload);

	// ------------------------------------------------------------
	// GET CONTROLLER CAPABILITIES (0x05)
	// ------------------------------------------------------------
	void GetControllerCapabilities();
	void DecodeControllerCapabilities(const payload_t& payload);

	// ------------------------------------------------------------
	// GET PROTOCOL VERSION (0x09)
	// ------------------------------------------------------------
	void GetProtocolVersion();
	void DecodeProtocolVersion(const payload_t& payload);

	// ------------------------------------------------------------
	// GET CAPABILITIES (0x07)
	// ------------------------------------------------------------
	void GetCapabilities();
	void DecodeCapabilities(const payload_t& payload);

	// ------------------------------------------------------------
	// GET NETWORK IDS FROM MEMORY (0x20)
	// ------------------------------------------------------------
	void GetNetworkIdsFromMemory();
	void DecodeNetworkIdsFromMemory(const payload_t& payload);

	// ------------------------------------------------------------
	// LIBRARY VERSION
	// ------------------------------------------------------------
	void GetLibraryVersion();
	void DecodeLibraryVersion(const payload_t& payload);

	// ------------------------------------------------------------
	// LIBRARY TYPE
	// ------------------------------------------------------------
	void GetLibraryType();
	void DecodeLibraryType(const payload_t& payload);

	static constexpr InitStep InitSequence[] =
	{
		{ eCommandIds::FUNC_ID_GET_INIT_DATA,
			ControllerInfo::eInitializationState::InitDataPending,                  
			ControllerInfo::eInitializationState::InitDataDone,
            &Initialize::GetInitData,
			&Initialize::DecodeInitData,
			true },
		{ eCommandIds::FUNC_ID_GET_CAPABILITIES,
			ControllerInfo::eInitializationState::InitCapabilitiesPending,           
			ControllerInfo::eInitializationState::InitCapabilitiesDone,
           &Initialize::GetCapabilities,
			&Initialize::DecodeCapabilities,
			true },
		{ eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES, 
			ControllerInfo::eInitializationState::InitControllerCapabilitiesPending,
			ControllerInfo::eInitializationState::InitControllerCapabilitiesDone, 
         &Initialize::GetControllerCapabilities,
			&Initialize::DecodeControllerCapabilities,
			true },
		{ eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION,        
			ControllerInfo::eInitializationState::InitProtocolVersionPending,
			ControllerInfo::eInitializationState::InitProtocolVersionDone,        
            &Initialize::GetProtocolVersion,
			&Initialize::DecodeProtocolVersion,
			false },
		{ eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY,  
			ControllerInfo::eInitializationState::InitNetworkIdsFromMemoryPending,
			ControllerInfo::eInitializationState::InitNetworkIdsFromMemoryDone,   
           &Initialize::GetNetworkIdsFromMemory,
			&Initialize::DecodeNetworkIdsFromMemory,
			true },
		{ eCommandIds::FUNC_ID_GET_LIBRARY_VERSION,         
			ControllerInfo::eInitializationState::InitLibraryVersionPending,
			ControllerInfo::eInitializationState::InitLibraryVersionDone,         
         &Initialize::GetLibraryVersion,
			&Initialize::DecodeLibraryVersion,
			true },
		{ eCommandIds::FUNC_ID_GET_LIBRARY_TYPE,            
			ControllerInfo::eInitializationState::InitLibraryTypePending,
			ControllerInfo::eInitializationState::InitLibraryTypeDone,            
            &Initialize::GetLibraryType,
			&Initialize::DecodeLibraryType,
			false },
	};


};

