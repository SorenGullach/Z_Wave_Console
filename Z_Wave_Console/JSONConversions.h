#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdio>

#include "JsonUtils.h"
#include "Logging.h"
#include "Module.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace JSONConversions
{
    inline std::string ToJSON(const ZW_Module& m)
    {
        json j;

        j["type"] = "controller_info";

        j["initialization"] = {
            {"state", m.InitializationStateString()}
        };

        j["api"] = {
            {"version", m.ApiVersion},
            {"capabilities", m.ApiCapabilities},
            {"capabilitiesDescription", m.DescribeApiCapabilities() }
        };

        j["controllerFlags"] = m.ControllerFlagStrings();
        j["chip"] = m.ChipStrings();
        j["protocolVersion"] = m.ProtocolVersionStrings();
        j["manufacturer"] = m.ManufacturerStrings();

        j["library"] = {
            {"version", m.LibraryVersion},
            {"type", m.libraryType },
            {"typeName", m.GetLibraryTypeName() }
        };

        j["application"] = {
            {"version", m.AppVersion},
            {"revision", m.AppRevision}
        };

        j["network"] = {
            {"homeId", m.HomeId},
            {"nodeId", m.NodeId}
        };

        // Node list
        j["nodeIds"] = json::array();
        for (auto& id : m.NodeIds)
            j["nodeIds"].push_back(id.value);

        // Supported API commands
        j["apiCommands"] = m.ApiCommands;

        return j.dump();
    }

    inline std::string ToJSON(const std::vector<ZW_Logging::LogEntry>& logs)
    {
        json j;
        j["type"] = "logs";

        for (auto& e : logs)
        {
            j["entries"].push_back({
                {"time", e.time},
                {"level", ZW_Logging::ToString(e.lt)},
                {"tag", e.tag},
                {"msg", e.msg}
                                   });
        }

        return j.dump();
    }

}
