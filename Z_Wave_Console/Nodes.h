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

	ZW_Node* Get(uint8_t nodeid);
	const ZW_Node* Get(uint8_t nodeid) const;
	ZW_Node* GetOrCreate(uint8_t nodeid, EnqueueFn enqueue);
	bool Exists(uint8_t nodeid) const;

	ZW_Node::CommandClassTag* GetCC(uint8_t nodeid, eCommandClass ccid);
	const ZW_Node::CommandClassTag* GetCC(uint8_t nodeid, eCommandClass ccid) const;
	bool HasCC(uint8_t nodeid, eCommandClass ccid) const;

	std::string ToString(int width) const;

	size_t Size() const { return nodes.size(); }
	auto begin() { return nodes.begin(); }
	auto end() { return nodes.end(); }
	auto begin() const { return nodes.begin(); }
	auto end() const { return nodes.end(); }

	void HandleCCDeviceReport(uint8_t nodeid, eCommandClass cmdclass, ZW_CmdId cmdid, const ZW_ByteVector& cmdparams);
	void HandleNodeFailed(uint8_t status);

private:
	std::unordered_map<uint8_t, std::unique_ptr<ZW_Node>> nodes;
};

