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


class ZW_NodeInfo
{

public:
	ZW_NodeInfo(uint16_t nodeId)
		: NodeId{ nodeId }, device(*this)
	{};

	ZW_NodeInfo(const ZW_NodeInfo&) = delete;
	ZW_NodeInfo& operator=(const ZW_NodeInfo&) = delete;
	ZW_NodeInfo(ZW_NodeInfo&&) noexcept = default;
	ZW_NodeInfo& operator=(ZW_NodeInfo&&) noexcept = default;

const uint16_t NodeId;

	//
	// Protocol info (decoded)
	//
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
	};

	// WakeUp struct for ZW_CC_WakeUp variables
	struct WakeUpInfo
	{
		std::chrono::system_clock::time_point lastWakeUp; // Last lastWakeUp
		uint32_t wakeUpInterval = 0; // Current wakeUpInterval
		uint32_t wakeUpMin = 0; // Minimum wakeUpInterval
		uint32_t wakeUpMax = 0; // Maximum wakeUpInterval
		uint32_t wakeUpDefault = 0; // Default wakeUpInterval
		bool hasLastReport = false; // Whether last report was received
	};

	enum class eInterviewState
	{
		NotInterviewed,
		ProtocolInfoPending,
		ProtocolInfoDone,
		NodeInfoPending,
		NodeInfoDone,
		CCVersionPending,
		CCVersionDone,
		CCMnfcSpecPending,
		CCMnfcSpecDone,
		InterviewDone,
	};
	void SetInterviewState(eInterviewState state)
	{
		DebugLockGuard lock(stateMutex);
		interviewState = state;
	}

	eInterviewState GetInterviewState() const
	{
		DebugLockGuard lock(stateMutex);
		return interviewState;
	}

	enum class eNodeState { Bad, Awake, Sleepy };

private:
	ProtocolInfo protocolInfo;

	WakeUpInfo wakeUpInfo;

protected:
	mutable DebugMutex stateMutex;
	eNodeState nodeState = eNodeState::Bad;
	eInterviewState interviewState = eInterviewState::NotInterviewed;
	ZW_Device device;

public:
	eNodeState GetState() const
	{
		DebugLockGuard lock(stateMutex);
		return nodeState;
	}

	virtual void WakeUp()
	{
		DebugLockGuard lock(stateMutex);
		nodeState = eNodeState::Awake;
	}

	virtual void Sleeping()
	{
		DebugLockGuard lock(stateMutex);
		nodeState = eNodeState::Sleepy;
	}


	bool IsListening() const
	{
		DebugLockGuard lock(stateMutex);
		return protocolInfo.isListening;
	}
	void SetProtocolInfo(const ProtocolInfo& info)
	{
		DebugLockGuard lock(stateMutex);
		protocolInfo = info;
	}

	void SetWakeUpInfo(const WakeUpInfo& wakeup) 
	{
		DebugLockGuard lock(stateMutex);
		wakeUpInfo = wakeup;
	}

	WakeUpInfo GetWakeUpInfo() const
	{
		DebugLockGuard lock(stateMutex);
		return wakeUpInfo;
	}

	//
	// Manufacturer info
	//
    struct ManufacturerInfo
    {
        uint16_t mfgId = 0;
        uint16_t prodType = 0;
        uint16_t prodId = 0;

        uint8_t deviceIdType = 0;
        uint8_t deviceIdFormat = 0;

        bool hasDeviceId = false;
        bool hasManufacturerData = false;
    };
    ManufacturerInfo manufacturerInfo;

	//
	// CC values
	//
	std::optional<uint8_t> batteryLevel;
	std::optional<uint8_t> basicValue;
	std::optional<uint8_t> switchBinaryValue;
	std::optional<uint8_t> switchMultilevelValue;
	std::optional<uint8_t> sensorBinaryValue;
	std::optional<uint8_t> protectionState;

	//
	// CC report structures
	//
	struct MeterInfo
	{
		bool hasValue = false;
		uint8_t meterType = 0;
		uint8_t value = 0;
	} meterInfo;

	struct ConfigurationInfo
	{
		bool hasLastReport = false;
		uint8_t paramNumber = 0;
		std::vector<uint8_t> raw;
	} configurationInfo;

	struct AssociationInfo
	{
		uint8_t groupId = 0;
		std::vector<uint8_t> nodes;
		bool hasLastReport = false;
	};
	std::vector<AssociationInfo> associationInfo;

	struct MultiChannelInfo
	{
		bool hasLastReport = false;
		std::vector<uint8_t> raw;
	} multiChannelInfo;

	struct MultiChannelAssociationInfo
	{
		bool hasLastReport = false;
		uint8_t groupId = 0;
		std::vector<uint8_t> raw;
	} multiChannelAssociationInfo;

	//
	// Command class table
	//
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

	//
	// NIF assignment + CC handler creation
	//
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
			ccs[cc].supported = true;
		}

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
	}

	
	//
	// CC dispatch
	//
	void HandleCCDeviceReport(uint8_t cmdClass, uint8_t cmdId, const std::vector<uint8_t>& cmdParams)
	{
		auto handler = device.GetHandler(static_cast<eCommandClass>(cmdClass));
		if (handler)
			handler->HandleReport(cmdId, cmdParams);
		else
			Log.AddL(eLogTypes::INFO, MakeTag(), "Unknown command class handler: 0x{:02X}", cmdClass);
	}

	bool GetFrame(ZW_APIFrame& frame, eCommandClass cmdClass, uint8_t cmdId,
				  const std::vector<uint8_t>& params = {})
	{
		if (!HasCC(cmdClass))
		{
			Log.AddL(eLogTypes::INFO, MakeTag(), "Node does not support command class: 0x{:02X}",
					 static_cast<uint8_t>(cmdClass));
			return false;
		}

		auto handler = device.GetHandler(cmdClass);
		if (handler)
		{
			handler->MakeFrame(frame, cmdId, params);
			return true;
		}

		Log.AddL(eLogTypes::INFO, MakeTag(), "Unknown command class handler: 0x{:02X}",
				 static_cast<uint8_t>(cmdClass));
		return false;
	}

	//
	// ToString()
	//
	std::string ToString() const;

};

class ZW_Node : public ZW_NodeInfo
{
public:
	ZW_Node(uint16_t nodeId, EnqueueFn enqueue) : ZW_NodeInfo(nodeId)
	{
		this->enqueue = std::move(enqueue);
		Start();
	};
	~ZW_Node() { Stop(); }

	ZW_Node(const ZW_Node&) = delete;
	ZW_Node& operator=(const ZW_Node&) = delete;
	ZW_Node(ZW_Node&&) noexcept = default;
	ZW_Node& operator=(ZW_Node&&) noexcept = default;

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
		stateCondition.notify_one();
		if (worker.joinable())
			worker.join();
	}

	void WakeUp() override
	{
		{
			DebugLockGuard lock(stateMutex);
			nodeState = eNodeState::Awake;
		}
		stateCondition.notify_one();
	}

	void Sleeping() override
	{
		DebugLockGuard lock(stateMutex);
		nodeState = eNodeState::Sleepy;
	}


	/// //////////////////////////////////////////////////////////////////////////////
	// jobs

	enum class eJobs
	{
		BATTERY_GET,
	};
	void EnqueueJob(eJobs job)
	{
		DebugLockGuard lock(stateMutex);
		jobQueue.push_back(Job{ job });
	}

private:
	EnqueueFn enqueue;
	std::thread worker;
	std::atomic<bool> running{ false };
	std::condition_variable stateCondition;

	struct Job
	{
		eJobs job;
	};
	std::vector<Job> jobQueue;

	bool TryPeekJob(Job& job)
	{
		DebugLockGuard lock(stateMutex);
		if (jobQueue.empty())
			return false;
		job = jobQueue.front();
		return true;
	}

	void PopJob()
	{
		DebugLockGuard lock(stateMutex);
		if (!jobQueue.empty())
			jobQueue.erase(jobQueue.begin());
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
		while (running.load())
		{
			// Wait for node to be awake or running to be false
			while (true)
			{
				if (GetState() == eNodeState::Awake || !running.load())
					break;
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}

			if (!running.load())
				break;

			if (nodeState == eNodeState::Awake)
			{
				if (GetInterviewState() == eInterviewState::InterviewDone || jobQueue.empty())
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					continue;
				}

				if( GetInterviewState() != eInterviewState::InterviewDone)
					RunInterview();

				if(!jobQueue.empty())
					CheckJobQueue();
			}
		}
	}

	void CheckJobQueue()
	{
		Job job;
		if (!TryPeekJob(job))
			return;

		switch (job.job)
		{
		case eJobs::BATTERY_GET:
			// TODO: Implement BATTERY_GET job handling
			break;
		default:
			// TODO: Handle unknown or unsupported jobs
			break;
		}

		PopJob();
	}

	void RunInterview()
	{
		switch (GetInterviewState())
		{
		case eInterviewState::NotInterviewed:
		case eInterviewState::ProtocolInfoPending:
		case eInterviewState::ProtocolInfoDone:
		case eInterviewState::NodeInfoPending:
			break;
		case eInterviewState::NodeInfoDone:
			// fallthrough
		case eInterviewState::CCVersionPending:
			SetInterviewState(eInterviewState::CCVersionPending);
			if (auto* handler = device.GetHandler(eCommandClass::VERSION))
			{
				for (const auto ccId : GetSupportedCCs())
				{
					auto* cc = GetCC(ccId);
					if (!cc || cc->versionOk)
						continue;

					int retryCount = 0;
					do
					{
						Log.AddL(eLogTypes::INFO, MakeTag(), ">> NODE VERSION_COMMAND_CLASS_GET CC [0x{:02X}] to node {}", (uint8_t)ccId, NodeId);

						ZW_APIFrame frame;
						handler->MakeFrame(frame, static_cast<uint8_t>(eVersionCommand::VERSION_COMMAND_CLASS_GET), { static_cast<uint8_t>(ccId) });
						enqueue(frame);
						std::this_thread::sleep_for(std::chrono::milliseconds(10));

						// Wait for VERSION_COMMAND_CLASS_REPORT which sets `versionOk`.
						if (!WaitUntil(std::chrono::seconds(5), [&]() { return cc->versionOk; }))
						{
							Log.AddL(eLogTypes::ERR, MakeTag(), "Version timeout: CC 0x{:02X} for node {}", static_cast<uint8_t>(ccId), NodeId);
						}
					} while (!cc->versionOk && retryCount++ < 3);
				}
				SetInterviewState(eInterviewState::CCVersionDone);
			}
			break;

		case eInterviewState::CCVersionDone:
		case eInterviewState::CCMnfcSpecPending:
			if (auto* cc = GetCC(eCommandClass::MANUFACTURER_SPECIFIC))
			{
				uint8_t version = cc->version; // TODO: support v2+ servers

				SetInterviewState(eInterviewState::CCMnfcSpecPending);
				if (auto* handler = device.GetHandler(eCommandClass::MANUFACTURER_SPECIFIC))
				{
					Log.AddL(eLogTypes::INFO, MakeTag(), ">> NODE MANUFACTURER_SPECIFIC_COMMAND_CLASS_GET to node {}", NodeId);

					if (version >= 1)
					{
						ZW_APIFrame frame;
						handler->MakeFrame(frame, static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET), { 0 });
						enqueue(frame);

						// Wait for MANUFACTURER_SPECIFIC_COMMAND_CLASS_REPORT which sets `hasManufacturerData`.
						if (!WaitUntil(std::chrono::seconds(5), [&]() { return manufacturerInfo.hasManufacturerData; }))
						{
							Log.AddL(eLogTypes::ERR, MakeTag(), "Manufacturer timeout: node {}", NodeId);
							break;
						}
					}
					if (version >= 2)
					{
						ZW_APIFrame frame;
						handler->MakeFrame(frame, static_cast<uint8_t>(eManufacturerSpecificCommand::DEVICE_SPECIFIC_GET_V2), { 0 });
						enqueue(frame);

						// Wait for MANUFACTURER_SPECIFIC_COMMAND_CLASS_REPORT which sets `hasDeviceId`.
						if (!WaitUntil(std::chrono::seconds(5), [&]() { return manufacturerInfo.hasDeviceId; }))
						{
							Log.AddL(eLogTypes::ERR, MakeTag(), "Manufacturer timeout: node {}", NodeId);
							break;
						}
					}
				}
				SetInterviewState(eInterviewState::CCMnfcSpecDone);
			}
			break;
		}
	}
};

