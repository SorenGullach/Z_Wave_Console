#include "ZWaveAPI.h"
#include "Notify.h"

ZWaveAPI::ZWaveAPI() = default;

ZWaveAPI::~ZWaveAPI() = default;

void NotifyUI(const UINotify notify, nodeid_t nodeId)
{
  (void)notify;
	(void)nodeId;
	// TODO: Implement server notification
	// Temporarily commented out for build optimization work
	/*
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
	*/
}
