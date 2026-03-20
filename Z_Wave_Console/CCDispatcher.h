#pragma once

#include <cstdint>
#include <vector>

class ZW_Nodes;

#include "APIFrame.h"

class ZW_CCDispatcher
{
public:
	ZW_CCDispatcher(EnqueueFn enqueue, ZW_Nodes& nodes) :
		nodes(nodes),
		enqueue(enqueue)
	{}

	void HandleCCFrame(const APIFrame::PayLoad& payload);
	void HandleCCFrameTimeout(const APIFrame::PayLoad& payload);

private:
	EnqueueFn enqueue;

	bool NodeIdBaseType16Bit = false;
	ZW_Nodes& nodes;

//	void HandleManufacturerSpecificCCReport(uint8_t cmdId, const std::vector<uint8_t>& cmdParams, uint16_t nodeId);
};

