#include <string>
#include <format>
#include <sstream>

#include "Module.h"

std::string ZW_Module::ToString()
{
	std::string out;
	// initialize state
	out += std::format("Initialized      : {} {}\n", (uint8_t)InitializationState, InitializationState == eInitializationState::Initialized ? "Yes" : "No");

	// API version
	const std::string apiVersionText =
		(ApiVersion >= 10)
		? std::format("{} Non standard", ApiVersion - 10)
		: std::to_string(ApiVersion);

	out += std::format("API Version      : {}\n", apiVersionText);

	// Compact grouped fields
	out += std::format("EndNode/Timer    : {} / {}\n",
					   IsEndNode ? "Yes" : "No",
					   HasTimerFunctions ? "Yes" : "No");

	out += std::format("Primary/SIS      : {} / {}\n",
					   IsPrimarayController ? "Yes" : "No",
					   HasSISFunctions ? "Yes" : "No");

	// Node list
	out += std::format("Node List        : ({}) ", NodeIds.size());
	for (auto n : NodeIds)
		out += std::format("{} ", n);
	out += "\n";

	// Chip DVC
	out += std::format("Chip             : Type={} Ver={}\n", (int)ChipType, ChipVersion);

	// Controller capabilities
	out += std::format("Secondary/Other  : {} / {}\n",
					   IsSecondaryController ? "Yes" : "No",
					   IsOtherNetwork ? "Yes" : "No");

	out += std::format("SIS/SUC/NoNodes  : {} / {} / {}\n",
					   IsSISPresent ? "Yes" : "No",
					   IsSUCEnabled ? "Yes" : "No",
					   IsNoNodesIncluded ? "Yes" : "No");

	// Protocol version
	out += std::format("Protocol Version : type=0x{:02X} {}.{}.{} (AF {})\n",
					   ProtocolType,
					   ProtocolMajorVersion,
					   ProtocolMinorVersion,
					   ProtocolRevisionVersion,
					   AppFrameworkBuildNumber);

	// Library
	out += std::format("Library          : {} (",
					   LibraryVersion);

	switch (libraryType)
	{
	case 0x01: out += "Static Controller"; break;
	case 0x02: out += "Bridge Controller"; break;
	case 0x03: out += "Portable Controller"; break;
	case 0x07: out += "Enhanced Slave"; break;
	case 0x08: out += "Enhanced Controller"; break;
	default:   out += std::format("Unknown 0x{:02X}", libraryType); break;
	}
	out += ")\n";

	// API commands
	out += std::format("API Commands     : ({}) ", ApiCommands.size());
	for (auto n : ApiCommands)
		out += std::format("{} ", n);
	out += "\n";

	// Capabilities
	out += std::format("App Version      : {}.{}\n", AppVersion, AppRevision);
	out += std::format("Manufacturer     : 0x{:02X} Type=0x{:02X} Product=0x{:02X}\n",
					   ManufacturerId, ProductType, ProductId);

	// Home ID
	out += std::format("HomeId/NodeId    : 0x{:08X} / {}\n",
					   HomeId, NodeId);

	return out;
}

// Human-readable initialization state
std::string ZW_Module::InitializationStateString() const
{
	switch (InitializationState)
	{
	case NotInitialized: return "NotInitialized";
	case InitDataPending: return "InitDataPending";
	case InitDataDone: return "InitDataDone";
	case InitControllerCapabilitiesPending: return "InitControllerCapabilitiesPending";
	case InitControllerCapabilitiesDone: return "InitControllerCapabilitiesDone";
	case InitCapabilitiesPending: return "InitCapabilitiesPending";
	case InitCapabilitiesDone: return "InitCapabilitiesDone";
	case InitProtocolVersionPending: return "InitProtocolVersionPending";
	case InitProtocolVersionDone: return "InitProtocolVersionDone";
	case InitNetworkIdsFromMemoryPending: return "InitNetworkIdsFromMemoryPending";
	case InitNetworkIdsFromMemoryDone: return "InitNetworkIdsFromMemoryDone";
	case InitLibraryVersionPending: return "InitLibraryVersionPending";
	case InitLibraryVersionDone: return "InitLibraryVersionDone";
	case InitLibraryTypePending: return "InitLibraryTypePending";
	case InitLibraryTypeDone: return "InitLibraryTypeDone";
	case Paused: return "Paused";
	case Initialized: return "Initialized";
	}
	return "Unknown";
}

// Protocol version as strings
std::vector<std::string> ZW_Module::ProtocolVersionStrings() const
{
	return {
		"type=" + std::to_string(ProtocolType),
		"major=" + std::to_string(ProtocolMajorVersion),
		"minor=" + std::to_string(ProtocolMinorVersion),
		"revision=" + std::to_string(ProtocolRevisionVersion)
	};
}

// Chip info
std::vector<std::string> ZW_Module::ChipStrings() const
{
	return {
		"type=" + std::to_string((int)ChipType),
		"typeName=" + ChipName(),
		"version=" + std::to_string(ChipVersion)
	};
}

// Manufacturer info
std::vector<std::string> ZW_Module::ManufacturerStrings() const
{
	return {
		"id=" + std::to_string(ManufacturerId),
		"idName=" + GetManufacturer(),
		"productType=" + std::to_string(ProductType),
		"productTypeName=" + GetProductTypeName(),
		"productId=" + std::to_string(ProductId),
		"productIdName=" + GetProductIdName()
	};
}

std::string ZW_Module::GetManufacturer() const
{
	switch (ManufacturerId)
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

std::string ZW_Module::GetProductTypeName() const
{
	switch (ProductType)
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

std::string ZW_Module::GetProductIdName() const
{
	switch (ProductId)
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

// Controller flags as readable strings
std::vector<std::string> ZW_Module::ControllerFlagStrings() const
{
	std::vector<std::string> flags;

	if (IsEndNode) flags.push_back("EndNode");
	if (HasTimerFunctions) flags.push_back("TimerFunctions");
	if (IsPrimarayController) flags.push_back("PrimaryController");
	if (HasSISFunctions) flags.push_back("SISFunctions");
	if (IsSecondaryController) flags.push_back("SecondaryController");
	if (IsOtherNetwork) flags.push_back("OtherNetwork");
	if (IsSISPresent) flags.push_back("SISPresent");
	if (IsSUCEnabled) flags.push_back("SUCEnabled");
	if (IsNoNodesIncluded) flags.push_back("NoNodesIncluded");

	return flags;
}

std::string ZW_Module::GetLibraryTypeName() const
{
	switch (libraryType)
	{
	case 0x00: return "Unknown";
	case 0x01: return "Static Controller";        // ZW_LIB_CONTROLLER_STATIC
	case 0x02: return "Controller";               // ZW_LIB_CONTROLLER
	case 0x03: return "Enhanced Slave";           // ZW_LIB_SLAVE_ENHANCED
	case 0x04: return "Slave";                    // ZW_LIB_SLAVE
	case 0x05: return "Installer";                // ZW_LIB_INSTALLER
	case 0x06: return "Routing Slave";            // ZW_LIB_SLAVE_ROUTING
	case 0x07: return "Bridge Controller";        // ZW_LIB_CONTROLLER_BRIDGE
	case 0x08: return "Device Under Test";        // ZW_LIB_DUT
	case 0x09: return "AV Remote";                // ZW_LIB_AVREMOTE
	case 0x0A: return "AV Device";                // ZW_LIB_AVDEVICE
	default:   return "Unknown";
	}
}

std::string ZW_Module::DescribeControllerCapabilities() const
{
	std::ostringstream oss{};
	if (IsSecondaryController) oss << "SecondaryController, ";
	if (IsOtherNetwork) oss << "OtherNetwork, ";
	if (IsSISPresent) oss << "SISPresent, ";
	if (IsSUCEnabled) oss << "SUCEnabled, ";
	if (IsNoNodesIncluded) oss << "NoNodesIncluded, ";

	std::string s = oss.str();
	if (!s.empty()) s.erase(s.size() - 2);
	return s.empty() ? "None" : s;
}

std::string ZW_Module::ChipName() const
{
	eChipTypes t = ChipType;
	switch (t)
	{
	case eChipTypes::ZW010x: return "ZW010x";
	case eChipTypes::ZW020x: return "ZW020x";
	case eChipTypes::ZW030x: return "ZW030x";
	case eChipTypes::ZW040x: return "ZW040x";
	case eChipTypes::ZW050x: return "ZW050x (500 series)";
	case eChipTypes::ZW070x: return "ZW070x (700 series)";
	case eChipTypes::ZW080x: return "ZW080x (800 series)";
	default: return "Unknown";
	}
}

std::string ZW_Module::DescribeApiCapabilities() const
{
	std::ostringstream oss;

	uint8_t caps = ApiCapabilities;
	if (caps & 0x01) oss << "EndNode, ";
	if (caps & 0x02) oss << "TimerFunctions, ";
	if (caps & 0x04) oss << "PrimaryController, ";
	if (caps & 0x08) oss << "SISFunctions, ";

	std::string s = oss.str();
	if (!s.empty()) s.erase(s.size() - 2);
	return s.empty() ? "None" : s;
}
