#pragma once

#include "Interface.h"
#include "Module.h"
#include "Nodes.h"
#include "APIFrame.h"
#include "InitializeManager.h"
#include "InterviewManager.h"
#include "CCDispatcher.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

class Z_Wave : private Interface
{
public:
	Z_Wave() :
		initializeManager([this](const ZW_APIFrame& f) { Enqueue(f); }, module),
		interviewManager([this](const ZW_APIFrame& f) { Enqueue(f); }, nodes),
		CCDispatcher([this](const ZW_APIFrame& f) { Enqueue(f); }, nodes)
	{}

	~Z_Wave()
	{
		ClosePort();
	}
	bool OpenPort(const std::string& portname)
	{
		if (Interface::OpenPort(portname))
		{
			StartJobWorker();
			return true;
		}
		return false;
	}
	void ClosePort()
	{
		StopJobWorker();
		Interface::ClosePort();
	}
	using Interface::IsSerialOpen;

	//******************************************************************
	// UI interface

	void StartInitialization()
	{
		if (module.InitializationState != ZW_Module::eInitializationState::NotInitialized)
			return;

		EnqueueJob(eJobs::INITIALIZE);
		/*
				initializeManager.Start();
				Log.AddL(eLogTypes::DVC, MakeTag(), "----------------------------------- Initialization done.");

				if(module.InitializationState == ZW_Module::eInitializationState::Initialized)
				StartInterview();
		*/
	}

	void StartInterview()
	{
		if (module.InitializationState != ZW_Module::eInitializationState::Initialized)
			return;

		for (size_t i = 0; i < module.NodeIds.size(); i++)
		{
			if (module.NodeIds[i] != (node_t)0 && module.NodeIds[i] != (node_t)1)
			{
				ZW_Node* node = nodes.GetOrCreate(module.NodeIds[i], [this](const ZW_APIFrame& f) { Enqueue(f); });
				if (node)
					node->SetInterviewState(ZW_Node::eInterviewState::NotInterviewed);
				EnqueueJob(eJobs::INTERVIEW, module.NodeIds[i]);
			}
		}
	}

	ZW_Module& GetModule() { return module; }
	ZW_Nodes& GetNodes() { return nodes; }

	std::string HostToString() { return module.ToString(); }

	std::string NodesToString(int width) const
	{
		return this->nodes.ToString(width);
	}

	void RequestBattery(node_t nodeid)
	{
		ZW_Node* node = nodes.Get(nodeid);
		if (!node) return;
		ZW_Node::Job job;
		job.job = ZW_Node::eJobs::BATTERY_GET;
		node->EnqueueJob(job);
	}

	void AssociationInterview(node_t nodeid)
	{
		ZW_Node* node = nodes.Get(nodeid);
		if (!node) return;
		ZW_Node::Job job;
		job.job = ZW_Node::eJobs::ASSOCIATION_INTERVIEW;
		node->EnqueueJob(job);
		job.job = ZW_Node::eJobs::MULTI_CHANNEL_ASSOCIATION_INTERVIEW;
		node->EnqueueJob(job);
	}

	void ConfigurationInterview(node_t nodeid, uint8_t param)
	{
		ZW_Node* node = nodes.Get(nodeid);
		if (!node) return;
		ZW_Node::Job job;
		job.job = ZW_Node::eJobs::CONFIGURATION_INTERVIEW;
		job.group = param; // config value to get
		job.value = 1; // num config to get
		node->EnqueueJob(job);
	}

	void ConfigurationInterviewAll(node_t nodeid)
	{
		ZW_Node* node = nodes.Get(nodeid);
		if (!node) return;
		ZW_Node::Job job;
		job.job = ZW_Node::eJobs::CONFIGURATION_INTERVIEW;
		job.group = 1;
		job.value = 255;
		node->EnqueueJob(job);
	}

	void IsDead(node_t nodeid)
	{
		ZW_APIFrame frame;
		frame.Make(eCommandIds::ZW_API_IS_NODE_FAILED, { nodeid.value, 0x0A });
		Enqueue(frame);
		//		EnqueueJob(eJobs::IS_DEAD, nodeid);
	}

	void Remove(node_t nodeid)
	{
		ZW_APIFrame frame;
		frame.Make(eCommandIds::ZW_API_REMOVE_FAILED_NODE, { nodeid.value, 0x0A });
		Enqueue(frame);
		StartInitialization();
	}

	void Bind(node_t nodeid, uint8_t groupid, node_t targetnodeid)
	{
		ZW_Node* node = nodes.Get(nodeid);
		if (!node) return;
		ZW_Node::Job job;
		job.job = ZW_Node::eJobs::BIND_COMMAND;
		job.group = groupid;
		job.nodeId = targetnodeid;
		node->EnqueueJob(job);
		AssociationInterview(nodeid);
	}

	void Unbind(node_t nodeid, uint8_t groupid, node_t targetnodeid)
	{
		ZW_Node* node = nodes.Get(nodeid);
		if (!node) return;
		ZW_Node::Job job;
		job.job = ZW_Node::eJobs::UNBIND_COMMAND;
		job.group = groupid;
		job.nodeId = targetnodeid;
		node->EnqueueJob(job);
		AssociationInterview(nodeid);
	}
	void MCBind(node_t nodeid, uint8_t groupid, node_t targetNodeid, uint8_t targetEndpoint)
	{
		ZW_Node* node = nodes.Get(nodeid);
		if (!node) return;
		if (node->HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION))
		{
			ZW_Node::Job job;
			job.job = ZW_Node::eJobs::MULTI_CHANNEL_BIND_COMMAND;
			job.group = groupid;
			job.nodeId = targetNodeid;
			job.endpoint = targetEndpoint;
			node->EnqueueJob(job);
		}
		else
			if (targetEndpoint == 0 && node->HasCC(eCommandClass::ASSOCIATION))
				Bind(nodeid, groupid, targetNodeid);
			else
				return;
		AssociationInterview(nodeid);
	}
	void MCUnbind(node_t nodeid, uint8_t groupid, node_t targetNodeid, uint8_t targetEndpoint)
	{
		ZW_Node* node = nodes.Get(nodeid);
		if (!node) return;
		if (node->HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION))
		{
			ZW_Node::Job job;
			job.job = ZW_Node::eJobs::MULTI_CHANNEL_UNBIND_COMMAND;
			job.group = groupid;
			job.nodeId = targetNodeid;
			job.endpoint = targetEndpoint;
			node->EnqueueJob(job);
		}
		else
			if (targetEndpoint == 0 && node->HasCC(eCommandClass::ASSOCIATION))
				Unbind(nodeid, groupid, targetNodeid);
			else
				return;
		AssociationInterview(nodeid);
	}

	void Configure(node_t nodeid, uint8_t param, uint32_t value, uint8_t size)
	{
		ZW_Node* node = nodes.Get(nodeid);
		if (!node) return;
		if (node->configurationInfo[param].valid) // only set if we have a valid value
		{
			ZW_Node::Job job;
			job.job = ZW_Node::eJobs::CONFIGURATION_COMMAND;
			job.group = param;
			job.value = value;
			job.cfgSize = (ZW_Node::eConfigSize)size;
			node->EnqueueJob(job);
		}
//		ConfigurationInterview(nodeid, param);
	}

protected:

	bool OnFrameReceived(const ZW_APIFrame& frame) override
	{
		Log.AddL(eLogTypes::RTU, MakeTag(), "<< {}", frame.Info());
		switch (frame.APICmd.CmdId)
		{
		case eCommandIds::FUNC_ID_GET_INIT_DATA:
		case eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES:
		case eCommandIds::FUNC_ID_GET_CAPABILITIES:
		case eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION:
		case eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY:
		case eCommandIds::FUNC_ID_GET_LIBRARY_VERSION:
		case eCommandIds::FUNC_ID_GET_LIBRARY_TYPE:
			Log.AddL(eLogTypes::DBG, MakeTag(), "<< route=initializeManager {}", frame.Info());
			return initializeManager.HandleFrame(frame);

		case eCommandIds::ZW_API_APPLICATION_UPDATE:
			{
				ApplicationUpdateEvent event = static_cast<ApplicationUpdateEvent>(frame.payload[0]);
				if (event == ApplicationUpdateEvent::UPDATE_STATE_NODE_INFO_RECEIVED
					|| event == ApplicationUpdateEvent::UPDATE_STATE_NODE_ADDED)
					EnqueueJob(eJobs::INTERVIEW, node_t{ frame.payload[1] });
			}
		case eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA:
		case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
			Log.AddL(eLogTypes::DBG, MakeTag(), "<< route=interviewManager {}", frame.Info());
			return interviewManager.HandleFrame(frame);

		case eCommandIds::ZW_API_CONTROLLER_SEND_DATA:
			//SendDataManager.HandleTransmitResult(frame.payload);
			if (frame.Type() == APIFrame::eFrameTypes::RES)
				Log.AddL(eLogTypes::RTU, MakeTag(), "<< route=sendData type=RES txStatus=0x{:02X} len={}", frame.payload[0], frame.payload.size());
			if (frame.Type() == APIFrame::eFrameTypes::REQ)
				Log.AddL(eLogTypes::RTU, MakeTag(), "<< route=sendData type=REQ sessionId={} txStatus=0x{:02X} len={}", frame.payload[0], frame.payload[1], frame.payload.size());
			return true;

		case eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER:
			Log.AddL(eLogTypes::DBG, MakeTag(), "<< route=ccDispatcher {}", frame.Info());
			CCDispatcher.HandleCCFrame(frame.payload);
			return true;

		case eCommandIds::ZW_API_IS_NODE_FAILED:
			nodes.HandleNodeFailed(frame.payload[0]);
			return true;

		case eCommandIds::ZW_API_REMOVE_FAILED_NODE:
		case eCommandIds::ZW_API_REMOVE_NODE_FROM_NETWORK:
			return true;
		}
		return false;
	}

	bool OnFrameReceivedTimeout(const ZW_APIFrame& frame) override
	{
		switch (frame.APICmd.CmdId)
		{
		case eCommandIds::FUNC_ID_GET_INIT_DATA:
		case eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES:
		case eCommandIds::FUNC_ID_GET_CAPABILITIES:
		case eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION:
		case eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY:
		case eCommandIds::FUNC_ID_GET_LIBRARY_VERSION:
		case eCommandIds::FUNC_ID_GET_LIBRARY_TYPE:
			Log.AddL(eLogTypes::DBG, MakeTag(), "<< route=initializeManager TIMEOUT {}", frame.Info());
			return initializeManager.HandleFrameTimeout(frame);

		case eCommandIds::ZW_API_APPLICATION_UPDATE:
		case eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA:
		case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
			Log.AddL(eLogTypes::DVC, MakeTag(), "<< route=interviewManager TIMEOUT {}", frame.Info());
			return interviewManager.HandleFrameTimeout(frame);

		case eCommandIds::ZW_API_CONTROLLER_SEND_DATA:
			return true;

		case eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER:
			Log.AddL(eLogTypes::DVC, MakeTag(), "<< route=ccDispatcher TIMEOUT {}", frame.Info());
			//CCDispatcher.HandleCCFrameTimeout(frame.payload);
			return true;
		}

		return false;
	}

private:
	ZW_Module module; // the API module instance
	ZW_Nodes nodes; // the node list

	ZW_InitializeManager initializeManager;
	ZW_InterviewManager interviewManager;
	ZW_CCDispatcher CCDispatcher;

	enum class eJobs
	{
		INITIALIZE,
		INTERVIEW,
	};
	struct Job
	{
		eJobs job;
		node_t nodeid;
		uint16_t attempts = 0;
	};

	std::mutex jobQueueMutex;
	std::vector<Job> jobQueue;

	std::thread jobWorker;
	std::atomic<bool> jobWorkerRunning{ false };
	std::chrono::steady_clock::time_point jobStartTime{};
	int jobAttempts = 0;
	static constexpr int kMaxJobAttempts = 5;
	static constexpr auto kJobTimeout = std::chrono::seconds(20);

	void EnqueueJob(eJobs job, node_t nodeid = (node_t)0)
	{
		std::scoped_lock lock(jobQueueMutex);
		jobQueue.push_back(Job{ job, nodeid });
	}

	bool TryPeekJob(Job& job)
	{
		std::scoped_lock lock(jobQueueMutex);
		if (jobQueue.empty())
			return false;
		job = jobQueue.front();
		return true;
	}

	void PopJob()
	{
		std::scoped_lock lock(jobQueueMutex);
		if (!jobQueue.empty())
			jobQueue.erase(jobQueue.begin());
	}

	enum class eJobResult
	{
		Pending,
		Done,
		Error
	};

	eJobResult DispatchJob(const Job& job)
	{
		switch (job.job)
		{
		case eJobs::INITIALIZE:
			//			 Start or continue initialization until it reports Done.
			if (module.InitializationState == ZW_Module::eInitializationState::NotInitialized)
				initializeManager.Start();
			//			else if (module.InitializationState == ZW_Module::eInitializationState::Paused)
				//			initializeManager.StartOrResume();
			else if (module.InitializationState == ZW_Module::eInitializationState::Initialized)
			{
				Log.AddL(eLogTypes::DVC, MakeTag(), "----------------------------------- Initialization done.");
				StartInterview();
				return eJobResult::Done;
			}
			return eJobResult::Pending;

		case eJobs::INTERVIEW:
			{
				ZW_Node* node;
				if (interviewManager.Done(job.nodeid))
				{
					if (node = nodes.Get(job.nodeid))
						Log.AddL(eLogTypes::DVC, MakeTag(), "----------------------------------- Interview pending. Node: {} {}", node->NodeId, node->IsListening() ? "Listening" : "Not listening");
					else
						Log.AddL(eLogTypes::DVC, MakeTag(), "----------------------------------- Interview done. Node: {}", job.nodeid);
					return eJobResult::Done;
				}
				if ((node = nodes.Get(job.nodeid)) && node->GetInterviewState() == ZW_Node::eInterviewState::NotInterviewed)
					Log.AddL(eLogTypes::DVC, MakeTag(), "----------------------------------- Interview started. Node: {}", node->NodeId);
				interviewManager.Start(job.nodeid);
			}
			return eJobResult::Pending;
		}
		return eJobResult::Error;
	}

	void HandleJobError(const Job& job)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "ZWave Job failed: {}", static_cast<int>(job.job));
		// Reset state so it can restart.
		if (job.job == eJobs::INITIALIZE)
		{
			//			initializeManager.Reset();
			EnqueueJob(eJobs::INITIALIZE);
		}
	}

	void JobWorkerLoop()
	{
		while (jobWorkerRunning.load())
		{
			Job job{};
			if (TryPeekJob(job))
			{
				// Track timing/attempts per-front-job.
				if (jobAttempts == 0)
					jobStartTime = std::chrono::steady_clock::now();

				const auto now = std::chrono::steady_clock::now();
				if (now - jobStartTime > kJobTimeout)
				{
					jobAttempts++;
					Log.AddL(eLogTypes::DVC, MakeTag(), "ZWave Job timeout: {} attempt {}", static_cast<int>(job.job), jobAttempts);
					jobStartTime = now;
					if (jobAttempts >= kMaxJobAttempts)
					{
						HandleJobError(job);
						PopJob();
						jobAttempts = 0;
					}
					std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(kJobTimeout) / kMaxJobAttempts);
					continue;
				}

				switch (DispatchJob(job))
				{
				case eJobResult::Done:
					PopJob();
					jobAttempts = 0;
					break;
				case eJobResult::Pending:
					//					jobAttempts++;
					std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(kJobTimeout) / (kMaxJobAttempts * 10));
					break;
				case eJobResult::Error:
					//				jobAttempts++;
					if (jobAttempts >= kMaxJobAttempts)
					{
						HandleJobError(job);
						PopJob();
						jobAttempts = 0;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(250));
					break;
				}
				continue;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	void StartJobWorker()
	{
		bool expected = false;
		if (!jobWorkerRunning.compare_exchange_strong(expected, true))
			return;

		jobWorker = std::thread([this]() { JobWorkerLoop(); });
	}

	void StopJobWorker()
	{
		jobWorkerRunning.store(false);
		if (jobWorker.joinable())
			jobWorker.join();
	}
};

