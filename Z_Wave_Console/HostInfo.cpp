

#include <string>
#include <format>

#include "Module.h"

std::string ZW_Module::ToString()
{
    std::string out;
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

    // Chip info
    out += std::format("Chip             : Type={} Ver={}\n", ChipType, ChipVersion);

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
    out += std::format("Manufacturer     : {}  Type={}  Product={}\n",
                       ManufacturerId, ProductType, ProductId);

    // Home ID
    out += std::format("HomeId/NodeId    : 0x{:08X} / 0x{:04X}\n",
                       HomeId, NodeId);

    return out;
}
