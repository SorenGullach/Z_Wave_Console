
#include <algorithm>
#include <iostream>
#include <sstream>
#include "JsonUtils.h"

#include "Logging.h"
#include "Z_Wave.h"
#include "TcpServer.h"
#include "JSONConversions.h"
#include "Notify.h"

static const int PORT = 5555;

ZW_Logging Log; // a global var used for logging
Z_Wave ZW;

class ZWaveCoreServer final : public TcpServer
{
public:
	explicit ZWaveCoreServer(int port)
		: TcpServer(port)
	{}

protected:
	std::string HandleMessage(const std::string& message) override
	{
		std::string type;
		if (!JsonUtils::TryExtractString(message, "type", type))
			return JsonUtils::MakeError("Invalid JSON or missing type", message);

		if (type == "ping")
			return R"({"type":"pong"})";

		if (type == "get_controller_info")
			return JSONConversions::ToJSON(ZW.GetModule());

		if (type == "list_nodes")
			return JSONConversions::ToJSON(ZW.GetNodes());

		if (type == "get_logs")
		{
			int count = ZW_Logging::maxEntries;
			JsonUtils::TryExtractInt(message, "count", count);
			if (count < 0)	count = 10;
			return JSONConversions::ToJSON(Log.GetLogEntrys(count));
		}

		if (type == "get_node")
		{
			int id = 0;
			JsonUtils::TryExtractInt(message, "node_id", id);
			return JSONConversions::ToJSON((node_t)id, ZW.GetNodes());
		}

		if (type == "set_config")
		{
			int id = 0, value = 0, param = 0, size = 0;
			JsonUtils::TryExtractInt(message, "node_id", id);
			JsonUtils::TryExtractInt(message, "param", param);
			JsonUtils::TryExtractInt(message, "value", value);
			JsonUtils::TryExtractInt(message, "size", size);
			ZW.Configure((node_t)id, param, value, size);
			return JsonUtils::MakeOk("", message);
		}

		if (type == "switch_binary")
		{
			int id = 0, value = 0;
			JsonUtils::TryExtractInt(message, "node_id", id);
			JsonUtils::TryExtractInt(message, "value", value);
			ZW.SwitchBinary((node_t)id, static_cast<uint8_t>(value));
			return JsonUtils::MakeOk("", message);
		}

		if (type == "mc_bind")
		{
			int id = 0, groupId = 0, targetNodeId = 0, targetEndpoint = 0;
			JsonUtils::TryExtractInt(message, "node_id", id);
			JsonUtils::TryExtractInt(message, "group_id", groupId);
			JsonUtils::TryExtractInt(message, "target_node_id", targetNodeId);
			JsonUtils::TryExtractInt(message, "target_endpoint", targetEndpoint);
			ZW.MCBind((node_t)id, static_cast<uint8_t>(groupId), (node_t)targetNodeId, static_cast<uint8_t>(targetEndpoint));
			return JsonUtils::MakeOk("", message);
		}

		if (type == "mc_unbind")
		{
			int id = 0, groupId = 0, targetNodeId = 0, targetEndpoint = 0;
			JsonUtils::TryExtractInt(message, "node_id", id);
			JsonUtils::TryExtractInt(message, "group_id", groupId);
			JsonUtils::TryExtractInt(message, "target_node_id", targetNodeId);
			JsonUtils::TryExtractInt(message, "target_endpoint", targetEndpoint);
			ZW.MCUnbind((node_t)id, static_cast<uint8_t>(groupId), (node_t)targetNodeId, static_cast<uint8_t>(targetEndpoint));
			return JsonUtils::MakeOk("", message);
		}

		if (type == "read_associations")
		{
			int id = 0;
			JsonUtils::TryExtractInt(message, "node_id", id);
			ZW.AssociationInterview((node_t)id);
			return JsonUtils::MakeOk("", message);
		}

		if (type == "read_configuration")
		{
			int id = 0, groupId = 0;
			JsonUtils::TryExtractInt(message, "node_id", id);
			JsonUtils::TryExtractInt(message, "group_id", groupId);
			ZW.ConfigurationInterview((node_t)id, static_cast<uint8_t>(groupId));
			return JsonUtils::MakeOk("", message);
		}

		/*
		if (type == "soft_reset")
		{
			if (!ZW.IsSerialOpen())
				return "{\"type\":\"error\",\"message\":\"Not connected\"}";
			ZW.StartInitialization();
			return "{\"type\":\"ok\"}";
		}
		*/
		return JsonUtils::MakeError("Unknown command", type);
	}
};

// Initialize server
ZWaveCoreServer server(PORT);

void NotifyUI(const UINotify notify, node_t nodeId)
{
	switch (notify)
	{
	case UINotify::LogChanged:
		server.SendToClient(
			"{\"type\":\"log_changed\"}"
		);
		break;

	case UINotify::ControllerChanged:
		server.SendToClient(
			"{\"type\":\"controller_changed\"}"
		);
		break;

	case UINotify::NodeListChanged:
		server.SendToClient(
			"{\"type\":\"node_list_changed\"}"
		);
		break;

	case UINotify::NodeChanged:
		server.SendToClient(
			"{\"type\":\"node_changed\",\"nodeId\":" + std::to_string(nodeId.value) + "}"
		);
		break;
	}
}

int main()
{
	Log.SetLogTypeOn(eLogTypes::ERR);
	Log.SetLogTypeOn(eLogTypes::ITF);
	Log.SetLogTypeOn(eLogTypes::RTU);
	Log.SetLogTypeOn(eLogTypes::DVC);
	Log.SetLogTypeOn(eLogTypes::ITW);
	Log.SetLogTypeOn(eLogTypes::DBG);
	Log.SetLogTypeOn(eLogTypes::WRN);

	std::cout << "Hello ZWave World! Im a ZWaveCore app that can connect to a ZWave network.\n";
	std::cout << "And connect to you with a TCP connection.\n\n";

	Log.AddL(eLogTypes::DBG, MakeTag(), "Hello ZWave World! Im a ZWaveCore app that can connect to a ZWave network.");
	Log.AddL(eLogTypes::DBG, MakeTag(), "And connect to you with a TCP connection.");

	ZW.OpenPort("COM3");

	ZW.StartInitialization();

	try
	{
		server.Run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Fatal error: " << e.what() << "\n";
	}

}