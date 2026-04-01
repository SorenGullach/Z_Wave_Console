#pragma once

#include "ControllerInfo.h"
#include "APIDispatcher.h"
#include "Initialize.h"

class Controller : public ControllerInfo, public APIDispatcher
{
public:
	Controller();
	~Controller();

	void Start()
	{
		initialize.Start();
	}

protected:
	bool InitializeFrame(const APIFrame& frame) override;
private:
	Initialize initialize;
};