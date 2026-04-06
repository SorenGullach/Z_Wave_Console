#pragma once

#include "Node.h"
#include <unordered_map>

class Nodes
{	
public:
	Nodes(EnqueueFn enqueue);

	Node* Get(nodeid_t nodeid);
	const Node* Get(nodeid_t nodeid) const;
	Node* GetOrCreate(nodeid_t nodeid);
	bool Exists(nodeid_t nodeid) const;

private:
	EnqueueFn enqueue;

	std::unordered_map<nodeid_t, std::unique_ptr<Node>> nodes;
};