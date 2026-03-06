#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

class ZW_NodeInfo;

#include "CommandClass.h"
#include "APIFrame.h"

struct ZW_CmdId
{
	uint8_t value;
	constexpr ZW_CmdId(uint8_t v) : value(v) {}

	template <typename E>
		requires std::is_enum_v<E>
	constexpr ZW_CmdId(E e) : value(static_cast<uint8_t>(e)) {}
};

using ZW_ByteVector = std::vector<uint8_t>;

class ZW_CCHandler
{
protected:
	ZW_NodeInfo& node;

public:
	ZW_CCHandler(ZW_NodeInfo& n) : node(n) {}
	virtual ~ZW_CCHandler() = default;

	virtual void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) = 0;
	virtual void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) = 0;
	virtual void SetValue(int value) {}
	virtual std::string ToString() const { return ""; }

	std::string ParamsToString(const ZW_ByteVector& params) const
	{
		std::string result = "Params = ";
		for (size_t i = 0; i < params.size(); i++)
		{
			result += std::format("0x{:02}", params[i]);
			if (i + 1 < params.size())
				result += ", ";
		}
		result += "";
		return result;
	}
};

class ZW_Device
{
	ZW_NodeInfo& node;
	std::unordered_map<eCommandClass, std::unique_ptr<ZW_CCHandler>> handlers;

public:
	ZW_Device(ZW_NodeInfo& n) : node(n) {}

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
	enum class eVersionCommand : uint8_t
	{
		VERSION_GET = 0x11, // Request Z-Wave protocol/library version
		VERSION_REPORT = 0x12, // Response to VERSION_GET

		VERSION_COMMAND_CLASS_GET = 0x13, // Ask for version of a specific Command Class
		VERSION_COMMAND_CLASS_REPORT = 0x14, // Response with CC version

		VERSION_CAPABILITIES_GET = 0x15, // Ask for Version CC capabilities (Z-Wave Plus)
		VERSION_CAPABILITIES_REPORT = 0x16, // Response with capabilities

		VERSION_ZWAVE_SOFTWARE_GET = 0x17, // Ask for Z-Wave software info (SDK, app, protocol)
		VERSION_ZWAVE_SOFTWARE_REPORT = 0x18  // Response with software info
	};

	static constexpr eCommandClass CC = eCommandClass::VERSION;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_Battery : public ZW_CCHandler
{
public:
	enum class eBatteryCommand : uint8_t
	{
		BATTERY_GET = 0x02,
		BATTERY_REPORT = 0x03
	};

	static constexpr eCommandClass CC = eCommandClass::BATTERY;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_ManufacturerSpecific : public ZW_CCHandler
{
public:
	enum class eManufacturerSpecificCommand : uint8_t
	{
		DEVICE_SPECIFIC_GET = 0x04,
		DEVICE_SPECIFIC_GET_V2 = 0x06,
		DEVICE_SPECIFIC_REPORT = 0x05,
		DEVICE_SPECIFIC_REPORT_V2 = 0x07
	};

	static constexpr eCommandClass CC = eCommandClass::MANUFACTURER_SPECIFIC;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_SwitchBinary : public ZW_CCHandler
{
public:
	enum class eSwitchBinaryCommand : uint8_t
	{
		SWITCH_BINARY_SET = 0x01,
		SWITCH_BINARY_GET = 0x02,
		SWITCH_BINARY_REPORT = 0x03,
	};

	static constexpr eCommandClass CC = eCommandClass::SWITCH_BINARY;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;

	void SetValue(int value) override
	{
		// send SWITCH_BINARY_SET
	}
};

class ZW_CC_Basic : public ZW_CCHandler
{
public:
	enum class eBasicCommand : uint8_t
	{
		BASIC_SET = 0x01,
		BASIC_GET = 0x02,
		BASIC_REPORT = 0x03,
	};

	static constexpr eCommandClass CC = eCommandClass::BASIC;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_SwitchMultilevel : public ZW_CCHandler
{
public:
	enum class eSwitchMultilevelCommand : uint8_t
	{
		SWITCH_MULTILEVEL_SET = 0x01,
		SWITCH_MULTILEVEL_GET = 0x02,
		SWITCH_MULTILEVEL_REPORT = 0x03,
	};

	static constexpr eCommandClass CC = eCommandClass::SWITCH_MULTILEVEL;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;

	void SetValue(int value) override
	{
		// send SWITCH_MULTILEVEL_SET
	}
};

class ZW_CC_SensorBinary : public ZW_CCHandler
{
public:
	enum class eSensorBinaryCommand : uint8_t
	{
		SENSOR_BINARY_GET = 0x02,
		SENSOR_BINARY_REPORT = 0x03,
	};

	static constexpr eCommandClass CC = eCommandClass::SENSOR_BINARY;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_Meter : public ZW_CCHandler
{
public:
	enum class eMeterCommand : uint8_t
	{
		METER_GET = 0x01,
		METER_REPORT = 0x02,
	};

	static constexpr eCommandClass CC = eCommandClass::METER;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_MultiChannel : public ZW_CCHandler
{
public:
	enum class eMultiChannelCommand : uint8_t
	{
		// End Point discovery
		MULTI_CHANNEL_END_POINT_GET = 0x07,
		MULTI_CHANNEL_END_POINT_REPORT = 0x08,

		// Capability discovery
		MULTI_CHANNEL_CAPABILITY_GET = 0x09,
		MULTI_CHANNEL_CAPABILITY_REPORT = 0x0A,

		// End Point Find
		MULTI_CHANNEL_END_POINT_FIND = 0x0B,
		MULTI_CHANNEL_END_POINT_FIND_REPORT = 0x0C,

		// Encapsulation
		MULTI_CHANNEL_CMD_ENCAP = 0x0D,

		// Aggregated endpoints
		MULTI_CHANNEL_AGGREGATED_MEMBERS_GET = 0x0E,
		MULTI_CHANNEL_AGGREGATED_MEMBERS_REPORT = 0x0F
	};

	static constexpr eCommandClass CC = eCommandClass::MULTI_CHANNEL;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_Configuration : public ZW_CCHandler
{
public:
	enum class eConfigurationCommand : uint8_t
	{
		CONFIGURATION_SET = 0x04,
		CONFIGURATION_GET = 0x05,
		CONFIGURATION_REPORT = 0x06
	};

	static constexpr eCommandClass CC = eCommandClass::CONFIGURATION;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_Protection : public ZW_CCHandler
{
public:
	enum class eProtectionCommand : uint8_t
	{
		PROTECTION_GET = 0x02,
		PROTECTION_REPORT = 0x03,
	};

	static constexpr eCommandClass CC = eCommandClass::PROTECTION;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_Association : public ZW_CCHandler
{
public:
	enum class eAssociationCommand : uint8_t
	{
		ASSOCIATION_SET = 0x01,
		ASSOCIATION_GET = 0x02,
		ASSOCIATION_REPORT = 0x03,
		ASSOCIATION_REMOVE = 0x04,
		ASSOCIATION_GROUPINGS_GET = 0x05,
		ASSOCIATION_GROUPINGS_REPORT = 0x06
	};

	static constexpr eCommandClass CC = eCommandClass::ASSOCIATION;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_MultiChannelAssociation : public ZW_CCHandler
{
public:
	enum class eMultiChannelAssociationCommand : uint8_t
	{
		MULTI_CHANNEL_ASSOCIATION_SET = 0x01,
		MULTI_CHANNEL_ASSOCIATION_GET = 0x02,
		MULTI_CHANNEL_ASSOCIATION_REPORT = 0x03,
		MULTI_CHANNEL_ASSOCIATION_REMOVE = 0x04,
		MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET = 0x05,
		MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT = 0x06
	};

	static constexpr eCommandClass CC = eCommandClass::MULTI_CHANNEL_ASSOCIATION;

	using ZW_CCHandler::ZW_CCHandler;

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const ZW_ByteVector& params) override;
	void HandleReport(ZW_CmdId cmdid, const ZW_ByteVector& params) override;
};

class ZW_CC_WakeUp : public ZW_CCHandler
{
public:
	static constexpr eCommandClass CC = eCommandClass::WAKE_UP;
	using ZW_CCHandler::ZW_CCHandler;

	enum class eWakeUpCommand : uint8_t
	{
		WAKE_UP_INTERVAL_SET = 0x04,
		WAKE_UP_INTERVAL_GET = 0x05,
		WAKE_UP_INTERVAL_REPORT = 0x06,
		WAKE_UP_NOTIFICATION = 0x07,
		WAKE_UP_NO_MORE_INFORMATION = 0x08,

		// v3+
		WAKE_UP_INTERVAL_CAPABILITIES_GET = 0x09,
		WAKE_UP_INTERVAL_CAPABILITIES_REPORT = 0x0A
	};

	void MakeFrame(ZW_APIFrame& frame, ZW_CmdId cmdid, const std::vector<uint8_t>& params) override;
	void HandleReport(ZW_CmdId cmdid, const std::vector<uint8_t>& params) override;
};
