#pragma once

#include "Interface.h"
#include "Module.h"
#include "Nodes.h"
#include "APIFrame.h"
#include "InitializeManager.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

class Z_Wave : private ZW_Interface
{
public:
	Z_Wave() : 
		initializeManager([this](const ZW_APIFrame& f) { Enqueue(f); }, module)
	{
	}

	~Z_Wave()
	{
		void ClosePort();
	}
	bool OpenPort(const std::string& portname)
	{
		if(ZW_Interface::OpenPort(portname))
		{
			StartJobWorker();
			return true;
		}
		return false;
	}
	void ClosePort() 
	{ 
		StopJobWorker();
		ZW_Interface::ClosePort(); 
	}

	//******************************************************************
	// UI interface

	void StartInitialization()
	{
		if (module.InitializationState != ZW_Module::eInitializationState::NotInitialized)
			return;

		EnqueueJob(eJobs::INITIALIZE);
	}

	void StartInterview()
	{
		if (module.InitializationState != ZW_Module::eInitializationState::Initialized)
			return;

		EnqueueJob(eJobs::INTERVIEW);
	}
	
	virtual std::string HostToString()
	{
		return module.ToString();
	}

	std::string NodesToString() const
	{
		return this->nodes.ToString();
	}

	void RequestBattery(uint16_t nodeid)
	{
		//UICommands.RequestBattery(nodeid);
	}

protected:
	
	bool OnFrameReceived(const ZW_APIFrame& frame) override
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
			Log.AddL(eLogTypes::DBG, MakeTag(), "<< initializeManager {}", frame.Info());
			return initializeManager.HandleFrame(frame);

		case eCommandIds::ZW_API_APPLICATION_UPDATE:
		case eCommandIds::ZW_API_GET_NODE_INFO_PROTOCOL_DATA:
		case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
		case eCommandIds::ZW_API_CONTROLLER_SEND_DATA:
			Log.AddL(eLogTypes::DBG, MakeTag(), "<< InterviewManager {}", frame.Info());
	//		return InterviewManager.FrameReceived(frame);
			return true;

		case eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER:
			Log.AddL(eLogTypes::DBG, MakeTag(), "<< CCDispatcher {}", frame.Info());
		//	CCDispatcher.HandleCCFrame(frame.payload);
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
			Log.AddL(eLogTypes::DBG, MakeTag(), "<< initializeManager timeout {}", frame.Info());
			return initializeManager.HandleFrameTimeout(frame);

		case eCommandIds::ZW_API_APPLICATION_UPDATE:
		case eCommandIds::ZW_API_REQUEST_NODE_INFORMATION:
		case eCommandIds::ZW_API_CONTROLLER_SEND_DATA:
			Log.AddL(eLogTypes::INFO, MakeTag(), "<< InterviewManager timeout {}", frame.Info());
	//		return InterviewManager.FrameReceivedTimeout(frame);
			return true;

		case eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER:
			Log.AddL(eLogTypes::INFO, MakeTag(), "<< CCDispatcher timeout {}", frame.Info());
//			CCDispatcher.HandleCCFrameTimeout(frame.payload);
			return true;
		}

		return false;
	}

private:
	ZW_Module module; // the API module instance
	ZW_Nodes nodes; // the node list

	ZW_InitializeManager initializeManager;

	enum class eJobs
	{
		INITIALIZE,
		INTERVIEW,
	};

	std::mutex jobQueueMutex;
	std::vector<eJobs> jobQueue;

	std::thread jobWorker;
	std::atomic<bool> jobWorkerRunning{ false };
	std::chrono::steady_clock::time_point jobStartTime{};
	int jobAttempts = 0;
	static constexpr int kMaxJobAttempts = 5;
	static constexpr auto kJobTimeout = std::chrono::seconds(5);

	void EnqueueJob(eJobs job)
	{
		std::scoped_lock lock(jobQueueMutex);
		jobQueue.push_back(job);
	}

	bool TryPeekJob(eJobs& job)
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

	eJobResult DispatchJob(eJobs job)
	{
		switch (job)
		{
		case eJobs::INITIALIZE:
			// Start or continue initialization until it reports Done.
			if (module.InitializationState == ZW_Module::eInitializationState::NotInitialized)
				initializeManager.Start();
			else if (module.InitializationState == ZW_Module::eInitializationState::Paused)
				initializeManager.Restart();
			else if (module.InitializationState == ZW_Module::eInitializationState::Initialized)
				return eJobResult::Done;

			return eJobResult::Pending;
		case eJobs::INTERVIEW:
			// TODO: kick off node interview logic
			return eJobResult::Done;
		}
		return eJobResult::Error;
	}

	void HandleJobError(eJobs job)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "Job failed: {}", static_cast<int>(job));
		// Reset state so it can restart.
		if (job == eJobs::INITIALIZE)
		{
			initializeManager.Reset();
			EnqueueJob(eJobs::INITIALIZE);
		}
	}

	void JobWorkerLoop()
	{
		while (jobWorkerRunning.load())
		{
			eJobs job{};
			if (TryPeekJob(job))
			{
				// Track timing/attempts per-front-job.
				if (jobAttempts == 0)
					jobStartTime = std::chrono::steady_clock::now();

				const auto now = std::chrono::steady_clock::now();
				if (now - jobStartTime > kJobTimeout)
				{
					jobAttempts++;
					Log.AddL(eLogTypes::INFO, MakeTag(), "Job timeout: {} attempt {}", static_cast<int>(job), jobAttempts);
					jobStartTime = now;
					if (jobAttempts >= kMaxJobAttempts)
					{
						HandleJobError(job);
						PopJob();
						jobAttempts = 0;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(250));
					continue;
				}

				switch (DispatchJob(job))
				{
				case eJobResult::Done:
					PopJob();
					jobAttempts = 0;
					break;
				case eJobResult::Pending:
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					break;
				case eJobResult::Error:
					jobAttempts++;
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

