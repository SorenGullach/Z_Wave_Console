#pragma once

#include "stdint.h"
#include <cstddef>
#include <vector>
#include <string>

#include "NodeId_t.h"
#include "APICommands.h"

class ControllerInfo;

class ControllerInfoFormatter
{
public:
	explicit ControllerInfoFormatter(const ControllerInfo& ci)
		: ci(ci)
	{
	}

	// Human-readable grouped fields
	std::string InitializationStateString() const;
	std::string APIVersionText() const;
	std::string ChipTypeName() const;
	std::string LibraryTypeName() const;
	std::string ProductIdName() const;
	std::string ManufacturerIdName() const;
	std::string ProductTypeName() const;

	// Full text dump
	std::string ToText() const;

	// JSON for GUI
//	std::string ToJson() const;

private:
	const ControllerInfo& ci;
};

// ----------------------- ControllerInfo -----------------------
class ControllerInfo
{
private:
	ControllerInfoFormatter formatter;
	friend class ControllerInfoFormatter;

public:
	ControllerInfo();

	std::string ToString() const 
	{
		return formatter.ToText();
	}

	static constexpr size_t BitsPerByte = 8;
	static constexpr size_t LibraryVersionTextLength = 12;
	static constexpr size_t ProtocolGitCommitHashLength = 16;
	static constexpr size_t MinNetworkIdsPayloadSize = sizeof(uint32_t) + sizeof(uint8_t);

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
	enum class eLibraryType : uint8_t
	{
		Unknown = 0x00,

		// Controllers
		StaticController = 0x01, // ZW_LIB_CONTROLLER_STATIC
		BridgeController = 0x02, // ZW_LIB_CONTROLLER
		PortableController = 0x03, // ZW_LIB_CONTROLLER_PORTABLE
		EnhancedController = 0x08, // ZW_LIB_CONTROLLER_ENHANCED

		// Slaves
		Slave = 0x04, // ZW_LIB_SLAVE
		Installer = 0x05, // ZW_LIB_INSTALLER
		RoutingSlave = 0x06, // ZW_LIB_SLAVE_ROUTING
		EnhancedSlave = 0x07, // ZW_LIB_SLAVE_ENHANCED

		// Special-purpose
		DeviceUnderTest = 0x09, // ZW_LIB_DUT
		AVRemote = 0x0A, // ZW_LIB_AVREMOTE
		AVDevice = 0x0B  // ZW_LIB_AVDEVICE
	};

	std::string LibraryVersion;
	eLibraryType libraryType = eLibraryType::Unknown;
	// Parsed from the returned version string (best-effort)
	int ProtocolMajor = 0;
	int ProtocolMinor = 0;

	// 4.3.2 Get Init Data (0x02)
	enum class eApiCapabilityFlags : uint8_t
	{
		EndNode = 1 << 0,
		TimerFunctions = 1 << 1,
		PrimaryController = 1 << 2,
		SISFunctions = 1 << 3
	};

	uint8_t ApiVersion = 0;
	uint8_t ApiCapabilities = 0;

	bool IsEndNode() const { return (ApiCapabilities & static_cast<uint8_t>(eApiCapabilityFlags::EndNode)) > 0; };
	bool HasTimerFunctions() const { return (ApiCapabilities & static_cast<uint8_t>(eApiCapabilityFlags::TimerFunctions)) > 0; };
	bool IsPrimaryController() const { return (ApiCapabilities & static_cast<uint8_t>(eApiCapabilityFlags::PrimaryController)) > 0; };
	bool HasSISFunctions() const { return (ApiCapabilities & static_cast<uint8_t>(eApiCapabilityFlags::SISFunctions)) > 0; };

	// 4.3.3 Get Node List (0x03)
	uint8_t NodeListLength = 0;
	std::vector<nodeid_t> NodeIds;

	// 4.3.1 Get Chip Type (0x01)
	eChipTypes ChipType = eChipTypes::Unknown;
	uint8_t ChipVersion = 0;

	// 4.3.4 Get Controller Capabilities (0x05)
	enum class eControllerCaps : uint8_t
	{
		Secondary = 1 << 0,
		OtherNet = 1 << 1,
		SISPresent = 1 << 2,
		SUCEnabled = 1 << 4,
		NoNodesIncluded = 1 << 5
	};
	uint8_t ControllerCapabilities = 0;
	bool IsSecondaryController() const { return (ControllerCapabilities & static_cast<uint8_t>(eControllerCaps::Secondary)) > 0; }
	bool IsOtherNetwork() const { return (ControllerCapabilities & static_cast<uint8_t>(eControllerCaps::OtherNet)) > 0; }
	bool IsSISPresent() const { return (ControllerCapabilities & static_cast<uint8_t>(eControllerCaps::SISPresent)) > 0; }
	bool IsSUCEnabled() const { return (ControllerCapabilities & static_cast<uint8_t>(eControllerCaps::SUCEnabled)) > 0; }
	bool IsNoNodesIncluded() const { return (ControllerCapabilities & static_cast<uint8_t>(eControllerCaps::NoNodesIncluded)) > 0; }

	// 4.3.10 Get Protocol Version (0x09)
	uint8_t ProtocolType = 0xFF; // Unknown
	uint8_t ProtocolMajorVersion = 0;
	uint8_t ProtocolMinorVersion = 0;
	uint8_t ProtocolRevisionVersion = 0;
	uint16_t AppFrameworkBuildNumber = 0;
	std::vector<uint8_t> ProtocolGitCommitHash;

	// 4.3.5 Get Capabilities (0x07)
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

	enum class eInitializationState
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

	bool HasAPICommand(eCommandIds cmdId) const
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

