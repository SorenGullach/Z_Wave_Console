#include "Controller.h"

Controller::Controller()
	: initialize([this](const APIFrame& f) { Enqueue(f); }, static_cast<ControllerInfo&>(*this))
{
}

Controller::~Controller() = default;

bool Controller::InitializeFrame(const APIFrame& frame)
{
	return initialize.HandleFrame(frame);
}