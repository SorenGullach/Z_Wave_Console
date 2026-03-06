

#include "Nodes.h"
#include "Node.h"

#include <utility>

ZW_Node* ZW_Nodes::Get(uint16_t nodeid)
{
	auto it = nodes.find(nodeid);
	if (it == nodes.end())
		return nullptr;
	return it->second.get();
}

const ZW_Node* ZW_Nodes::Get(uint16_t nodeid) const
{
	auto it = nodes.find(nodeid);
	if (it == nodes.end())
		return nullptr;
	return it->second.get();
}

ZW_Node* ZW_Nodes::GetOrCreate(uint16_t nodeid, EnqueueFn enqueue)
{
    auto it = nodes.find(nodeid);
    if (it != nodes.end())
        return it->second.get();

    auto n = std::make_unique<ZW_Node>(nodeid, enqueue);
    auto* raw = n.get();
    nodes.emplace(nodeid, std::move(n));
    return raw;
}

bool ZW_Nodes::Exists(uint16_t nodeid) const
{
	return nodes.find(nodeid) != nodes.end();
}

ZW_Node::CommandClassTag* ZW_Nodes::GetCC(uint16_t nodeid, eCommandClass ccid)
{
	// Returns the CC tag for a node (even if unsupported). Returns nullptr if node doesn't exist.
	auto* n = Get(nodeid);
	if (!n)
		return nullptr;
	return &n->ccs[static_cast<uint8_t>(ccid)];
}

const ZW_Node::CommandClassTag* ZW_Nodes::GetCC(uint16_t nodeid, eCommandClass ccid) const
{
	// Const overload.
	auto* n = Get(nodeid);
	if (!n)
		return nullptr;
	return &n->ccs[static_cast<uint8_t>(ccid)];
}

bool ZW_Nodes::HasCC(uint16_t nodeid, eCommandClass ccid) const
{
	// True only if node exists and CC is marked supported.
	auto* tag = GetCC(nodeid, ccid);
	return tag != nullptr && tag->supported;
}

std::string ZW_Nodes::ToString() const
{
	// Human readable dump.
	std::string out;
	for (const auto& kvp : nodes)
	{
		const auto& n = *kvp.second;
		if (n.NodeId == 0)
			continue;
		out += n.ToString();
		out += "\n";
	}
	return out;
}

void ZW_Nodes::HandleCCDeviceReport(uint16_t nodeid, eCommandClass cmdclass, ZW_CmdId cmdid, const ZW_ByteVector& cmdparams)
{
	// Dispatch a command-class report to the owning node/device.
	auto* n = Get(nodeid);
	if (!n)
	{
		Log.AddL(eLogTypes::INFO, MakeTag(), "HandleCCDeviceReport: node {} not found", nodeid);
		return;
	}
	n->HandleCCDeviceReport(cmdclass, cmdid, cmdparams);
}

