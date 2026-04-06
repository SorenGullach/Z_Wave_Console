#include "ControllerInfo.h"
#include "FormatCompat.h" // for std::format, std::format_args, std::format_to, std::to_string, std::vformat, std::vformat_to, <common.h>

std::string ControllerInfoFormatter::ToText() const
{
	std::string out;
	// initialize state
	out += std::format("Initialized      : {} {}\n",
					   (uint8_t)ci.InitializationState,
					   ci.InitializationState == ControllerInfo::eInitializationState::Initialized ? "Yes" : "No");

	// API version
	out += std::format("API Version      : {}\n", APIVersionText());

	// Compact grouped fields
	out += std::format("EndNode/Timer    : {} / {}\n",
					   ci.IsEndNode() ? "Yes" : "No",
					   ci.HasTimerFunctions() ? "Yes" : "No");

	out += std::format("Primary/SIS      : {} / {}\n",
					   ci.IsPrimaryController() ? "Yes" : "No",
					   ci.HasSISFunctions() ? "Yes" : "No");

	// Node list
	out += std::format("Node List        : ({}) ", ci.NodeIds.size());
	for (auto n : ci.NodeIds)
		out += std::format("{} ", n.Value());
	out += "\n";

	// Chip DVC
	out += std::format("Chip             : Type={} {} V{}\n", (int)ci.ChipType, ChipTypeName(), ci.ChipVersion);

	// Controller capabilities
	out += std::format("SecondaryController {} \tOtherNetwork {}\n",
					   ci.IsSecondaryController() ? "Yes" : "No",
					   ci.IsOtherNetwork() ? "Yes" : "No");

	out += std::format("SISPresent {} \tSUCEnabled {} \tNoNodesIncluded {}\n",
					   ci.IsSISPresent() ? "Yes" : "No",
					   ci.IsSUCEnabled() ? "Yes" : "No",
					   ci.IsNoNodesIncluded() ? "Yes" : "No");

	// Protocol version
	if (ci.ProtocolType == 1)
		out += std::format("Protocol Version : type=0x{:02X} {}.{}.{} (AF {})\n",
						   ci.ProtocolType,
						   ci.ProtocolMajorVersion,
						   ci.ProtocolMinorVersion,
						   ci.ProtocolRevisionVersion,
						   ci.AppFrameworkBuildNumber);
	else
		out += std::format("Protocol Version : not found\n");

	// Library
	out += std::format("Library          : Version= {} \tType={} {}\n",
					   ci.LibraryVersion,
					   (uint8_t)ci.libraryType,
					   LibraryTypeName());

	// API commands
	out += std::format("API Commands     : ({}) ", ci.ApiCommands.size());
	for (auto n : ci.ApiCommands)
		out += std::format("{} ", n);
	out += "\n";

	// Capabilities
	out += std::format("App Version      : {}.{}\n", ci.AppVersion, ci.AppRevision);
	out += std::format("Manufacturer     : 0x{:02X} {}\n",
					   ci.ManufacturerId,
					   ManufacturerIdName());
	out += std::format("ProductType      : 0x{:02X} {}\n",
					   ci.ProductType,
					   ProductTypeName());
	out += std::format("ProductId        : 0x{:02X} {}\n",
					   ci.ProductId,
					   ProductIdName());

	// Home ID
	out += std::format("HomeId/NodeId    : 0x{:08X} / {}\n",
					   ci.HomeId, ci.NodeId);

	return out;
}

// Human-readable initialization state
std::string ControllerInfoFormatter::InitializationStateString() const
{
	using IS = ControllerInfo::eInitializationState;

	switch (ci.InitializationState)
	{
	case IS::NotInitialized: return "NotInitialized";
	case IS::InitDataPending: return "InitDataPending";
	case IS::InitDataDone: return "InitDataDone";
	case IS::InitControllerCapabilitiesPending: return "InitControllerCapabilitiesPending";
	case IS::InitControllerCapabilitiesDone: return "InitControllerCapabilitiesDone";
	case IS::InitCapabilitiesPending: return "InitCapabilitiesPending";
	case IS::InitCapabilitiesDone: return "InitCapabilitiesDone";
	case IS::InitProtocolVersionPending: return "InitProtocolVersionPending";
	case IS::InitProtocolVersionDone: return "InitProtocolVersionDone";
	case IS::InitNetworkIdsFromMemoryPending: return "InitNetworkIdsFromMemoryPending";
	case IS::InitNetworkIdsFromMemoryDone: return "InitNetworkIdsFromMemoryDone";
	case IS::InitLibraryVersionPending: return "InitLibraryVersionPending";
	case IS::InitLibraryVersionDone: return "InitLibraryVersionDone";
	case IS::InitLibraryTypePending: return "InitLibraryTypePending";
	case IS::InitLibraryTypeDone: return "InitLibraryTypeDone";
	case IS::Paused: return "Paused";
	case IS::Initialized: return "Initialized";
	}
	return "Unknown";
}

std::string ControllerInfoFormatter::APIVersionText() const
{
	if (ci.ApiVersion > 9)
		return std::format("{} Z-Wave Alliance ", ci.ApiVersion - 9);

	return std::format("{} manufacturer-specific ", ci.ApiVersion);
}

std::string ControllerInfoFormatter::ChipTypeName() const
{
	using CT = ControllerInfo::eChipTypes;

	switch (ci.ChipType)
	{
	case CT::ZW010x: return "ZW010x (100 series)";
	case CT::ZW020x: return "ZW020x (200 series)";
	case CT::ZW030x: return "ZW030x (300 series)";
	case CT::ZW040x: return "ZW040x (400 series)";
	case CT::ZW050x: return "ZW050x (500 series)";
	case CT::ZW070x: return "ZW070x (700 series)";
	case CT::ZW080x: return "ZW080x (800 series)";
	default: return "Unknown";
	}
}

std::string ControllerInfoFormatter::LibraryTypeName() const
{
	using LT = ControllerInfo::eLibraryType;

	switch (ci.libraryType)
	{
	case LT::Unknown:            return "Unknown";

	case LT::StaticController:   return "Static Controller";
	case LT::BridgeController:   return "Bridge Controller";
	case LT::PortableController: return "Portable Controller";
	case LT::EnhancedController: return "Enhanced Controller";

	case LT::Slave:              return "Slave";
	case LT::Installer:          return "Installer";
	case LT::RoutingSlave:       return "Routing Slave";
	case LT::EnhancedSlave:      return "Enhanced Slave";

	case LT::DeviceUnderTest:    return "Device Under Test";
	case LT::AVRemote:           return "AV Remote";
	case LT::AVDevice:           return "AV Device";
	}

	return "Unknown";
}

std::string ControllerInfoFormatter::ManufacturerIdName() const
{
	switch (ci.ManufacturerId)
	{
	case 0x0001: return "Sigma Designs";
	case 0x0086: return "Merten / Schneider Electric";
	case 0x010F: return "Fibaro";
	case 0x014A: return "Heatit / Thermo-Floor";
	case 0x018E: return "Aeotec";
	case 0x021F: return "Zooz";
	case 0x0258: return "Shelly";
	case 0x0260: return "Qubino";
	case 0x027A: return "GE / Jasco";
	case 0x0315: return "Ring";
	case 0x0371: return "Inovelli";
		// Add more as needed…
	default: return "Unknown";
	}
}

std::string ControllerInfoFormatter::ProductTypeName() const
{
	switch (ci.ProductType)
	{
	case 0x0001: return "Binary Switch";
	case 0x0002: return "Multilevel Switch";
	case 0x0003: return "Binary Sensor";
	case 0x0004: return "Multilevel Sensor";
	case 0x0005: return "Remote Control";
	case 0x0006: return "Wall Controller";
	case 0x0007: return "Thermostat";
	case 0x0008: return "Meter";
	case 0x0009: return "Scene Controller";
	case 0x0100: return "Generic Controller";
	case 0x0200: return "Lighting Device";
	case 0x0300: return "Security Device";
		// Add more as needed…
	default: return "Unknown";
	}
}

std::string ControllerInfoFormatter::ProductIdName() const
{
	switch (ci.ProductId)
	{
	case 0x0001: return "Basic Device";
	case 0x0002: return "Dimmer";
	case 0x0003: return "Relay Switch";
	case 0x0004: return "Motion Sensor";
	case 0x0005: return "Door/Window Sensor";
	case 0x0006: return "Keypad";
	case 0x0007: return "Energy Meter";
	case 0x0008: return "Scene Controller";
	case 0x1000: return "Wall Controller (Merten)";
	case 0x2000: return "Heatit Z-Push Button";
		// Add more as needed…
	default: return "Unknown";
	}
}
