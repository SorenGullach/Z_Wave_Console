#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "NodeId_t.h"
#include "APIFrame.h"
#include "CommandClass.h"
#include "DebugMutex.h"
#include "NodeCommandClasses.h"

class NodeInfo
{
public:
	NodeInfo(nodeid_t nodeid);

	const nodeid_t nodeId;

	enum class eInterviewState
	{
		NotInterviewed, ProtocolInfoPending, ProtocolInfoDone, NodeInfoPending, NodeInfoDone,
		CCVersionPending, CCVersionDone, CCMnfcSpecPending, CCMnfcSpecDone, CCMultiChannelPending, CCMultiChannelDone, InterviewDone,
	};

	enum class eNodeState
	{
		New,
		Awake,
		Sleepy
	};

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

	void SetProtocolInfo(const ProtocolInfo& DVC);
	bool IsListening() const;

	struct WakeUpInfo
	{
		std::chrono::system_clock::time_point lastWakeUp;
		uint32_t wakeUpInterval = 0;
		uint32_t wakeUpMin = 0;
		uint32_t wakeUpMax = 0;
		uint32_t wakeUpDefault = 0;
		bool hasLastReport = false;
	};

	void SetWakeUpInfo(const WakeUpInfo& wakeup);
	WakeUpInfo GetWakeUpInfo() const;

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

	struct ConfigurationInfo
	{
		uint8_t paramNumber = 0;
		uint8_t size = 0;
		int32_t value = 0;
		bool valid = false;
	};

	struct EndpointInfo
	{
		uint8_t endpointId = 0;
		uint8_t generic = 0;
		uint8_t specific = 0;
		std::vector<uint8_t> supportedCCs;
		bool hasCapabilityReport = false;
	};

	struct MultiChannelEndpointInfo
	{
		uint8_t endpointCount = 0;
		bool hasEndpointReport = false;
		std::vector<EndpointInfo> endpoints;
	};

	struct MultiChannelAssociationMember
	{
		nodeid_t nodeId{};
		uint8_t endpointId = 0;
		bool valid = false;
	};

	struct MultiChannelAssociationGroup
	{
		uint8_t groupId = 0;
		uint8_t maxNodes = 0;
		std::array<MultiChannelAssociationMember, 255> members;
		bool hasLastReport = false;
	};

	void SetNIF(uint8_t basicType, uint8_t genericClass, uint8_t specificClass,
		const std::vector<uint8_t>& commandClasses);

	struct CommandClassTag
	{
		eCommandClass id = static_cast<eCommandClass>(0);
		bool supported = false;
		uint8_t version = 0;
		bool versionOk = false;
	};

	CommandClassTag* GetCC(eCommandClass ccId);
	const CommandClassTag* GetCC(eCommandClass ccId) const;
	bool HasCC(eCommandClass ccId) const;
	std::vector<eCommandClass> GetSupportedCCs() const;

	struct MeterInfo
	{
		bool hasValue = false;
		uint8_t meterType = 0;
		uint8_t value = 0;
	};

	void SetInterviewState(eInterviewState state);
	eInterviewState GetInterviewState() const;
	eNodeState GetState() const;
	virtual void WakeUp();
	virtual void Sleeping();

	virtual void SendFrame(const APIFrame& frame) = 0;

private:
	friend class Node;
	friend class CC_Version;
	friend class CC_Battery;
	friend class CC_ManufacturerSpecific;
	friend class CC_SwitchBinary;
	friend class CC_Basic;
	friend class CC_SwitchMultilevel;
	friend class CC_SensorBinary;
	friend class CC_Meter;
	friend class CC_MultiChannel;
	friend class CC_Configuration;
	friend class CC_Protection;
	friend class CC_Association;
	friend class CC_MultiChannelAssociation;
	friend class CC_WakeUp;

	mutable DebugMutex stateMutex;

	std::string Floor = "Floor";
	std::string Room = "Room";

	eNodeState nodeState = eNodeState::New;
	eInterviewState interviewState = eInterviewState::NotInterviewed;
	ProtocolInfo protocolInfo;
	WakeUpInfo wakeUpInfo;
	ManufacturerInfo manufacturerInfo;
	std::array<ConfigurationInfo, 255> configurationInfo{};
	MultiChannelEndpointInfo multiChannel;
	std::vector<MultiChannelAssociationGroup> multiChannelAssociationGroups;
	std::array<CommandClassTag, 256> ccs{};
	MeterInfo meterInfo;
	std::optional<uint8_t> batteryLevel;
	std::optional<uint8_t> basicValue;
	std::optional<uint8_t> switchBinaryValue;
	std::optional<uint8_t> switchMultilevelValue;
	std::optional<uint8_t> sensorBinaryValue;
	std::optional<uint8_t> protectionState;
	CCHandlerFactory ccHandlerFactory;

	bool supportsCC(eCommandClass cc) const;
};
