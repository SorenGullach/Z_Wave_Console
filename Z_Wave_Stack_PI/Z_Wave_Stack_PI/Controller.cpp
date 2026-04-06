#include "Controller.h"
#include <iostream> 

Controller::Controller()
    : Nodes([this](const APIFrame& f) { Enqueue(f); })
	, initialize([this](const APIFrame& f) { Enqueue(f); }, static_cast<ControllerInfo&>(*this))
	, nodeInterview(static_cast<Nodes&>(*this), [this](const APIFrame& f) { Enqueue(f); })
{
}

Controller::~Controller() = default;

void Controller::Start()
{
	initialize.Start();

	while(!initialize.Done())
	{
		
	}

	if (InitializationState == eInitializationState::Initialized)
	{
        std::cout << ControllerInfo::ToString() << "\n";
		for (auto id : NodeIds)
		{
			Nodes::GetOrCreate(id);

			nodeInterview.Start(id);
		
			while (!nodeInterview.Done(id))
			{
			}
		}
	}
}

bool Controller::OnFrameReceived(const APIFrame& frame)
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
		return initialize.HandleFrame(frame);

	case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
	case eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA:
		return nodeInterview.HandleFrame(frame);

	case eCommandIds::ZW_API_CONTROLLER_SEND_DATA:
		if (frame.Type() == APIFrame::eFrameTypes::RES)
			Log.AddL(eLogTypes::RTU, MakeTag(), "<< route=sendData type=RES txStatus=0x{:02X} len={}", frame.payload[0], frame.payload.size());
		if (frame.Type() == APIFrame::eFrameTypes::REQ)
			Log.AddL(eLogTypes::RTU, MakeTag(), "<< route=sendData type=REQ sessionId={} txStatus=0x{:02X} len={}", frame.payload[0], frame.payload[1], frame.payload.size());
		return true;

	case eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER:
		Log.AddL(eLogTypes::RTU, MakeTag(), "<< route=ccDispatcher {}", frame.Info());
//		CCDispatcher.HandleCCFrame(frame.payload);
		return true;

	case eCommandIds::ZW_API_IS_NODE_FAILED:
//		nodes.HandleNodeFailed(frame.payload[0]);
		return true;

	case eCommandIds::ZW_API_REMOVE_FAILED_NODE:
	case eCommandIds::ZW_API_REMOVE_NODE_FROM_NETWORK:
		return true;

	default:
		return false;

	}
}

bool Controller::OnFrameReceivedTimeout(const APIFrame& frame)
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
		Log.AddL(eLogTypes::DBG, MakeTag(), "<< route=initializeManager TIMEOUT {}", frame.Info());
		return initialize.HandleFrameTimeout(frame);

	case eCommandIds::ZW_API_APPLICATION_UPDATE:
	case eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA:
	case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
		Log.AddL(eLogTypes::DVC, MakeTag(), "<< route=interviewManager TIMEOUT {}", frame.Info());
		return nodeInterview.HandleFrameTimeout(frame);

	case eCommandIds::ZW_API_CONTROLLER_SEND_DATA:
		return true;

	case eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER:
		Log.AddL(eLogTypes::DVC, MakeTag(), "<< route=ccDispatcher TIMEOUT {}", frame.Info());
		//CCDispatcher.HandleCCFrameTimeout(frame.payload);
		return true;

	default:
		return false;
	}
}
