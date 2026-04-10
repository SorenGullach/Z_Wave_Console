#pragma once

#include "ControllerInfo.h"
#include "Interface.h"
#include "Initialize.h"
#include "NodeInterview.h"
#include "Nodes.h"

class Controller : public Nodes, public ControllerInfo, public Interface
{
public:
	Controller();
	~Controller();

	void Start();

protected:
	virtual bool OnFrameReceived(const APIFrame& frame) override;
	virtual bool OnFrameReceivedTimeout(const APIFrame& frame) override;

private:
	Initialize initialize;
	NodeInterview nodeInterview;

	// Add these declarations:
	void HandleCCFrame(const payload_t& payload);
	void HandleCCFrameTimeout(const payload_t& payload);
};