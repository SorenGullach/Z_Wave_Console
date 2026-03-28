#pragma once
#include "NodeId.h"

enum class UINotify
{
    LogChanged,
    ControllerChanged,
    NodeListChanged,
    NodeChanged,
};

extern void NotifyUI(const UINotify notify, node_t nodeId=(node_t)0);
