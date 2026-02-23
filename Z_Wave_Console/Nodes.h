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

	ZW_Node* Get(uint16_t nodeid);
	const ZW_Node* Get(uint16_t nodeid) const;
	ZW_Node* GetOrCreate(uint16_t nodeid);
	bool Exists(uint16_t nodeid) const;

	ZW_Node::CommandClassTag* GetCC(uint16_t nodeid, eCommandClass ccid);
	const ZW_Node::CommandClassTag* GetCC(uint16_t nodeid, eCommandClass ccid) const;
	bool HasCC(uint16_t nodeid, eCommandClass ccid) const;

	std::string ToString() const;

	size_t Size() const { return nodes.size(); }
	auto begin() { return nodes.begin(); }
	auto end() { return nodes.end(); }
	auto begin() const { return nodes.begin(); }
	auto end() const { return nodes.end(); }

	void HandleCCDeviceReport(uint16_t nodeid, uint8_t cmdclass, uint8_t cmdid, const std::vector<uint8_t>& cmdparams);

private:
	std::unordered_map<uint16_t, std::unique_ptr<ZW_Node>> nodes;
};
