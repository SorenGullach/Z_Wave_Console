#pragma once

#include "Logging.h"
#include "Interface.h"

class APIDispatcher : public Interface
{
public:

protected:
	virtual bool OnFrameReceived(const APIFrame& frame) override;
	virtual bool OnFrameReceivedTimeout(const APIFrame& frame) override;

	virtual bool InitializeFrame(const APIFrame& frame)
	{
		Log.AddL(eLogTypes::ITZ, MakeTag(), "InitializeManager: {}", frame.Info());
		return true;
	}
};
