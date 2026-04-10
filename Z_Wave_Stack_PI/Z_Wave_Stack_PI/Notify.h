#pragma once
#include "Strong_id.h"

enum class UINotify
{
    LogChanged,
    ControllerChanged,
    NodeListChanged,
    NodeChanged,
};

extern void NotifyUI(const UINotify notify, nodeid_t nodeId = nullptr);
