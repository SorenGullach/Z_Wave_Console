#pragma once

#include "APICommands.h"

#include <string>
#include <vector>
#include <cstdint>

class ZW_Module
{
public:
	std::string ToString();

	// 4.3.11 Get Library Version (0x15)
	std::string LibraryVersion;
	uint8_t libraryType = 0;
	// Parsed from the returned version string (best-effort)
	int ProtocolMajor = 0;
	int ProtocolMinor = 0;

	// 4.3.2 Get Init Data (0x02)
	uint8_t ApiVersion = 0;
	uint8_t ApiCapabilities = 0;
	bool IsEndNode = false, HasTimerFunctions = false, IsPrimarayController = false, HasSISFunctions = false;
	uint8_t NodeListLength = 0;
	std::vector<uint8_t> NodeIds;
	uint8_t ChipType = 0;
	uint8_t ChipVersion = 0;

	// 4.3.4 Get Controller Capabilities (0x05)
	bool IsSecondaryController = false, IsOtherNetwork = false, IsSISPresent = false, IsSUCEnabled = false, IsNoNodesIncluded = false;

	// 4.3.10 Get Protocol Version (0x09)
	uint8_t ProtocolType = 0;
	uint8_t ProtocolMajorVersion = 0;
	uint8_t ProtocolMinorVersion = 0;
	uint8_t ProtocolRevisionVersion = 0;
	uint16_t AppFrameworkBuildNumber = 0;
	std::vector<uint8_t> ProtocolGitCommitHash;

	// 4.3.6 Get Capabilities (0x07)
	uint8_t AppVersion = 0;
	uint8_t AppRevision = 0;
	uint16_t ManufacturerId = 0;
	uint16_t ProductType = 0;
	uint16_t ProductId = 0;
	// Supported API command bitmask decoded into a list of command IDs.
	std::vector<uint8_t> ApiCommands;

	// 4.5 Get Network IDs From Memory (0x20)
	uint32_t HomeId = 0;
	uint16_t NodeId = 0;

	enum eInitializationState
	{
		NotInitialized,
		InitDataPending,
		InitDataDone,
		InitControllerCapabilitiesPending,
		InitControllerCapabilitiesDone,
		InitCapabilitiesPending,
		InitCapabilitiesDone,
		InitProtocolVersionPending,
		InitProtocolVersionDone,
		InitNetworkIdsFromMemoryPending,
		InitNetworkIdsFromMemoryDone,
		InitLibraryVersionPending,
		InitLibraryVersionDone,
		InitLibraryTypePending,
		InitLibraryTypeDone,
		Paused,
		Initialized
	} InitializationState = eInitializationState::NotInitialized;

	bool HasAPICommand(eCommandIds cmdId)
	{
		if(cmdId == eCommandIds::FUNC_ID_GET_INIT_DATA) // always expected to be a part of a module initialization sequence
			return true;
		if (cmdId == eCommandIds::FUNC_ID_GET_CAPABILITIES) // always expected to be a part of a module initialization sequence
			return true;

		for (auto& apicmd : ApiCommands)
		{
			if (apicmd == static_cast<uint8_t>(cmdId))
				return true;
		}
		return false;
	}
};

