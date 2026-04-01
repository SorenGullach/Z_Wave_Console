
#include "APIDispatcher.h"

bool APIDispatcher::OnFrameReceived(const APIFrame& frame)
{
	switch (frame.APICmd.CmdId)
	{
	case eCommandIds::FUNC_ID_GET_INIT_DATA:
	case eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES:
	case eCommandIds::FUNC_ID_GET_CAPABILITIES:
	case eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION:
	case eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY:
	case eCommandIds::FUNC_ID_GET_LIBRARY_VERSION:
	case eCommandIds::FUNC_ID_GET_LIBRARY_TYPE:
		InitializeFrame(frame);
		return true;
	}
	return false;
}

bool APIDispatcher::OnFrameReceivedTimeout(const APIFrame& frame)
{
	(void)frame;
	return false;
}
