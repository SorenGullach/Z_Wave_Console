#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>
#include <optional>

#include "Logging.h"
//#include "ZW_CommandClass.h"
#include "Device.h"
#include "APIFrame.h"

class ZW_Node : public ZW_Device
{
public:
    ZW_Node() : ZW_Device(*this) {}

    ZW_Node(const ZW_Node&) = delete;
    ZW_Node& operator=(const ZW_Node&) = delete;
    ZW_Node(ZW_Node&&) noexcept = default;
    ZW_Node& operator=(ZW_Node&&) noexcept = default;

    uint16_t NodeId = 0;

    //
    // Protocol info (decoded)
    //
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
        std::vector<uint8_t> deviceIdData;

        bool hasDeviceId = false;
        bool hasManufacturerData = false;
    } manufacturerInfo;

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
        basic = basicType;
        generic = genericClass;
        specific = specificClass;

        ccs.fill({});
        for (auto cc : commandClasses)
        {
            ccs[cc].id = static_cast<eCommandClass>(cc);
            ccs[cc].supported = true;
        }

        if (HasCC(eCommandClass::BATTERY)) AddHandler<ZW_CC_Battery>();
        if (HasCC(eCommandClass::SWITCH_BINARY)) AddHandler<ZW_CC_SwitchBinary>();
        if (HasCC(eCommandClass::BASIC)) AddHandler<ZW_CC_Basic>();
        if (HasCC(eCommandClass::SWITCH_MULTILEVEL)) AddHandler<ZW_CC_SwitchMultilevel>();
        if (HasCC(eCommandClass::SENSOR_BINARY)) AddHandler<ZW_CC_SensorBinary>();
        if (HasCC(eCommandClass::METER)) AddHandler<ZW_CC_Meter>();
        if (HasCC(eCommandClass::MULTI_CHANNEL)) AddHandler<ZW_CC_MultiChannel>();
        if (HasCC(eCommandClass::CONFIGURATION)) AddHandler<ZW_CC_Configuration>();
        if (HasCC(eCommandClass::PROTECTION)) AddHandler<ZW_CC_Protection>();
        if (HasCC(eCommandClass::ASSOCIATION)) AddHandler<ZW_CC_Association>();
        if (HasCC(eCommandClass::MULTI_CHANNEL_ASSOCIATION)) AddHandler<ZW_CC_MultiChannelAssociation>();
    }

    //
    // Node state
    //
    enum class eNodeState { Bad, Awake, Sleepy } nodeState = eNodeState::Bad;

    enum class eInterviewState
    {
        Waitting,
        WattingProtocolInfo,
        DoneProtocolInfo,
        FailProtocolInfo,
        WattingNodeInfo,
        DoneNodeInfo,
        FailNodeInfo,
        WattingCCInfo,
        DoneCCInfo,
        FailCCInfo
    } interviewState = eInterviewState::Waitting;

    //
    // CC dispatch
    //
    void HandleCCDeviceReport(uint8_t cmdClass, uint8_t cmdId, const std::vector<uint8_t>& cmdParams)
    {
        auto handler = GetHandler(static_cast<eCommandClass>(cmdClass));
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

        auto handler = GetHandler(cmdClass);
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
    // ToString() unchanged
    //
    std::string ToString() const;
};
