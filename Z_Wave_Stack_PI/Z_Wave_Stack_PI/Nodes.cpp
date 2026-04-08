
#include "Nodes.h"
#include "Notify.h"

Nodes::Nodes(EnqueueFn enqueue)
	: enqueue(std::move(enqueue))
{
}

Node* Nodes::Get(nodeid_t nodeid)
{
    auto it = nodes.find(nodeid);
    if (it != nodes.end())
        return it->second.get();  // Dereference the unique_ptr

    return nullptr;
}

const Node* Nodes::Get(nodeid_t nodeid) const
{
    auto it = nodes.find(nodeid);
    if (it != nodes.end())
        return it->second.get();  // Dereference the unique_ptr

    return nullptr;
}

Node* Nodes::GetOrCreate(nodeid_t nodeid)
{
    auto it = nodes.find(nodeid);
    if (it != nodes.end())
        return it->second.get();  // Dereference the unique_ptr

    auto n = std::make_unique<Node>(nodeid, enqueue);
    auto* pNode = n.get();
    nodes.emplace(nodeid, std::move(n));
    NotifyUI(UINotify::NodeListChanged, nodeid);
    return pNode;  // Dereference the pointer
}

bool Nodes::Exists(nodeid_t nodeid) const
{
    auto it = nodes.find(nodeid);
    return (it != nodes.end());
}

void Nodes::HandleCCDeviceReport(nodeid_t nodeid, eCommandClass cmdclass, ccid_t cmdid, const ccparams_t& cmdparams)
{
    // Dispatch a command-class report to the owning node/device.
    auto* n = Get(nodeid);
    if (!n)
    {
        Log.AddL(eLogTypes::DVC, MakeTag(), "HandleCCDeviceReport: node {} not found", nodeid);
        return;
    }

    n->HandleCCDeviceReport(cmdclass, cmdid, cmdparams);
}
