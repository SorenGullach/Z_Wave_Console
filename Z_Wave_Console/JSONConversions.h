#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdio>

#include "JsonUtils.h"
#include "Logging.h"
#include "Module.h"
#include "Node.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace JSONConversions
{
	// make a list of nodes
	inline std::string ToJSON(const ZW_Nodes& ns)
	{
		json j;
		j["type"] = "nodes_list";
		j["nodes"] = json::array();

		for (const auto& kv : ns)
		{
			const ZW_Node* node = kv.second.get();
			if (!node)
				continue;

			json n;

			n["nodeId"] = node->NodeId.value;
			n["floor"] = node->Floor.empty() ? "Unknown" : node->Floor;
			n["room"] = node->Room.empty() ? "Unknown" : node->Room;
			n["state"] = node->NodeStateString();
			n["interviewState"] = node->InterviewStateString();

			j["nodes"].push_back(n);
		}

		return j.dump();
	}

	inline std::string ToJSONCfg(const node_t nodeid, const ZW_Nodes& ns)
	{
		const ZW_Node* node = ns.Get(nodeid);
		json j;

		if (!node)
		{
			j["type"] = "node_info";
			j["error"] = "Node not found";
			return j.dump();
		}

		j["type"] = "node_info";
		j["nodeId"] = node->NodeId.value;

		// Floor / Room (UI helpers)
		j["floor"] = node->Floor.empty() ? "Unknown" : node->Floor;
		j["room"] = node->Room.empty() ? "Unknown" : node->Room;

		// State
		j["state"] = node->NodeStateString();
		j["interviewState"] = node->InterviewStateString();

		// Configuration parameters
		{
			json cfg = json::array();
			for (const auto& c : node->configurationInfo)
			{
				//if (!c.valid) continue;

				cfg.push_back({
					{"param", c.paramNumber},
					{"size", c.size},
					{"value", c.value},
					{"valid", c.valid}
							  });
			}
			j["configuration"] = cfg;
		}

		return j.dump();

	}

	inline std::string ToJSON(const node_t nodeid, const ZW_Nodes& ns)
	{
		const ZW_Node* node = ns.Get(nodeid);
		json j;

		if (!node)
		{
			j["type"] = "node_info";
			j["error"] = "Node not found";
			return j.dump();
		}

		j["type"] = "node_info";
		j["nodeId"] = node->NodeId.value;

		// Floor / Room (UI helpers)
		j["floor"] = node->Floor.empty() ? "Unknown" : node->Floor;
		j["room"] = node->Room.empty() ? "Unknown" : node->Room;

		// State
		j["state"] = node->NodeStateString();
		j["interviewState"] = node->InterviewStateString();

		// Protocol Info
		{
			const auto& p = node->protocolInfo;
			j["protocol"] = {
				{"basic", p.basic},
				{"generic", p.generic},
				{"specific", p.specific},
				{"listening", p.isListening},
				{"routing", p.isRouting},
				{"speed", p.supportedSpeed},
				{"protocolVersion", p.protocolVersion},
				{"optionalFunctionality", p.optionalFunctionality},
				{"sensor1000ms", p.sensor1000ms},
				{"sensor250ms", p.sensor250ms},
				{"beamCapable", p.beamCapable},
				{"routingEndNode", p.routingEndNode},
				{"specificDevice", p.specificDevice},
				{"controllerNode", p.controllerNode},
				{"security", p.security}
			};
		}

		// Manufacturer Info
		{
			const auto& m = node->manufacturerInfo;
			j["manufacturer"] = {
				{"id", m.mfgId},
				{"productType", m.prodType},
				{"productId", m.prodId},
				{"deviceIdType", m.deviceIdType},
				{"deviceIdFormat", m.deviceIdFormat},
				{"hasDeviceId", m.hasDeviceId},
				{"hasManufacturerData", m.hasManufacturerData}
			};
		}

		// WakeUp Info
		{
			const auto& w = node->wakeUpInfo;
			j["wakeUp"] = {
				{"interval", w.wakeUpInterval},
				{"min", w.wakeUpMin},
				{"max", w.wakeUpMax},
				{"default", w.wakeUpDefault},
				{"hasLastReport", w.hasLastReport}
			};
		}

		// Battery / Basic / Switch / Sensor values
		j["values"] = {
			{"battery", node->batteryLevel.has_value() ? *node->batteryLevel : -1},
			{"basic", node->basicValue.has_value() ? *node->basicValue : -1},
			{"switchBinary", node->switchBinaryValue.has_value() ? *node->switchBinaryValue : -1},
			{"switchMultilevel", node->switchMultilevelValue.has_value() ? *node->switchMultilevelValue : -1},
			{"sensorBinary", node->sensorBinaryValue.has_value() ? *node->sensorBinaryValue : -1},
			{"protection", node->protectionState.has_value() ? *node->protectionState : -1}
		};

		// Supported Command Classes
		{
			json ccList = json::array();

			for (auto cc : node->GetSupportedCCs())
			{
				auto* tag = node->GetCC(cc);
				if (tag)
				{
					ccList.push_back({
						{"cc", cc},
						{"name", CommandClassToString(cc)},
						{"version", tag->version},
									 });
				}
			}
			j["supportedCCs"] = ccList;
		}

		// Multi-channel endpoints
		{
			const auto& mc = node->multiChannel;
			json endpoints = json::array();

			for (const auto& ep : mc.endpoints)
			{
				endpoints.push_back({
					{"endpointId", ep.endpointId},
					{"generic", ep.generic},
					{"specific", ep.specific},
					{"supportedCCs", ep.supportedCCs},
					{"hasCapabilityReport", ep.hasCapabilityReport}
									});
			}

			j["multiChannel"] = {
				{"endpointCount", mc.endpointCount},
				{"hasEndpointReport", mc.hasEndpointReport},
				{"endpoints", endpoints}
			};
		}

		// Association Groups
		{
			json groups = json::array();
			for (const auto& g : node->associationGroups)
			{
				json members = json::array();
				for (const auto& m : g.nodeList)
				{
					if (m.valid)
						members.push_back(m.nodeId.value);
				}

				groups.push_back({
					{"groupId", g.groupId},
					{"maxNodes", g.maxNodes},
					{"members", members},
					{"hasLastReport", g.hasLastReport}
								 });
			}
			j["associationGroups"] = groups;
		}

		// Multi-channel association groups
		{
			json groups = json::array();
			for (const auto& g : node->multiChannelAssociationGroups)
			{
				json members = json::array();
				for (const auto& m : g.members)
				{
					if (m.valid)
					{
						members.push_back({
							{"nodeId", m.nodeId.value},
							{"endpoint", m.endpointId}
										  });
					}
				}

				groups.push_back({
					{"groupId", g.groupId},
					{"maxNodes", g.maxNodes},
					{"members", members},
					{"hasLastReport", g.hasLastReport}
								 });
			}
			j["multiChannelAssociations"] = groups;
		}

		// Configuration parameters
		{
			json cfg = json::array();
			for (const auto& c : node->configurationInfo)
			{
				//if (!c.valid) continue;

				cfg.push_back({
					{"param", c.paramNumber},
					{"size", c.size},
					{"value", c.value},
					{"valid", c.valid}
							  });
			}
			j["configuration"] = cfg;
		}

		return j.dump();
	}

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
