
#include <string>
#include "NodeInfo.h"

std::string NodeInfoFormatter::SupportedSpeedName() const
{
	using eSupportedSpeeds = NodeInfo::ProtocolInfo::eSupportedSpeeds;
	switch (ni.protocolInfo.SupportedSpeed())
	{
	case eSupportedSpeeds::Speed9k6:   return "9.6 kbps";
	case eSupportedSpeeds::Speed40k:   return "40 kbps";
	case eSupportedSpeeds::Speed100k:  return "100 kbps";
	case eSupportedSpeeds::SpeedUnknown:
		return "Unknown";
	}
	return "Unknown";
}

std::string NodeInfoFormatter::ProtocolVersionName() const
{
	using eProtocolVersions = NodeInfo::ProtocolInfo::eProtocolVersions;
	switch (ni.protocolInfo.ProtocolVersion())
	{
	case eProtocolVersions::V1: return "Z-Wave Protocol v1";
	case eProtocolVersions::V2: return "Z-Wave Protocol v2";
	case eProtocolVersions::V3: return "Z-Wave Protocol v3";
	}
	return "Unknown Protocol Version";
}

std::string NodeInfoFormatter::BasicDeviceTypeName() const
{
	using eBasicDeviceType = NodeInfo::ProtocolInfo::eBasicDeviceType;
	switch (ni.protocolInfo.BasicType())
	{
	case eBasicDeviceType::Controller:
		return "Controller";

	case eBasicDeviceType::StaticController:
		return "Static Controller";

	case eBasicDeviceType::Slave:
		return "Slave";

	case eBasicDeviceType::RoutingSlave:
		return "Routing Slave";

	case eBasicDeviceType::Unknown:
		return "Unknown";
	}
	return "Unknown";
}

std::string NodeInfoFormatter::GenericDeviceClassName() const
{
	using eGenericDeviceClass = NodeInfo::ProtocolInfo::eGenericDeviceClass;
	switch (ni.protocolInfo.GenericDeviceClass())
	{
	case eGenericDeviceClass::GenericController:   return "Generic Controller";
	case eGenericDeviceClass::StaticController:    return "Static Controller";
	case eGenericDeviceClass::AvControlPoint:      return "AV Control Point";
	case eGenericDeviceClass::Display:             return "Display";
	case eGenericDeviceClass::NetworkExtender:     return "Network Extender";
	case eGenericDeviceClass::Appliance:           return "Appliance";
	case eGenericDeviceClass::SensorNotification:  return "Notification Sensor";
	case eGenericDeviceClass::Thermostat:          return "Thermostat";
	case eGenericDeviceClass::WindowCovering:      return "Window Covering";
	case eGenericDeviceClass::RepeaterSlave:       return "Repeater Slave";

	case eGenericDeviceClass::SwitchBinary:        return "Binary Switch";
	case eGenericDeviceClass::SwitchMultilevel:    return "Multilevel Switch";
	case eGenericDeviceClass::SwitchRemote:        return "Remote Switch";
	case eGenericDeviceClass::SwitchToggle:        return "Toggle Switch";

	case eGenericDeviceClass::SensorBinary:        return "Binary Sensor";
	case eGenericDeviceClass::SensorMultilevel:    return "Multilevel Sensor";

	case eGenericDeviceClass::PulseMeter:          return "Pulse Meter";
	case eGenericDeviceClass::Meter:               return "Meter";

	case eGenericDeviceClass::EntryControl:        return "Entry Control";

	case eGenericDeviceClass::MultiChannelDevice: return "Multi-Channel Device";

	case eGenericDeviceClass::Unknown:
		return "Unknown Generic Device Class";
	}
	return "Unknown Generic Device Class";
}

std::string NodeInfoFormatter::SpecificDeviceClassName() const
{
	using eGeneric = NodeInfo::ProtocolInfo::eGenericDeviceClass;

	const uint8_t spec = ni.protocolInfo.SpecificDeviceClass();

	switch (ni.protocolInfo.GenericDeviceClass())
	{
		// ---------------------------------------------------------
		// Switch Multilevel (0x11)
		// ---------------------------------------------------------
	case eGeneric::SwitchMultilevel:
		switch (static_cast<CC_SwitchMultilevel::eSpecificDeviceClass>(spec))
		{
		case CC_SwitchMultilevel::eSpecificDeviceClass::PowerSwitchMultilevel: return "Power Multilevel Switch";
		case CC_SwitchMultilevel::eSpecificDeviceClass::MotorMultiposition:    return "Motor Multiposition";
		case CC_SwitchMultilevel::eSpecificDeviceClass::SceneSwitchMultilevel: return "Scene Multilevel Switch";
		case CC_SwitchMultilevel::eSpecificDeviceClass::ClassA:                return "Class A";
		case CC_SwitchMultilevel::eSpecificDeviceClass::ClassB:                return "Class B";
		case CC_SwitchMultilevel::eSpecificDeviceClass::ClassC:                return "Class C";
		case CC_SwitchMultilevel::eSpecificDeviceClass::Unknown:			   return "Unknown";
		}
		break; // no default so compiler warns if enum extended

		// ---------------------------------------------------------
		// Switch Binary (0x10)
		// ---------------------------------------------------------
	case eGeneric::SwitchBinary:
		switch (static_cast<CC_SwitchBinary::eSpecificDeviceClass>(spec))
		{
		case CC_SwitchBinary::eSpecificDeviceClass::PowerSwitchBinary: return "Power Binary Switch";
		case CC_SwitchBinary::eSpecificDeviceClass::SceneSwitchBinary: return "Scene Binary Switch";
		case CC_SwitchBinary::eSpecificDeviceClass::Unknown:		   return "Unknown";
		}
		break;
		/*
		// ---------------------------------------------------------
		// Sensor Multilevel (0x21)
		// ---------------------------------------------------------
	case eGeneric::SensorMultilevel:
		switch (static_cast<CC_SensorMultilevel::eSpecificDeviceClass>(spec))
		{
		case CC_SensorMultilevel::eSpecificDeviceClass::AirTemperature:        return "Air Temperature Sensor";
		case CC_SensorMultilevel::eSpecificDeviceClass::GeneralPurpose:        return "General Purpose Sensor";
		case CC_SensorMultilevel::eSpecificDeviceClass::Luminance:             return "Luminance Sensor";
		case CC_SensorMultilevel::eSpecificDeviceClass::Power:                 return "Power Sensor";
		case CC_SensorMultilevel::eSpecificDeviceClass::Humidity:              return "Humidity Sensor";
		case CC_SensorMultilevel::eSpecificDeviceClass::Velocity:              return "Velocity Sensor";
		case CC_SensorMultilevel::eSpecificDeviceClass::Direction:             return "Direction Sensor";
		case CC_SensorMultilevel::eSpecificDeviceClass::AtmosphericPressure:   return "Atmospheric Pressure Sensor";
		case CC_SensorMultilevel::eSpecificDeviceClass::Unknown:				return "Unknown";
		}
		break;
		*/
		// ---------------------------------------------------------
		// Sensor Binary (0x20)
		// ---------------------------------------------------------
	case eGeneric::SensorBinary:
		switch (static_cast<CC_SensorBinary::eSpecificDeviceClass>(spec))
		{
		case CC_SensorBinary::eSpecificDeviceClass::GeneralPurpose: return "General Purpose Binary Sensor";
		case CC_SensorBinary::eSpecificDeviceClass::Smoke:          return "Smoke Sensor";
		case CC_SensorBinary::eSpecificDeviceClass::CO:             return "CO Sensor";
		case CC_SensorBinary::eSpecificDeviceClass::CO2:            return "CO2 Sensor";
		case CC_SensorBinary::eSpecificDeviceClass::Heat:           return "Heat Sensor";
		case CC_SensorBinary::eSpecificDeviceClass::Water:          return "Water Sensor";
		case CC_SensorBinary::eSpecificDeviceClass::Freeze:         return "Freeze Sensor";
		case CC_SensorBinary::eSpecificDeviceClass::Tamper:         return "Tamper Sensor";
		case CC_SensorBinary::eSpecificDeviceClass::Unknown:		   return "Unknown";
		}
		break;
		/*
		// ---------------------------------------------------------
		// Entry Control (0x40)
		// ---------------------------------------------------------
	case eGeneric::EntryControl:
		switch (static_cast<CC_EntryControl::eSpecificDeviceClass>(spec))
		{
		case CC_EntryControl::eSpecificDeviceClass::DoorLock:         return "Door Lock";
		case CC_EntryControl::eSpecificDeviceClass::AdvancedDoorLock: return "Advanced Door Lock";
			case CC_EntryControl::eSpecificDeviceClass::Unknown:		   return "Unknown";
		}
		break;
		*/
		// ---------------------------------------------------------
		// Unknown generic class
		// ---------------------------------------------------------
	default:
		break;
	}

	return "Unknown Specific Device Class";
}

std::string NodeInfoFormatter::ToTextProtocol() const
{
	const auto& p = ni.protocolInfo;

	std::string s;

	s += "Basic Device Type   : " + BasicDeviceTypeName() + "\n";
	s += "Generic Device Class: " + GenericDeviceClassName() + "\n";
	s += "Specific Device Class: " + SpecificDeviceClassName() + "\n";

	s += "Listening           : " + std::string(p.IsListening() ? "Yes" : "No") + "\n";
	s += "Routing             : " + std::string(p.IsRouting() ? "Yes" : "No") + "\n";

	s += "Supported Speed     : " + SupportedSpeedName() + "\n";
	s += "Protocol Version    : " + ProtocolVersionName() + "\n";

	s += "Optional Func       : " + std::string(p.OptionalFunctionality() ? "Yes" : "No") + "\n";
	s += "Sensor 1000ms       : " + std::string(p.Sensor1000ms() ? "Yes" : "No") + "\n";
	s += "Sensor 250ms        : " + std::string(p.Sensor250ms() ? "Yes" : "No") + "\n";
	s += "Beam Capable        : " + std::string(p.BeamCapability() ? "Yes" : "No") + "\n";
	s += "Routing End Node    : " + std::string(p.RoutingEndNode() ? "Yes" : "No") + "\n";
	s += "Specific Device     : " + std::string(p.SpecificDevice() ? "Yes" : "No") + "\n";
	s += "Controller Node     : " + std::string(p.ControllerNode() ? "Yes" : "No") + "\n";
	s += "Security            : " + std::string(p.Security() ? "Yes" : "No") + "\n";

	s += "Speed Extension     : " + std::to_string(static_cast<int>(p.SpeedExtension())) + "\n";

	return s;
}

std::string NodeInfoFormatter::ToText() const
{
	return ToTextProtocol();
}
