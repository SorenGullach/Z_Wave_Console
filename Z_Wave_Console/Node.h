#pragma once


#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>
#include <sstream>
#include <iomanip>
#include <utility>
#include "DebugMutex.h"
#include "Logging.h"
#include "Device.h"
#include "APIFrame.h"
#include "CommandClass.h"
#include "NodeId.h"


class ZW_NodeInfo
{
public:
	// ===================== Enums =====================
	enum class eInterviewState
	{
		NotInterviewed, ProtocolInfoPending, ProtocolInfoDone, NodeInfoPending, NodeInfoDone,
		CCVersionPending, CCVersionDone, CCMnfcSpecPending, CCMnfcSpecDone, CCMultiChannelPending, CCMultiChannelDone, InterviewDone,
	};
	enum class eNodeState { New, Awake, Sleepy };

	// ===================== Members =====================
	const node_t NodeId;
	std::string Floor = "Floor";
	std::string Room = "Room";

	// ---------- Protocol DVC ----------
	struct ProtocolInfo
	{
		uint8_t basic = 0;
		uint8_t generic = 0;
		uint8_t specific = 0;
		bool isListening = false;
		bool isRouting = false;
		uint8_t supportedSpeed = 0;
		uint8_t protocolVersion = 0;
		bool optionalFunctionality = false;
		bool sensor1000ms = false;
		bool sensor250ms = false;
		bool beamCapable = false;
		bool routingEndNode = false;
		bool specificDevice = false;
		bool controllerNode = false;
		bool security = false;
	} protocolInfo;
	void SetProtocolInfo(const ProtocolInfo& DVC)
	{
		DebugLockGuard lock(stateMutex);
		protocolInfo = DVC;
		NotifyUI(UINotify::NodeChanged, NodeId);
	}
	bool IsListening() const
	{
		DebugLockGuard lock(stateMutex);
		return protocolInfo.isListening;
	}

	// ---------- WakeUp DVC ----------
	struct WakeUpInfo
	{
		std::chrono::system_clock::time_point lastWakeUp;
		uint32_t wakeUpInterval = 0;
		uint32_t wakeUpMin = 0;
		uint32_t wakeUpMax = 0;
		uint32_t wakeUpDefault = 0;
		bool hasLastReport = false;
	} wakeUpInfo;
	void SetWakeUpInfo(const WakeUpInfo& wakeup)
	{
		DebugLockGuard lock(stateMutex);
		wakeUpInfo = wakeup;
		NotifyUI(UINotify::NodeChanged, NodeId);
	}
	WakeUpInfo GetWakeUpInfo() const
	{
		DebugLockGuard lock(stateMutex);
		return wakeUpInfo;
	}

	// ---------- Manufacturer DVC ----------
	struct ManufacturerInfo
	{
		uint16_t mfgId = 0;
		uint16_t prodType = 0;
		uint16_t prodId = 0;
		uint8_t deviceIdType = 0;
		uint8_t deviceIdFormat = 0;
		bool hasDeviceId = false;
		bool hasManufacturerData = false;
	} manufacturerInfo;

	// ---------- Meter DVC ----------
	struct MeterInfo
	{
		bool hasValue = false;
		uint8_t meterType = 0;
		uint8_t value = 0;
	} meterInfo;

	// ---------- Configuration DVC (CC 0x70) ----------
	struct ConfigurationInfo
	{
		uint8_t paramNumber = 0;          // Parameter number (1..255)
		uint8_t size = 0;                 // Size in bytes (1..4)
		int32_t value = 0;                // Signed big-endian interpreted value
		bool valid = false;               // True when a REPORT has been received
	};
	std::array<ConfigurationInfo, 255> configurationInfo{};

	// ---------- Association DVC (CC 0x85) ----------
	struct AssociationGroupNode
	{
		bool valid = false;                // True when a REPORT has been received
		node_t nodeId{};
	};
	struct AssociationGroup
	{
		uint8_t groupId = 0;
		uint8_t maxNodes = 0;
		std::array<AssociationGroupNode, 255> nodeList;     // normale node-id'er
		bool hasLastReport = false;
	};
	std::vector<AssociationGroup> associationGroups;

	// ---------- Multi Channel Endpoints (CC 0x60) ----------
	struct EndpointInfo
	{
		uint8_t endpointId = 0;
		uint8_t generic = 0;
		uint8_t specific = 0;
		std::vector<uint8_t> supportedCCs; // CC-liste for endpoint
		bool hasCapabilityReport = false;
	};

	struct MultiChannelEndpointInfo
	{
		uint8_t endpointCount = 0;
		bool hasEndpointReport = false;
		std::vector<EndpointInfo> endpoints;
	};
	MultiChannelEndpointInfo multiChannel;

	// ---------- Multi Channel Association (CC 0x8E) ----------
	struct MultiChannelAssociationMember
	{
		node_t nodeId{};
		uint8_t endpointId = 0; // 0 = root endpoint
		bool valid = false;
	};

	struct MultiChannelAssociationGroup
	{
		uint8_t groupId = 0;
		uint8_t maxNodes = 0;
		std::array<MultiChannelAssociationMember, 255> members;
		bool hasLastReport = false;
	};
	std::vector<MultiChannelAssociationGroup> multiChannelAssociationGroups;

	// ---------- Command Class Table ----------
	struct CommandClassTag
	{
		eCommandClass id = static_cast<eCommandClass>(0);
		bool supported = false;
		uint8_t version = 0;
		bool versionOk = false;
	};
	std::array<CommandClassTag, 256> ccs{};
	CommandClassTag* GetCC(eCommandClass ccId)
	{
		auto& cc = ccs[static_cast<uint8_t>(ccId)];
		return cc.supported ? &cc : nullptr;
	}
	const CommandClassTag* GetCC(eCommandClass ccId) const
	{
		const auto& cc = ccs[static_cast<uint8_t>(ccId)];
		return cc.supported ? &cc : nullptr;
	}
	bool HasCC(eCommandClass ccId) const
	{
		return ccs[static_cast<uint8_t>(ccId)].supported;
	}
	std::vector<eCommandClass> GetSupportedCCs() const
	{
		std::vector<eCommandClass> result;
		for (const auto& cc : ccs)
			if (cc.supported)
				result.push_back(cc.id);
		return result;
	}

	// ---------- CC Values ----------
	std::optional<uint8_t> batteryLevel;
	std::optional<uint8_t> basicValue;
	std::optional<uint8_t> switchBinaryValue;
	std::optional<uint8_t> switchMultilevelValue;
	std::optional<uint8_t> sensorBinaryValue;
	std::optional<uint8_t> protectionState;

	std::string NodeStateString() const;
	std::string InterviewStateString() const;

protected:
	mutable DebugMutex stateMutex;
	eNodeState nodeState = eNodeState::New;
	eInterviewState interviewState = eInterviewState::NotInterviewed;
	ZW_Device device;

	bool supportsCC(eCommandClass cc) const;

public:
	// ===================== Constructors =====================
	ZW_NodeInfo(node_t nodeid)
		: NodeId{ nodeid }, device(*this)
	{
		int i = 0;
		for (auto& ci : configurationInfo)
		{
			ci.paramNumber = i++;
			ci.size = 1;
			ci.value = 0;
			ci.valid = false;
		}
	}
	ZW_NodeInfo(const ZW_NodeInfo&) = delete;
	ZW_NodeInfo& operator=(const ZW_NodeInfo&) = delete;
	ZW_NodeInfo(ZW_NodeInfo&&) noexcept = default;
	ZW_NodeInfo& operator=(ZW_NodeInfo&&) noexcept = default;

	// ===================== State/Interview =====================
	void SetInterviewState(eInterviewState state)
	{
		DebugLockGuard lock(stateMutex);
		interviewState = state;
		NotifyUI(UINotify::NodeChanged, NodeId);
	}
	eInterviewState GetInterviewState() const
	{
		DebugLockGuard lock(stateMutex);
		return interviewState;
	}
	eNodeState GetState() const
	{
		DebugLockGuard lock(stateMutex);
		return nodeState;
	}
	virtual void WakeUp()
	{
		DebugLockGuard lock(stateMutex);
		nodeState = eNodeState::Awake;
		NotifyUI(UINotify::NodeChanged, NodeId);
	}
	virtual void Sleeping()
	{
		DebugLockGuard lock(stateMutex);
		nodeState = eNodeState::Sleepy;
		NotifyUI(UINotify::NodeChanged, NodeId);
	}

	// ---------- NIF/CC Handler ----------
	void SetNIF(uint8_t basicType, uint8_t genericClass, uint8_t specificClass,
				const std::vector<uint8_t>& commandClasses)
	{
		DebugLockGuard lock(stateMutex);
		protocolInfo.basic = basicType;
		protocolInfo.generic = genericClass;
		protocolInfo.specific = specificClass;
		ccs.fill({});
		for (auto cc : commandClasses)
		{
			ccs[cc].id = static_cast<eCommandClass>(cc);
			ccs[cc].supported = supportsCC(ccs[cc].id);
		}
		if (HasCC(eCommandClass::BASIC)) device.AddHandler<ZW_CC_Basic>();
		if (HasCC(eCommandClass::VERSION)) device.AddHandler<ZW_CC_Version>();
		if (HasCC(eCommandClass::MANUFACTURER_SPECIFIC)) device.AddHandler<ZW_CC_ManufacturerSpecific>();
		if (HasCC(eCommandClass::BATTERY)) device.AddHandler<ZW_CC_Battery>();
		if (HasCC(eCommandClass::SWITCH_BINARY)) device.AddHandler<ZW_CC_SwitchBinary>();
		if (HasCC(eCommandClass::BASIC)) device.AddHandler<ZW_CC_Basic>();
		if (HasCC(eCommandClass::SWITCH_MULTILEVEL)) device.AddHandler<ZW_CC_SwitchMultilevel>();
		if (HasCC(eCommandClass::SENSOR_BINARY)) device.AddHandler<ZW_CC_SensorBinary>();
		if (HasCC(eCommandClass::METER)) device.AddHandler<ZW_CC_Meter>();
		if (HasCC(eCommandClass::MULTI_CHANNEL)) device.AddHandler<ZW_CC_MultiChannel>();
		if (HasCC(eCommandClass::CONFIGURATION)) device.AddHandler<ZW_CC_Configuration>();
		if (HasCC(eCommandClass::PROTECTION)) device.AddHandler<ZW_CC_Protection>();
		if (HasCC(eCommandClass::ASSOCIATION)) device.AddHandler<ZW_CC_Association>();
		if (HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION)) device.AddHandler<ZW_CC_MultiChannelAssociation>();
		if (HasCC(eCommandClass::WAKE_UP)) device.AddHandler<ZW_CC_WakeUp>();
		NotifyUI(UINotify::NodeChanged, NodeId);
	}

	// ---------- CC Dispatch ----------
	void HandleCCDeviceReport(eCommandClass cmdClass, ZW_CmdId cmdId, const ZW_ByteVector& cmdParams)
	{
		ZW_ByteVector innerParams = cmdParams;
		uint8_t destiationEndpoint = 0;
		if (cmdClass == eCommandClass::MULTI_CHANNEL &&
			cmdId.value == static_cast<uint8_t>(ZW_CC_MultiChannel::eMultiChannelCommand::MULTI_CHANNEL_CMD_ENCAP))
		{
			// pull out the encapsulated command
			// | 0x60 | 0x06 | DE | CC | CMD | PAYLOAD...
			if (cmdParams.size() >= 3)
			{
				destiationEndpoint = cmdParams[0];
				cmdClass = static_cast<eCommandClass>(cmdParams[1]);
				cmdId = ZW_CmdId(cmdParams[2]);
				innerParams.assign(cmdParams.begin() + 3, cmdParams.end());
				Log.AddL(eLogTypes::DVC, MakeTag(),
						 "Encapsulated command class: 0x{:02X} {} (to endpoint {})",
						 (uint8_t)cmdClass, CommandClassToString(cmdClass), destiationEndpoint);
			}
			else
			{
				Log.AddL(eLogTypes::ERR, MakeTag(), "Malformed multi-channel encapsulated command");
			}
		}
		auto handler = device.GetHandler(cmdClass);
		if (handler)
		{
			handler->HandleReport(cmdId, destiationEndpoint, innerParams);
		}
		else
			Log.AddL(eLogTypes::DVC, MakeTag(), "Unknown command class handler: 0x{:02X} {}", (uint8_t)cmdClass, CommandClassToString(cmdClass));
	}
	bool GetFrame(ZW_APIFrame& frame, eCommandClass cmdClass, ZW_CmdId cmdId,
				  const ZW_ByteVector& params = {})
	{
		if (!HasCC(cmdClass))
		{
			Log.AddL(eLogTypes::DVC, MakeTag(), "Node does not support command class: 0x{:02X} {}",
					 (uint8_t)cmdClass, CommandClassToString(cmdClass));
			return false;
		}
		auto handler = device.GetHandler(cmdClass);
		if (handler)
		{
			handler->MakeFrame(frame, ZW_CmdId(cmdId), params);
			return true;
		}
		Log.AddL(eLogTypes::DVC, MakeTag(), "Unknown command class handler: 0x{:02X} {}",
				 (uint8_t)cmdClass, CommandClassToString(cmdClass));
		return false;
	}

	// ---------- ToString ----------
	std::string ToString(int width) const;

public:
	virtual void SendFrame(const ZW_APIFrame& frame) = 0;
};

class ZW_Node : public ZW_NodeInfo
{
public:
	ZW_Node(node_t nodeid, EnqueueFn enqueue) : ZW_NodeInfo(nodeid)
	{
		this->enqueue = std::move(enqueue);
		Start();
	};
	~ZW_Node() { Stop(); }

	ZW_Node(const ZW_Node&) = delete;
	ZW_Node& operator=(const ZW_Node&) = delete;
	// Non-movable: the worker thread captures `this`. Moving would leave the thread
	// running against a moved-from object, causing use-after-move/dangling member
	// access (e.g., invalid `device` handlers).
	ZW_Node(ZW_Node&&) = delete;
	ZW_Node& operator=(ZW_Node&&) = delete;

	void SendFrame(const ZW_APIFrame& frame) override
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
		//		stateCondition.notify_one();
		if (worker.joinable())
			worker.join();
	}

	void WakeUp() override
	{
		{
			DebugLockGuard lock(stateMutex);
			if (nodeState != eNodeState::Awake)
			{
				nodeState = eNodeState::Awake;
				NotifyUI(UINotify::NodeChanged, NodeId);
			}
		}
		//		stateCondition.notify_one();
	}

	void Sleeping() override
	{
		DebugLockGuard lock(stateMutex);
		if (!protocolInfo.isListening && nodeState != eNodeState::Sleepy)
		{
			nodeState = eNodeState::Sleepy;
			NotifyUI(UINotify::NodeChanged, NodeId);
		}
	}


	/// //////////////////////////////////////////////////////////////////////////////
	// jobs

	enum class eJobs
	{
		BATTERY_GET,
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
		node_t nodeId;
		uint8_t endpoint;

		uint32_t value;
		eConfigSize cfgSize = eConfigSize::OneByte;
	};
	void EnqueueJob(Job job)
	{
		if (!SupportsJob(job.job))
		{
			Log.AddL(eLogTypes::DVC, MakeTag(), "Node {} does not support job: {}", NodeId, (uint8_t)job.job);
			return;
		}

		DebugLockGuard lock(stateMutex);
		jobQueue.push_back(job);
		Log.AddL(eLogTypes::DVC, MakeTag(), "EnqueueJob: num={}", jobQueue.size());
	}

	bool HandleNodeFailed(uint8_t status);

private:
	EnqueueFn enqueue;
	std::thread worker;
	std::atomic<bool> running{ false };
	//	std::condition_variable stateCondition;

	std::vector<Job> jobQueue;

	enum class eIsDeadStates { Idle, Checking };
	struct IsDeadState
	{
		eIsDeadStates isDeadState = eIsDeadStates::Idle;
	};
	static IsDeadState isDeadState;

	bool SupportsJob(eJobs type)
	{
		switch (type)
		{
		case eJobs::BATTERY_GET:
			return HasCC(eCommandClass::BATTERY);

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
	/*
	bool IsJobQueueEmpty() const
	{
		DebugLockGuard lock(stateMutex);
		return jobQueue.empty();
	}
	*/
	bool TryPeekJob(Job& job)
	{
		DebugLockGuard lock(stateMutex);
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
		DebugLockGuard lock(stateMutex);
		if (!jobQueue.empty())
			jobQueue.erase(jobQueue.begin());
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
		case eJobs::ASSOCIATION_INTERVIEW:
			doneOrError = ExecuteAssociationInterviewJob();
			break;
		case eJobs::MULTI_CHANNEL_ASSOCIATION_INTERVIEW:
			doneOrError = ExecuteMultiChannelAssociationInterviewJob();
			break;
		case eJobs::CONFIGURATION_INTERVIEW:
			doneOrError = ExecuteConfigurationInterviewJob(job.group, (uint8_t)(job.value&0xFF));
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

	void ProcessIsDead();

	void ProcessInterviewState();

	bool ExecuteBatteryCommandJob();
	bool ExecuteAssociationInterviewJob();
	bool ExecuteMultiChannelAssociationInterviewJob();
	bool ExecuteConfigurationInterviewJob(uint8_t startparam, uint8_t numParams);
	bool ExecuteBindCommandJob(uint8_t groupId, node_t nodeid);
	bool ExecuteUnBindCommandJob(uint8_t groupId, node_t nodeid);
	bool ExecuteMultiChannelUnBindCommandJob(uint8_t groupId, node_t nodeid, uint8_t endpoint);
	bool ExecuteMultiChannelBindCommandJob(uint8_t groupId, node_t nodeid, uint8_t endpoint);
	bool ExecuteConfigurationCommandJob(uint8_t paramNumber, eConfigSize size, uint32_t value);

};
