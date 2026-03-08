#pragma once


#include <chrono>
#include <vector>
#include <thread>

#include "Logging.h"
#include "APIFrame.h"
#include "Nodes.h"
#include "Node.h"

class ZW_InterviewManager
{
public:
	ZW_InterviewManager(EnqueueFn enqueue, ZW_Nodes& nodes) :
		nodes(nodes),
		enqueue(enqueue),
		activeNode(nullptr)
	{};

	void Start(uint8_t nodeid)
	{
		ZW_Node* node = nodes.GetOrCreate(nodeid, enqueue);
		if (!node) return;

		switch (node->GetInterviewState())
		{
		case ZW_Node::eInterviewState::NotInterviewed:
			RequestNodeProtocolInfo(nodeid);
			break;

			//case ZW_Node::eInterviewState::ProtocolInfoDone:	// handled in HandleFrame
			//case ZW_Node::eInterviewState::NodeInfoDone:		// handled in InteviewManager
			//case ZW_Node::eInterviewState::CCVersionDone:		// handled in Device
			//case ZW_Node::eInterviewState::CCMnfcSpecDone:	// handled in Device
			// CCMultiChannelPending, CCMultiChannelDone // handled in 

		case ZW_Node::eInterviewState::CCMultiChannelDone:
			node->SetInterviewState(ZW_Node::eInterviewState::InterviewDone);
			ZW_Node::Job job;
			job.job = ZW_Node::eJobs::ASSOCIATION_INTERVIEW;
			node->EnqueueJob(job);
			job.job = ZW_Node::eJobs::MULTI_CHANNEL_ASSOCIATION_INTERVIEW;
			node->EnqueueJob(job);
			job.job = ZW_Node::eJobs::CONFIGURATION_INTERVIEW;
			node->EnqueueJob(job);
			break;
		}
	}

	bool Done(uint8_t nodeid) const
	{
		const ZW_Node* node = nodes.Get(nodeid);
		if (node)
		{
			if (!node->IsListening() && node->GetInterviewState() == ZW_Node::eInterviewState::ProtocolInfoDone)
				return true;

			if (node->IsListening() && node->GetInterviewState() == ZW_Node::eInterviewState::InterviewDone)
				return true;
		}
		return false;
	}

	bool HandleFrame(const ZW_APIFrame& frame)
	{
		Log.AddL(eLogTypes::DBG, MakeTag(), "HandleFrame called for node {}: {}", activeNode ? activeNode->NodeId : 0, frame.Info());

		switch (frame.APICmd.CmdId)
		{
		case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
			// ignore
			return true;
		case eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA:
			if (activeNode == nullptr)
			{
				Log.AddL(eLogTypes::ERR, MakeTag(), "<< NODE_PROTOCOL_INFO: unexpected response (no current node)");
				return true;
			}
			DecodeNodeProtocolInfo(activeNode->NodeId, frame.payload);
			if (activeNode->GetInterviewState() == ZW_Node::eInterviewState::ProtocolInfoDone && activeNode->IsListening())
				RequestNodeInformation(activeNode->NodeId);
			return true;
		case eCommandIds::ZW_API_APPLICATION_UPDATE:
			HandleApplicationUpdate(frame);
			return true;
		}

		return false;

	}

	bool HandleFrameTimeout(const ZW_APIFrame& frame)
	{
		Log.AddL(eLogTypes::INFO, MakeTag(), "HandleFrameTimeout called for node {}: {}",
				 activeNode ? activeNode->NodeId : 0, frame.Info());
		return false;
	}

	void HandleApplicationUpdate(const ZW_APIFrame& frame)
	{
		const auto& p = frame.payload;
		if (p.size() < 2)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "<< payload too short: {}", frame.Info());
			return;
		}

		ApplicationUpdateEvent event = static_cast<ApplicationUpdateEvent>(p[0]);

		switch (event)
		{
		case ApplicationUpdateEvent::UPDATE_STATE_NODE_INFO_RECEIVED:
			DecodeNodeInfo(p);
			break;

		case ApplicationUpdateEvent::UPDATE_STATE_NODE_INFO_REQ_FAILED:
			Log.AddL(eLogTypes::ERR, MakeTag(), "<< {}: event={} payloadLen={}", ToString(eCommandIds::ZW_API_APPLICATION_UPDATE), ToString(event), p.size());
			if (activeNode != nullptr)
			{
				ZW_Node* node = activeNode;
				if (node && node->GetInterviewState() == ZW_Node::eInterviewState::NodeInfoPending && node->IsListening())
					node->SetInterviewState(ZW_Node::eInterviewState::NodeInfoPending);
			}
			break;

		default:
			Log.AddL(eLogTypes::DBG, MakeTag(), "<< {}: unexpected event=0x{:02X} payloadLen={}", ToString(eCommandIds::ZW_API_APPLICATION_UPDATE), static_cast<uint8_t>(event), p.size());
			break;
		}

	}

private:
	EnqueueFn enqueue;
	ZW_Nodes& nodes;
	ZW_Node* activeNode = nullptr;

	void RequestNodeProtocolInfo(uint8_t nodeid);
	void DecodeNodeProtocolInfo(uint8_t nodeid, const std::vector<uint8_t>& payload);
	void RequestNodeInformation(uint8_t nodeid);
	void DecodeNodeInfo(const std::vector<uint8_t>& payload);
};

