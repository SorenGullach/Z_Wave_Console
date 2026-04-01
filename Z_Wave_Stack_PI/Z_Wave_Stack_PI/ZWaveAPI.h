#pragma once

#include "Controller.h"

class ZWaveAPI : public Controller
{
public:
	ZWaveAPI();
	~ZWaveAPI();

	using Controller::Start;

};
