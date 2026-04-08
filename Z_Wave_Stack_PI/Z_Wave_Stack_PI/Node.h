#pragma once

#include "Logging.h"
#include "NodeInfo.h"
#include "APIFrame.h"
#include "CommandClass.h"

class Node : public NodeInfo
{
public:
	Node(nodeid_t id, EnqueueFn enqueue)
		: NodeInfo(id)
		, enqueue(enqueue)
	{
		Start();
	}
	~Node() { Stop(); }

	void SendFrame(const APIFrame& frame) override
	{
		enqueue(frame);
	}

	void Start()
	{
		bool expected = false;
		if (!running.compare_exchange_strong(expected, true))
			return;

		worker = std::thread([this]() { WorkerTask(); });
	}

	void Stop()
	{
		running.store(false);
		if (worker.joinable())
			worker.join();
	}

	void WakeUp()
	{
       bool notify = false;
		{
            DebugLockGuard lock(stateMutex);
			if (nodeState != eNodeState::Awake)
			{
				nodeState = eNodeState::Awake;
                notify = true;
			}
		}
       if (notify)
			NotifyUI(UINotify::NodeChanged, nodeId);
	}

	void Sleeping()
	{
        bool notify = false;
		{
			DebugLockGuard lock(stateMutex);
			if (!protocolInfo.IsListening() && nodeState != eNodeState::Sleepy)
			{
				nodeState = eNodeState::Sleepy;
				notify = true;
			}
		}
		if (notify)
		{
			NotifyUI(UINotify::NodeChanged, nodeId);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// jobs
	enum class eJobs
	{
		BATTERY_GET,
		SWITCH_BINARY,
		ASSOCIATION_INTERVIEW,
		MULTI_CHANNEL_ASSOCIATION_INTERVIEW,
		CONFIGURATION_INTERVIEW,
		BIND_COMMAND,
		UNBIND_COMMAND,
		MULTI_CHANNEL_UNBIND_COMMAND,
		MULTI_CHANNEL_BIND_COMMAND,
		CONFIGURATION_COMMAND
	};

	enum class eConfigSize : uint8_t { OneByte = 1, TwoBytes = 2, FourBytes = 4 };
	struct Job
	{
		eJobs job;
		uint8_t group;
		nodeid_t nodeId;
		uint8_t endpoint;

		uint32_t value;
		eConfigSize cfgSize = eConfigSize::OneByte;
	};
	void EnqueueJob(Job job)
	{
		if (!SupportsJob(job.job))
		{
			Log.AddL(eLogTypes::DVC, MakeTag(), "Node {} does not support job: {}", nodeId, (uint8_t)job.job);
			return;
		}

		jobQueue.push_back(job);
		NotifyUI(UINotify::NodeListChanged, nodeId);
		Log.AddL(eLogTypes::DVC, MakeTag(), "EnqueueJob: num={}", jobQueue.size());
	}

	void HandleCCDeviceReport(eCommandClass cmdClass, ccid_t cmdId, const ccparams_t& cmdParams);

private:
	EnqueueFn enqueue;

	/////////////////////////////////////////////////////////////////////////////////
	// jobs
	std::thread worker;
	std::atomic<bool> running{ false };

	std::vector<Job> jobQueue;

	bool SupportsJob(eJobs type)
	{
		switch (type)
		{
		case eJobs::BATTERY_GET:
			return HasCC(eCommandClass::BATTERY);

		case eJobs::SWITCH_BINARY:
			return HasCC(eCommandClass::SWITCH_BINARY);

		case eJobs::CONFIGURATION_INTERVIEW:
		case eJobs::CONFIGURATION_COMMAND:
			return HasCC(eCommandClass::CONFIGURATION);

		case eJobs::ASSOCIATION_INTERVIEW:
		case eJobs::BIND_COMMAND:
		case eJobs::UNBIND_COMMAND:
			return HasCC(eCommandClass::ASSOCIATION);

		case eJobs::MULTI_CHANNEL_ASSOCIATION_INTERVIEW:
		case eJobs::MULTI_CHANNEL_UNBIND_COMMAND:
		case eJobs::MULTI_CHANNEL_BIND_COMMAND:
			return HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION);
		default:
			Log.AddL(eLogTypes::DVC, MakeTag(), "Unknown job: {}", (uint8_t)type);
			return true;
		}
	}

	bool TryPeekJob(Job& job)
	{
//		DebugLockGuard lock(stateMutex);
		if (jobQueue.empty())
		{
			//			Log.AddL(eLogTypes::DVC, MakeTag(), "Job queue empty");
			return false;
		}
		job = jobQueue.front();
		Log.AddL(eLogTypes::DVC, MakeTag(), "Job peeked: {}", (uint8_t)job.job);
		return true;
	}

	void PopJob()
	{
//		DebugLockGuard lock(stateMutex);
		if (!jobQueue.empty())
			jobQueue.erase(jobQueue.begin());
		NotifyUI(UINotify::NodeListChanged, nodeId);
		Log.AddL(eLogTypes::DVC, MakeTag(), "Job erased queue size: {}", jobQueue.size());
	}

	template <typename Pred>
	bool WaitUntil(std::chrono::milliseconds timeout, Pred&& predicate)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (!predicate())
		{
			if (std::chrono::steady_clock::now() >= deadline)
				return false;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return true;
	}

	void WorkerTask()
	{
		auto awakeIdleSince = std::chrono::steady_clock::time_point{};
		auto deadIdleSince = std::chrono::steady_clock::now();
		while (running.load())
		{
			// Wait for node to be awake or running to be false
			while (true)
			{
				if (std::chrono::steady_clock::now() - deadIdleSince >= std::chrono::seconds(10))
				{
					//					ProcessIsDead();
					deadIdleSince = std::chrono::steady_clock::now();
				}

				if (GetState() == eNodeState::Awake || !running.load())
				{
					deadIdleSince = std::chrono::steady_clock::now();
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}

			if (!running.load())
				break;

			if (GetState() == eNodeState::Awake)
			{
				if (awakeIdleSince.time_since_epoch().count() == 0)
					awakeIdleSince = std::chrono::steady_clock::now();

				if (GetInterviewState() != eInterviewState::InterviewDone)
				{
					ProcessInterviewState();
					awakeIdleSince = std::chrono::steady_clock::now();
					continue;
				}

				Job job;
				if (TryPeekJob(job))
				{
					ExecuteJob(job);
					awakeIdleSince = std::chrono::steady_clock::now();
					continue;
				}

				if (std::chrono::steady_clock::now() - awakeIdleSince >= std::chrono::seconds(5))
				{
					Sleeping();
					awakeIdleSince = {};
					continue;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}

		}
	}

	void ExecuteJob(Job job)
	{
		bool doneOrError = true;
		switch (job.job)
		{
		case eJobs::BATTERY_GET:
			doneOrError = ExecuteBatteryCommandJob();
			break;
		case eJobs::SWITCH_BINARY:
			doneOrError = ExecuteSwitchBinaryCommandJob(static_cast<uint8_t>(job.value & 0xFF));
			break;
		case eJobs::ASSOCIATION_INTERVIEW:
			doneOrError = ExecuteAssociationInterviewJob();
			break;
		case eJobs::MULTI_CHANNEL_ASSOCIATION_INTERVIEW:
			doneOrError = ExecuteMultiChannelAssociationInterviewJob();
			break;
		case eJobs::CONFIGURATION_INTERVIEW:
			doneOrError = ExecuteConfigurationInterviewJob(job.group, (uint8_t)(job.value & 0xFF));
			break;
		case eJobs::BIND_COMMAND:
			doneOrError = ExecuteBindCommandJob(job.group, job.nodeId);
			break;
		case eJobs::UNBIND_COMMAND:
			doneOrError = ExecuteUnBindCommandJob(job.group, job.nodeId);
			break;
		case eJobs::MULTI_CHANNEL_UNBIND_COMMAND:
			doneOrError = ExecuteMultiChannelUnBindCommandJob(job.group, job.nodeId, job.endpoint);
			break;
		case eJobs::MULTI_CHANNEL_BIND_COMMAND:
			doneOrError = ExecuteMultiChannelBindCommandJob(job.group, job.nodeId, job.endpoint);
			break;
		case eJobs::CONFIGURATION_COMMAND:
			doneOrError = ExecuteConfigurationCommandJob(job.group, job.cfgSize, job.value);
			break;
		default:
			Log.AddL(eLogTypes::DVC, MakeTag(), "Unknown job: {}", (uint8_t)job.job);
			break;
		}

		if (doneOrError)
		{
			PopJob();
		}
		else // timeout
		{
			Sleeping();
		}
	}


	void ProcessInterviewState();

	bool ExecuteBatteryCommandJob();
	bool ExecuteSwitchBinaryCommandJob(uint8_t value);

	bool ExecuteAssociationInterviewJob();
	bool ExecuteMultiChannelAssociationInterviewJob();
	bool ExecuteConfigurationInterviewJob(uint8_t startparam, uint8_t numParams);

	bool ExecuteBindCommandJob(uint8_t groupId, nodeid_t nodeid);
	bool ExecuteUnBindCommandJob(uint8_t groupId, nodeid_t nodeid);
	bool ExecuteMultiChannelUnBindCommandJob(uint8_t groupId, nodeid_t nodeid, uint8_t endpoint);
	bool ExecuteMultiChannelBindCommandJob(uint8_t groupId, nodeid_t nodeid, uint8_t endpoint);

	bool ExecuteConfigurationCommandJob(uint8_t paramNumber, eConfigSize size, uint32_t value);

};