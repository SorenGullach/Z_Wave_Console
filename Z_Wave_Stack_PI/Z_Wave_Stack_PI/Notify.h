#pragma once
#include "NodeId_t.h"

enum class UINotify
{
    LogChanged,
    ControllerChanged,
    NodeListChanged,
    NodeChanged,
};

extern void NotifyUI(const UINotify notify, nodeid_t nodeId=(nodeid_t)0);
