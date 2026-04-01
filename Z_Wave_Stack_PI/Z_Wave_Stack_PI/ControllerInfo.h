#pragma once

#include "stdint.h"
#include <cstddef>
#include <vector>
#include <string>

#include "NodeId.h"
#include "APICommands.h"

class ControllerInfo
{
public:
	ControllerInfo();

	enum class eChipTypes : uint8_t
	{
		Unknown = 0x00,

		// Classic Z-Wave (pre-500 series)
		ZW010x = 0x01,   // ZW0102, ZW0103
		ZW020x = 0x02,   // ZW0201, ZW0202
		ZW030x = 0x03,   // ZW0301

		// 400 series
		ZW040x = 0x04,   // ZW0401, ZW0402

		// 500 series (Z-Wave Plus Gen1)
		ZW050x = 0x05,   // ZW0500, ZW0501

		// 700 series (Z-Wave Plus Gen2)
		ZW070x = 0x07,   // ZGM130, ZGM230

		// 800 series (Z-Wave Plus Gen3)
		ZW080x = 0x08    // ZGM230S, ZGM230P
	};

	// 4.3.11 Get Library Version (0x15)
 enum class LibraryType : uint8_t
	{
		Unknown = 0x00,
		StaticController = 0x01,
		BridgeController = 0x02,
		PortableController = 0x03,
		EnhancedSlave = 0x07,
		EnhancedController = 0x08
	};

	static constexpr size_t BitsPerByte = 8;
	static constexpr size_t LibraryVersionTextLength = 12;
	static constexpr size_t ProtocolGitCommitHashLength = 16;
	static constexpr size_t MinNetworkIdsPayloadSize = sizeof(uint32_t) + sizeof(uint8_t);

	std::string LibraryVersion;
    LibraryType libraryType = LibraryType::Unknown;
	// Parsed from the returned version string (best-effort)
	int ProtocolMajor = 0;
	int ProtocolMinor = 0;

	// 4.3.2 Get Init Data (0x02)
 enum ApiCapabilityFlags : uint8_t
	{
		EndNode = 0x01,
		TimerFunctions = 0x02,
		PrimaryController = 0x04,
		SISFunctions = 0x08
	};

	uint8_t ApiVersion = 0;
	uint8_t ApiCapabilities = 0;
	bool IsEndNode = false, HasTimerFunctions = false, IsPrimarayController = false, HasSISFunctions = false;
	uint8_t NodeListLength = 0;
	std::vector<node_t> NodeIds;

	eChipTypes ChipType = eChipTypes::Unknown;
	uint8_t ChipVersion = 0;

	// 4.3.4 Get Controller Capabilities (0x05)
	enum ControllerCaps : uint8_t
	{
		Secondary = 0x01,
		OtherNet = 0x02,
		SIS = 0x04,
		SUC = 0x10,
		NoNodes = 0x20
	};
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
		if (cmdId == eCommandIds::FUNC_ID_GET_INIT_DATA) // always expected to be a part of a module initialization sequence
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