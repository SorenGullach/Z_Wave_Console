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

class NodeInfo;

class NodeInfoFormatter
{
public:
	explicit NodeInfoFormatter(const NodeInfo& ni)
		: ni(ni)
	{
	}

	// Human-readable grouped fields
	std::string SupportedSpeedName() const;
	std::string ProtocolVersionName() const;
	std::string BasicDeviceTypeName() const;
	std::string GenericDeviceClassName() const;
	std::string SpecificDeviceClassName() const;

	// Full text dump
	std::string ToTextProtocol() const;
	std::string ToText() const;

	// JSON for GUI
//	std::string ToJson() const;

private:
	const NodeInfo& ni;
};

class NodeInfo
{
private:
	NodeInfoFormatter formatter;
	friend class NodeInfoFormatter;

public:
	NodeInfo(nodeid_t nodeid);

	std::string ToString() const { return formatter.ToText(); }

	const nodeid_t nodeId;

	// ----------------- Interview state -----------------
	enum class eInterviewState
	{
		NotInterviewed, ProtocolInfoPending, ProtocolInfoDone, NodeInfoPending, NodeInfoDone,
		CCVersionPending, CCVersionDone, CCMnfcSpecPending, CCMnfcSpecDone, CCMultiChannelPending, CCMultiChannelDone, InterviewDone,
	};

	// ----------------- Node state -----------------
	enum class eNodeState
	{
		New,
		Awake,
		Sleepy
	};

	// ----------------- Protocol info -----------------
	struct ProtocolInfo
	{
		enum class eProtocolFlags : uint8_t
		{
			Listening = 0x80,
			Routing = 0x40
		};

		enum class eSupportedSpeeds : uint8_t
		{
			SpeedUnknown = 0,
			Speed9k6 = 1,
			Speed40k = 2,
			Speed100k = 3,
			// extend if needed
		};

		enum class eProtocolVersions : uint8_t
		{
			V1 = 1,
			V2 = 2,
			V3 = 3,
			// extend if needed
		};

		enum class eNodeFeatures : uint8_t
		{
			Security = 0x01,
			ControllerNode = 0x02,
			SpecificDevice = 0x04,
			RoutingEndNode = 0x08,
			BeamCapability = 0x10,
			Sensor250ms = 0x20,
			Sensor1000ms = 0x40,
			OptionalFunctionality = 0x80
		};

		enum class eSpeedExtensions : uint8_t
		{
			None = 0,
			Ext1 = 1,
			Ext2 = 2,
			Ext3 = 3
		};

		enum class eBasicDeviceType : uint8_t
		{
			Controller = 0x01,
			StaticController = 0x02,
			Slave = 0x03,
			RoutingSlave = 0x04,
			Unknown = 0x00
		};

		enum class eGenericDeviceClass : uint8_t
		{
			GenericController = 0x01,
			StaticController = 0x02,
			AvControlPoint = 0x03,
			Display = 0x04,
			NetworkExtender = 0x05,
			Appliance = 0x06,
			SensorNotification = 0x07,
			Thermostat = 0x08,
			WindowCovering = 0x09,
			RepeaterSlave = 0x0F,
			EntryControl = 0x40,
			SensorBinary = 0x20,
			SensorMultilevel = 0x21,
			Meter = 0x31,
			SwitchBinary = 0x10,
			SwitchMultilevel = 0x11,
			SwitchRemote = 0x12,
			SwitchToggle = 0x13,
			PulseMeter = 0x30,
			MultiChannelDevice = 0xA1,
			Unknown = 0x00
		};

		eBasicDeviceType basicType = eBasicDeviceType::Unknown;
		eGenericDeviceClass genericDeviceClass = eGenericDeviceClass::Unknown;
		uint8_t specificDeviceClass = 0;

		uint8_t b0 = 0;   // Byte 0 from protocol info
		uint8_t b1 = 0;   // Byte 1
		uint8_t b2 = 0;   // Byte 2

		// --- Byte 0 ---
		bool IsListening() const { return (b0 & static_cast<uint8_t>(eProtocolFlags::Listening)) != 0; }
		bool IsRouting()   const { return (b0 & static_cast<uint8_t>(eProtocolFlags::Routing)) != 0; }

		eSupportedSpeeds SupportedSpeed() const { return static_cast<eSupportedSpeeds>((b0 >> 3) & 0x07); }
		eProtocolVersions ProtocolVersion() const { return static_cast<eProtocolVersions>(b0 & 0x07); }

		// --- Byte 1 ---
		bool HasFeature(eNodeFeatures f) const { return (b1 & static_cast<uint8_t>(f)) != 0; }

		bool Security()              const { return HasFeature(eNodeFeatures::Security); }
		bool ControllerNode()        const { return HasFeature(eNodeFeatures::ControllerNode); }
		bool SpecificDevice()        const { return HasFeature(eNodeFeatures::SpecificDevice); }
		bool RoutingEndNode()        const { return HasFeature(eNodeFeatures::RoutingEndNode); }
		bool BeamCapability()        const { return HasFeature(eNodeFeatures::BeamCapability); }
		bool Sensor250ms()           const { return HasFeature(eNodeFeatures::Sensor250ms); }
		bool Sensor1000ms()          const { return HasFeature(eNodeFeatures::Sensor1000ms); }
		bool OptionalFunctionality() const { return HasFeature(eNodeFeatures::OptionalFunctionality); }

		// --- Byte 2 ---
		eSpeedExtensions SpeedExtension() const { return static_cast<eSpeedExtensions>(b2 & 0x07); }

		eBasicDeviceType BasicType() const { return basicType; }
		eGenericDeviceClass GenericDeviceClass() const { return genericDeviceClass; }
		uint8_t SpecificDeviceClass() const { return specificDeviceClass; }
	};

	std::string ToStringProtocol() const { return formatter.ToTextProtocol(); }
	void SetProtocolInfo(const ProtocolInfo& DVC);
	bool IsListening() const;
	void SetNIF(ProtocolInfo::eBasicDeviceType basicType, 
				ProtocolInfo::eGenericDeviceClass genericClass,
				uint8_t specificClass, 
				const std::vector<uint8_t>& commandClasses);

	// ----------------- Wake up info -----------------
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

	// ----------------- Device info -----------------
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

	// ----------------- Configuration info -----------------
	struct ConfigurationInfo
	{
		uint8_t paramNumber = 0;
		uint8_t size = 0;
		int32_t value = 0;
		bool valid = false;
	};

	// ----------------- Multi-channel info -----------------
	struct EndpointInfo
	{
		uint8_t endpointId = 0;
		uint8_t generic = 0;
		uint8_t specific = 0;
		std::vector<uint8_t> supportedCCs;
		bool hasCapabilityReport = false;
	};

	// ----------------- Multi-channel info -----------------
	struct MultiChannelEndpointInfo
	{
		uint8_t endpointCount = 0;
		bool hasEndpointReport = false;
		std::vector<EndpointInfo> endpoints;
	};

	// ----------------- Multi-channel info -----------------
	struct MultiChannelAssociationMember
	{
		nodeid_t nodeId{};
		uint8_t endpointId = 0;
		bool valid = false;
	};

	// ----------------- Multi-channel info -----------------
	struct MultiChannelAssociationGroup
	{
		uint8_t groupId = 0;
		uint8_t maxNodes = 0;
		std::array<MultiChannelAssociationMember, 255> members;
		bool hasLastReport = false;
	};

	// ----------------- Command class info -----------------
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

	// ----------------- Meter info -----------------
	struct MeterInfo
	{
		bool hasValue = false;
		uint8_t meterType = 0;
		uint8_t value = 0;
	};

	// ----------------- Node state -----------------
	void SetInterviewState(eInterviewState state);
	eInterviewState GetInterviewState() const;
	eNodeState GetState() const;

	// ----------------- Wake/sleeping --------------
	virtual void WakeUp();
	virtual void Sleeping();

	// ----------------- Frame sending -----------------
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
