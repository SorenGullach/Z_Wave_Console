#pragma once

#include "APIFrame.h"
#include "ControllerInfo.h"

class Initialize
{
public:
  Initialize(EnqueueFn enqueue, ControllerInfo& controllerinfo);

	void Start();
	
	bool HandleFrame(const ZW_APIFrame& frame);
	bool HandleFrameTimeout(const ZW_APIFrame& frame);

    private:
	EnqueueFn enqueue;
	ControllerInfo& controllerInfo;
	int currentStep;

	void Continue(); // continue initialization

	// ===============================================================
	// Init sequence (table-driven)
	// ===============================================================
	using InitAction = void (Initialize::*)();
	using InitDecodeAction = void (Initialize::*)(const APIFrame::PayLoad& payload);

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
	void DecodeInitData(const APIFrame::PayLoad& payload);

	// ------------------------------------------------------------
	// GET CONTROLLER CAPABILITIES (0x05)
	// ------------------------------------------------------------
	void GetControllerCapabilities();
	void DecodeControllerCapabilities(const APIFrame::PayLoad& payload);

	// ------------------------------------------------------------
	// GET PROTOCOL VERSION (0x09)
	// ------------------------------------------------------------
	void GetProtocolVersion();
	void DecodeProtocolVersion(const APIFrame::PayLoad& payload);

	// ------------------------------------------------------------
	// GET CAPABILITIES (0x07)
	// ------------------------------------------------------------
	void GetCapabilities();
	void DecodeCapabilities(const APIFrame::PayLoad& payload);

	// ------------------------------------------------------------
	// GET NETWORK IDS FROM MEMORY (0x20)
	// ------------------------------------------------------------
	void GetNetworkIdsFromMemory();
	void DecodeNetworkIdsFromMemory(const APIFrame::PayLoad& payload);

	// ------------------------------------------------------------
	// LIBRARY VERSION
	// ------------------------------------------------------------
	void GetLibraryVersion();
	void DecodeLibraryVersion(const APIFrame::PayLoad& payload);

	// ------------------------------------------------------------
	// LIBRARY TYPE
	// ------------------------------------------------------------
	void GetLibraryType();
	void DecodeLibraryType(const APIFrame::PayLoad& payload);

	static constexpr InitStep InitSequence[] =
	{
		{ eCommandIds::FUNC_ID_GET_INIT_DATA,
			ControllerInfo::eInitializationState::InitDataPending,                  
			ControllerInfo::eInitializationState::InitDataDone,
			&GetInitData,               
			&DecodeInitData,               
			true },
		{ eCommandIds::FUNC_ID_GET_CAPABILITIES,
			ControllerInfo::eInitializationState::InitCapabilitiesPending,           
			ControllerInfo::eInitializationState::InitCapabilitiesDone,
			&GetCapabilities,            
			&DecodeCapabilities,            
			true },
		{ eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES, 
			ControllerInfo::eInitializationState::InitControllerCapabilitiesPending,
			ControllerInfo::eInitializationState::InitControllerCapabilitiesDone, 
			&GetControllerCapabilities,
			&DecodeControllerCapabilities, 
			true },
		{ eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION,        
			ControllerInfo::eInitializationState::InitProtocolVersionPending,
			ControllerInfo::eInitializationState::InitProtocolVersionDone,        
			&GetProtocolVersion,
			&DecodeProtocolVersion,        
			false },
		{ eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY,  
			ControllerInfo::eInitializationState::InitNetworkIdsFromMemoryPending,
			ControllerInfo::eInitializationState::InitNetworkIdsFromMemoryDone,   
			&GetNetworkIdsFromMemory,
			&DecodeNetworkIdsFromMemory,   
			true },
		{ eCommandIds::FUNC_ID_GET_LIBRARY_VERSION,         
			ControllerInfo::eInitializationState::InitLibraryVersionPending,
			ControllerInfo::eInitializationState::InitLibraryVersionDone,         
			&GetLibraryVersion,
			&DecodeLibraryVersion,         
			true },
		{ eCommandIds::FUNC_ID_GET_LIBRARY_TYPE,            
			ControllerInfo::eInitializationState::InitLibraryTypePending,
			ControllerInfo::eInitializationState::InitLibraryTypeDone,            
			&GetLibraryType,
			&DecodeLibraryType,            
			false },
	};


};

