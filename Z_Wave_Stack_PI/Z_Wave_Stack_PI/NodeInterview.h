
#include "Nodes.h"

class NodeInterview
{
public:
	NodeInterview(Nodes& nodes, EnqueueFn enqueue);

	void Start(nodeid_t nodeid);
	bool Done(nodeid_t nodeid); const

	bool HandleFrame(const APIFrame& frame);
	bool HandleFrameTimeout(const APIFrame& frame);

private:
	Nodes& nodes;
	EnqueueFn enqueue;
	Node* activeNode;

	void HandleApplicationUpdate(const APIFrame& frame);

	void RequestNodeProtocolInfo(nodeid_t nodeid);
	void DecodeNodeProtocolInfo(nodeid_t nodeid, const APIFrame::PayLoad& payload);
	void RequestNodeInformation(nodeid_t nodeid);
	void DecodeNodeInfo(const APIFrame::PayLoad& payload);

};