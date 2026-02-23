#pragma once

#include <cstdint>
#include <string>

enum class eCommandClass : uint8_t
{
	NO_OPERATION = 0x00,
	BASIC = 0x20,
	SWITCH_BINARY = 0x25,
	SWITCH_MULTILEVEL = 0x26,
	SENSOR_BINARY = 0x30,
	SENSOR_MULTILEVEL = 0x31,
	METER = 0x32,
	COLOR_SWITCH = 0x33,
	ASSOCIATION = 0x85,
	ASSOCIATION_GRP_INFO = 0x59,
	CONFIGURATION = 0x70,
	MANUFACTURER_SPECIFIC = 0x72,
	POWERLEVEL = 0x73,
	PROTECTION = 0x75,
	FIRMWARE_UPDATE_MD = 0x7A,
	DEVICE_RESET_LOCALLY = 0x5A,
	NOTIFICATION = 0x71,
	MULTI_CHANNEL = 0x60,
	MULTI_CHANNEL_ASSOCIATION = 0x8E,
	SECURITY_0 = 0x98,
	SECURITY_2 = 0x9F,
	VERSION = 0x86,
	ZWAVEPLUS_INFO = 0x5E,
	SUPERVISION = 0x6C,
	TRANSPORT_SERVICE = 0x55,
	APPLICATION_STATUS = 0x22,
	WAKE_UP = 0x84,
	BATTERY = 0x80,
	THERMOSTAT_MODE = 0x40,
	THERMOSTAT_SETPOINT = 0x43,
	THERMOSTAT_FAN_MODE = 0x44,
	THERMOSTAT_OPERATING_STATE = 0x42,
};

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

enum class eBatteryCommand : uint8_t
{
	BATTERY_GET = 0x02,
	BATTERY_REPORT = 0x03
};

// Common Z-Wave Command Class command IDs used by handlers
enum class eSwitchBinaryCommand : uint8_t
{
	SWITCH_BINARY_SET = 0x01,
	SWITCH_BINARY_GET = 0x02,
	SWITCH_BINARY_REPORT = 0x03,
};

enum class eBasicCommand : uint8_t
{
	BASIC_SET = 0x01,
	BASIC_GET = 0x02,
	BASIC_REPORT = 0x03,
};

enum class eSwitchMultilevelCommand : uint8_t
{
	SWITCH_MULTILEVEL_SET = 0x01,
	SWITCH_MULTILEVEL_GET = 0x02,
	SWITCH_MULTILEVEL_REPORT = 0x03,
};

enum class eSensorBinaryCommand : uint8_t
{
	SENSOR_BINARY_GET = 0x02,
	SENSOR_BINARY_REPORT = 0x03,
};

enum class eMeterCommand : uint8_t
{
	METER_GET = 0x01,
	METER_REPORT = 0x02,
};

enum class eMultiChannelCommand : uint8_t
{
	MULTI_CHANNEL_CAPABILITY_GET = 0x09,
	MULTI_CHANNEL_CAPABILITY_REPORT = 0x0A,
};

enum class eConfigurationCommand : uint8_t
{
	CONFIGURATION_GET = 0x05,
	CONFIGURATION_REPORT = 0x06,
};

enum class eProtectionCommand : uint8_t
{
	PROTECTION_GET = 0x02,
	PROTECTION_REPORT = 0x03,
};

enum class eAssociationCommand : uint8_t
{
	ASSOCIATION_SET = 0x01,
	ASSOCIATION_GET = 0x02,
	ASSOCIATION_REPORT = 0x03,
	ASSOCIATION_REMOVE = 0x04,
	ASSOCIATION_GROUPINGS_GET = 0x05,
	ASSOCIATION_GROUPINGS_REPORT = 0x06
};

enum class eMultiChannelAssociationCommand : uint8_t
{
	MULTI_CHANNEL_ASSOCIATION_SET = 0x01,
	MULTI_CHANNEL_ASSOCIATION_GET = 0x02,
	MULTI_CHANNEL_ASSOCIATION_REPORT = 0x03,
	MULTI_CHANNEL_ASSOCIATION_REMOVE = 0x04
};

enum class eAssociationGroupInfoCommand : uint8_t
{
	AGI_GROUP_NAME_GET = 0x01,
	AGI_GROUP_NAME_REPORT = 0x02,
	AGI_GROUP_INFO_GET = 0x03,
	AGI_GROUP_INFO_REPORT = 0x04,
	AGI_GROUP_COMMAND_LIST_GET = 0x05,
	AGI_GROUP_COMMAND_LIST_REPORT = 0x06
};

inline std::string CommandClassToString(eCommandClass cc)
{
	switch (cc)
	{
	case eCommandClass::BASIC: return "BASIC";
	case eCommandClass::NO_OPERATION: return "NO_OPERATION";
	case eCommandClass::SWITCH_BINARY: return "SWITCH_BINARY";
	case eCommandClass::SWITCH_MULTILEVEL: return "SWITCH_MULTILEVEL";
	case eCommandClass::SENSOR_BINARY: return "SENSOR_BINARY";
	case eCommandClass::SENSOR_MULTILEVEL: return "SENSOR_MULTILEVEL";
	case eCommandClass::METER: return "METER";
	case eCommandClass::COLOR_SWITCH: return "COLOR_SWITCH";
	case eCommandClass::ASSOCIATION: return "ASSOCIATION";
	case eCommandClass::ASSOCIATION_GRP_INFO: return "ASSOCIATION_GRP_INFO";
	case eCommandClass::CONFIGURATION: return "CONFIGURATION";
	case eCommandClass::MANUFACTURER_SPECIFIC: return "MANUFACTURER_SPECIFIC";
	case eCommandClass::POWERLEVEL: return "POWERLEVEL";
	case eCommandClass::PROTECTION: return "PROTECTION";
	case eCommandClass::FIRMWARE_UPDATE_MD: return "FIRMWARE_UPDATE_MD";
	case eCommandClass::DEVICE_RESET_LOCALLY: return "DEVICE_RESET_LOCALLY";
	case eCommandClass::NOTIFICATION: return "NOTIFICATION";
	}
}
