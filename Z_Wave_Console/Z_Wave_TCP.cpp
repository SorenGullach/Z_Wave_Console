
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
/*
		if (type == "list_nodes")
			return ""; //GetNodeList();
*/
		if (type == "get_logs")
		{
			int count = 200;
			JsonUtils::TryExtractInt(message, "count", count);
			if (count < 0)
				count = 0;
			return JSONConversions::ToJSON(Log.GetLogEntrys(count));
		}
		/*
		if (type == "get_node")
		{
			int id = 0;
			JsonUtils::TryExtractInt(message, "nodeId", id);
			return ""; //GetNodeDetails((node_t)id);
		}

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
		server.SendToClient("{\"type\":\"log_changed\"}");
		break;

	case UINotify::ControllerChanged:
		server.SendToClient("{\"type\":\"controller_changed\"}");
		break;

	case UINotify::NodeListChanged:
		server.SendToClient("{\"type\":\"node_list_changed\"}");
		break;

	case UINotify::NodeChanged:
		if (nodeId.value > 1 && nodeId.value < 232)
			server.SendToClient(
				"{\"type\":\"node_changed\",\"nodeId\":" + std::to_string(nodeId.value) + "}"
			);
		break;

	case UINotify::NodeStateChanged:
		if (nodeId.value > 1 && nodeId.value < 232)
			server.SendToClient(
				"{\"type\":\"node_state_changed\",\"nodeId\":" + std::to_string(nodeId.value) + "}"
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