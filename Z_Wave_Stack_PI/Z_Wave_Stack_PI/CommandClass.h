#pragma once

#include <cstdint>
#include <string>

enum class eCommandClass : uint8_t
{
    NO_OPERATION = 0x00,
    APPLICATION_STATUS = 0x22,

    BASIC = 0x20,
    SWITCH_BINARY = 0x25,
    SWITCH_MULTILEVEL = 0x26,

    SENSOR_BINARY = 0x30,
    SENSOR_MULTILEVEL = 0x31,
    METER = 0x32,
    COLOR_SWITCH = 0x33,

    ASSOCIATION = 0x85,
    ASSOCIATION_GRP_INFO = 0x59,
    MULTI_CHANNEL_ASSOCIATION = 0x8E,

    MULTI_CHANNEL = 0x60,   // NOTE: Only valid if device has endpoints

    CONFIGURATION = 0x70,
    MANUFACTURER_SPECIFIC = 0x72,
    POWERLEVEL = 0x73,
    PROTECTION = 0x75,
    FIRMWARE_UPDATE_MD = 0x7A,

    DEVICE_RESET_LOCALLY = 0x5A,
    NOTIFICATION = 0x71,
    ZWAVEPLUS_INFO = 0x5E,

    SUPERVISION = 0x6C,
    TRANSPORT_SERVICE = 0x55,
    CRC16_ENCAP = 0x56,
    MULTI_CMD = 0x8F,

    SECURITY_0 = 0x98,
    SECURITY_2 = 0x9F,

    VERSION = 0x86,
    WAKE_UP = 0x84,
    BATTERY = 0x80,

    CENTRAL_SCENE = 0x5B,
    SCENE_ACTIVATION = 0x2B,
    SCENE_CONTROLLER_CONF = 0x2D,

    INDICATOR = 0x87,
    USER_CODE = 0x63,
    DOOR_LOCK = 0x62,
    BARRIER_OPERATOR = 0x66,
    SOUND_SWITCH = 0x79,
    ENTRY_CONTROL = 0x6F,

    THERMOSTAT_MODE = 0x40,
    THERMOSTAT_SETPOINT = 0x43,
    THERMOSTAT_FAN_MODE = 0x44,
    THERMOSTAT_OPERATING_STATE = 0x42
};

inline std::string CommandClassToString(eCommandClass cc)
{
	switch (cc)
	{
	case eCommandClass::BASIC: return "BASIC";                                  // 0x20
	case eCommandClass::NO_OPERATION: return "NO_OPERATION";                    // 0x00
	case eCommandClass::SWITCH_BINARY: return "SWITCH_BINARY";                  // 0x25
	case eCommandClass::SWITCH_MULTILEVEL: return "SWITCH_MULTILEVEL";          // 0x26
	case eCommandClass::SENSOR_BINARY: return "SENSOR_BINARY";                  // 0x30
	case eCommandClass::SENSOR_MULTILEVEL: return "SENSOR_MULTILEVEL";          // 0x31
	case eCommandClass::METER: return "METER";                                  // 0x32
	case eCommandClass::COLOR_SWITCH: return "COLOR_SWITCH";                    // 0x33
	case eCommandClass::ASSOCIATION: return "ASSOCIATION";                      // 0x85
	case eCommandClass::ASSOCIATION_GRP_INFO: return "ASSOCIATION_GRP_INFO";    // 0x59
	case eCommandClass::CONFIGURATION: return "CONFIGURATION";                  // 0x70
	case eCommandClass::MANUFACTURER_SPECIFIC: return "MANUFACTURER_SPECIFIC";  // 0x72
	case eCommandClass::POWERLEVEL: return "POWERLEVEL";                        // 0x73
	case eCommandClass::PROTECTION: return "PROTECTION";                        // 0x75
	case eCommandClass::FIRMWARE_UPDATE_MD: return "FIRMWARE_UPDATE_MD";        // 0x7A
	case eCommandClass::DEVICE_RESET_LOCALLY: return "DEVICE_RESET_LOCALLY";    // 0x5A
	case eCommandClass::NOTIFICATION: return "NOTIFICATION";                    // 0x71
	case eCommandClass::BATTERY: return "BATTERY";                              // 0x50
	case eCommandClass::DOOR_LOCK: return "DOOR_LOCK";                          // 0x62
	case eCommandClass::MULTI_CHANNEL: return "MULTI_CHANNEL";                  // 0x60
	case eCommandClass::MULTI_CHANNEL_ASSOCIATION: return "MULTI_CHANNEL_ASSOCIATION"; // 0x8E
	case eCommandClass::SECURITY_2: return "SECURITY_2";                        // 0x9F
	case eCommandClass::SUPERVISION: return "SUPERVISION";                      // 0x6C
	case eCommandClass::TRANSPORT_SERVICE: return "TRANSPORT_SERVICE";          // 0x55
	case eCommandClass::VERSION: return "VERSION";                              // 0x86
	case eCommandClass::WAKE_UP: return "WAKE_UP";                              // 0x84
	case eCommandClass::APPLICATION_STATUS: return "APPLICATION_STATUS";        // 0x22
	case eCommandClass::ZWAVEPLUS_INFO: return "ZWAVEPLUS_INFO";                // 0x5E
	case eCommandClass::CRC16_ENCAP: return "CRC16_ENCAP";                      // 0x56
	case eCommandClass::MULTI_CMD: return "MULTI_CMD";                          // 0x8F
	case eCommandClass::SECURITY_0: return "SECURITY_0";                        // 0x98
	case eCommandClass::CENTRAL_SCENE: return "CENTRAL_SCENE";                  // 0x5B
	case eCommandClass::SCENE_ACTIVATION: return "SCENE_ACTIVATION";            // 0x2B
	case eCommandClass::SCENE_CONTROLLER_CONF: return "SCENE_CONTROLLER_CONF";  // 0x2D
	case eCommandClass::INDICATOR: return "INDICATOR";                          // 0x87
	case eCommandClass::USER_CODE: return "USER_CODE";                          // 0x63
	case eCommandClass::BARRIER_OPERATOR: return "BARRIER_OPERATOR";            // 0x66
	case eCommandClass::SOUND_SWITCH: return "SOUND_SWITCH";                    // 0x79
	case eCommandClass::ENTRY_CONTROL: return "ENTRY_CONTROL";                  // 0x6F
	case eCommandClass::THERMOSTAT_MODE: return "THERMOSTAT_MODE";              // 0x40
	case eCommandClass::THERMOSTAT_SETPOINT: return "THERMOSTAT_SETPOINT";      // 0x43
	case eCommandClass::THERMOSTAT_FAN_MODE: return "THERMOSTAT_FAN_MODE";      // 0x44
	case eCommandClass::THERMOSTAT_OPERATING_STATE: return "THERMOSTAT_OPERATING_STATE"; // 0x42
	default: return "UNKNOWN";
	}
}

