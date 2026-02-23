#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

class ZW_Node;

#include "CommandClass.h"
#include "APIFrame.h"

class ZW_CCHandler
{
protected:
    ZW_Node& node;

public:
    ZW_CCHandler(ZW_Node& n) : node(n) {}
    virtual ~ZW_CCHandler() = default;

    virtual void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) = 0;
    virtual void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) = 0;
    virtual void SetValue(int value) {}
    virtual std::string ToString() const { return ""; }

    std::string ParamsToString(const std::vector<uint8_t>& params) const
    {
		std::string result = "Params = ";
		for (size_t i = 0; i < params.size(); i++)
		{
			result += std::format("0x{:02}",params[i]);
			if (i + 1 < params.size())
				result += ", ";
		}
		result += "";
		return result;
    }
};

class ZW_Device
{
    ZW_Node& node;
    std::unordered_map<eCommandClass, std::unique_ptr<ZW_CCHandler>> handlers;

public:
    ZW_Device(ZW_Node& n) : node(n) {}

    template<typename T>
    void AddHandler()
    {
        auto ccId = T::CC;
        handlers[ccId] = std::unique_ptr<ZW_CCHandler>(new T(node));
    }

    ZW_CCHandler* GetHandler(eCommandClass ccId)
    {
        auto it = handlers.find(ccId);
        return it != handlers.end() ? it->second.get() : nullptr;
    }
};

class ZW_CC_Version : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::VERSION;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};

class ZW_CC_Battery : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::BATTERY;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};

class ZW_CC_SwitchBinary : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::SWITCH_BINARY;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;

    void SetValue(int value) override
    {
        // send SWITCH_BINARY_SET
    }
};

class ZW_CC_Basic : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::BASIC;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};

class ZW_CC_SwitchMultilevel : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::SWITCH_MULTILEVEL;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;

    void SetValue(int value) override
    {
        // send SWITCH_MULTILEVEL_SET
    }
};

class ZW_CC_SensorBinary : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::SENSOR_BINARY;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};

class ZW_CC_Meter : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::METER;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};

class ZW_CC_MultiChannel : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::MULTI_CHANNEL;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};

class ZW_CC_Configuration : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::CONFIGURATION;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};

class ZW_CC_Protection : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::PROTECTION;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};

class ZW_CC_Association : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::ASSOCIATION;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};

class ZW_CC_MultiChannelAssociation : public ZW_CCHandler
{
public:
    static constexpr eCommandClass CC = eCommandClass::MULTI_CHANNEL_ASSOCIATION;

    using ZW_CCHandler::ZW_CCHandler;

    void MakeFrame(ZW_APIFrame& frame, uint8_t cmdId, const std::vector<uint8_t>& params) override;
    void HandleReport(uint8_t cmdId, const std::vector<uint8_t>& params) override;
};
