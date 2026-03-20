#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "Node.h"


class ZW_Nodes
{
private:

public:
	ZW_Nodes() {}

	ZW_Node* Get(node_t nodeid);
	const ZW_Node* Get(node_t nodeid) const;
	ZW_Node* GetOrCreate(node_t nodeid, EnqueueFn enqueue);
	bool Exists(node_t nodeid) const;

	ZW_Node::CommandClassTag* GetCC(node_t nodeid, eCommandClass ccid);
	const ZW_Node::CommandClassTag* GetCC(node_t nodeid, eCommandClass ccid) const;
	bool HasCC(node_t nodeid, eCommandClass ccid) const;

	std::string ToString(int width) const;

	size_t Size() const { return nodes.size(); }
	auto begin() { return nodes.begin(); }
	auto end() { return nodes.end(); }
	auto begin() const { return nodes.begin(); }
	auto end() const { return nodes.end(); }

	void HandleCCDeviceReport(node_t nodeid, eCommandClass cmdclass, ZW_CmdId cmdid, const ZW_ByteVector& cmdparams);
	void HandleNodeFailed(uint8_t status);

private:
	std::unordered_map<node_t, std::unique_ptr<ZW_Node>> nodes;
};

