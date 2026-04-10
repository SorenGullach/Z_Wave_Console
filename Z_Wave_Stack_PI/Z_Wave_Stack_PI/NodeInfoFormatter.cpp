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
	case eBasicDeviceType::Controller:			return "Controller";
	case eBasicDeviceType::StaticController:	return "Static Controller";
	case eBasicDeviceType::Slave:				return "Slave";
	case eBasicDeviceType::RoutingSlave:		return "Routing Slave";
	case eBasicDeviceType::Unknown:				return "Unknown";
	}
	return "Unknown";
}

std::string NodeInfoFormatter::ManufacturerName() const
{
	const auto& m = ni.manufacturerInfo;

	switch (m.mfgId)
	{
	case 0x007A: return "Merten / Schneider Electric (Leviton firmware)";
	case 0x0086: return "Merten / Schneider Electric";
	case 0x0002: return "ACT HomePro";
	case 0x0003: return "GE / Jasco";
	case 0x001A: return "Fibaro";
	case 0x010F: return "Fibaro Group";
	case 0x014A: return "Zooz";
	case 0x0154: return "Popp & Co";
	case 0x018E: return "Aeotec";
	case 0x019B: return "Philio";
	case 0x021F: return "Qubino";
	case 0x0224: return "Heatit / ThermoFloor";
	case 0x0258: return "Shelly";
	default:     return "Unknown Manufacturer";
	}
}

std::string NodeInfoFormatter::ProductName() const
{
	const auto& m = ni.manufacturerInfo;

	// Merten 506004
	if (m.mfgId == 0x0086 && m.prodType == 0x0002 && m.prodId == 0x0001)
		return "Merten 506004 Battery Wall Transmitter";

	if (m.mfgId == 0x007A && m.prodType == 0x8001 && m.prodId == 0x8002)
		return "Merten CONNECT 506004 Battery Wall Transmitter";

	// Heatit Z-Push Button 8
	if (m.mfgId == 0x0224 && m.prodType == 0x0003 && m.prodId == 0x0001)
		return "Heatit Z-Push Button 8";

	// Aeotec Nano Switch
	if (m.mfgId == 0x018E && m.prodType == 0x0003 && m.prodId == 0x006F)
		return "Aeotec Nano Switch";

	return "Unknown Product";
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

	case eGenericDeviceClass::MultiChannelDevice:  return "Multi-Channel Device";

	case eGenericDeviceClass::Unknown:			   return "Unknown Generic Device Class";
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

	std::string s = "========== Protocol ==========\n";

	s += std::format("Basic Device Type   : {}\n", BasicDeviceTypeName());
	s += std::format("Generic Device Class: {}\n", GenericDeviceClassName());
	s += std::format("Specific Device Class: {}\n", SpecificDeviceClassName());

	s += std::format("Listening           : {}{}\n",
					 p.IsListening() ? "Yes" : "No",
					 p.IsListening() ? " (Always awake)" : "");

	s += std::format("Routing             : {}{}\n",
					 p.IsRouting() ? "Yes" : "No",
					 p.IsRouting() ? " (Can forward frames)" : "");

	s += std::format("Supported Speed     : {}\n", SupportedSpeedName());
	s += std::format("Protocol Version    : {}\n", ProtocolVersionName());

	s += std::format("Optional Func       : {}\n", p.OptionalFunctionality() ? "Yes" : "No");
	s += std::format("Sensor 1000ms       : {}\n", p.Sensor1000ms() ? "Yes" : "No");
	s += std::format("Sensor 250ms        : {}\n", p.Sensor250ms() ? "Yes" : "No");

	s += std::format("Beam Capable        : {}{}\n",
					 p.BeamCapability() ? "Yes" : "No",
					 p.BeamCapability() ? " (FLiRS capable)" : "");

	s += std::format("Routing End Node    : {}\n", p.RoutingEndNode() ? "Yes" : "No");
	s += std::format("Specific Device     : {}\n", p.SpecificDevice() ? "Yes" : "No");
	s += std::format("Controller Node     : {}\n", p.ControllerNode() ? "Yes" : "No");

	s += std::format("Security            : {}{}\n",
					 p.Security() ? "Yes" : "No",
					 p.Security() ? " (S0/S2 supported)" : "");

	s += std::format("Speed Extension     : {}\n",
					 static_cast<int>(p.SpeedExtension()));

	return s;
}

std::string NodeInfoFormatter::ToTextCommandClass() const
{
	const auto ccs = ni.GetSupportedCCs();

	std::string s = "======= Command Classes =======\n";
	if (ccs.empty())
		return "Command Classes    : n/a\n";

	for (auto cc : ccs)
	{
		const auto* tag = ni.GetCC(cc);
		const std::string name = CommandClassToString(cc);

		s += std::format("CC 0x{:02X} V{} {}\n",
						 static_cast<int>(cc),
						 tag->versionOk ? tag->version : 0,
						 name);
	}

	return s;
}

// ---------------------------------------------------------
// Manufacturer
// ---------------------------------------------------------
std::string NodeInfoFormatter::ToTextManufacturer() const
{
	const auto& m = ni.manufacturerInfo;

	std::string s = "======== Manufacturer ========\n";

	if (!m.hasManufacturerData)
	{
		s += "Manufacturer Data   : n/a\n";
		return s;
	}

	const std::string mfgName = ManufacturerName();
	const std::string prodName = ProductName();

	s += std::format("Manufacturer Id     : 0x{:04X} ({})\n",
					 m.mfgId, mfgName);

	s += std::format("Product Type        : 0x{:04X}\n", m.prodType);

	s += std::format("Product Id          : 0x{:04X} ({})\n",
					 m.prodId, prodName);

	if (m.hasDeviceId)
	{
		s += std::format("Device Id Type      : 0x{:02X}\n", m.deviceIdType);
		s += std::format("Device Id Format    : 0x{:02X}\n", m.deviceIdFormat);
	}

	return s;
}

// ---------------------------------------------------------
// Multi Channel
// ---------------------------------------------------------
std::string NodeInfoFormatter::ToTextMultiChannel() const
{
	const auto& mc = ni.multiChannel;

	if (!mc.hasEndpointReport)
		return "";

	std::string s = "======== Multi Channel =======\n";
	s += std::format("Endpoint Count      : {}\n", mc.endpointCount);

	for (const auto& ep : mc.endpoints)
	{
		std::string gen = GenericDeviceClassName();
		std::string spec = SpecificDeviceClassName();

		s += std::format("Endpoint {}          : {} / {} ({} CCs){}\n",
						 ep.endpointId,
						 gen,
						 spec,
						 ep.supportedCCs.size(),
						 ep.hasCapabilityReport ? "" : " (pending)");
	}

	return s;
}

// ---------------------------------------------------------
// Wake Up
// ---------------------------------------------------------
std::string NodeInfoFormatter::ToTextWakeUp() const
{
	const auto& w = ni.wakeUpInfo;

	std::string s = "=========== Wake Up ==========\n";

	if (!w.hasLastReport)
	{
		s += "Wake Up Info        : n/a\n";
		return s;
	}

	s += std::format("Interval            : {}\n", w.wakeUpInterval);
	s += std::format("Minimum             : {}\n", w.wakeUpMin);
	s += std::format("Maximum             : {}\n", w.wakeUpMax);
	s += std::format("Default             : {}\n", w.wakeUpDefault);

	return s;
}

// ---------------------------------------------------------
// Values
// ---------------------------------------------------------
static std::string OptionalByteText(const std::optional<uint8_t>& value)
{
	if (!value.has_value())
		return "n/a";

	uint8_t v = value.value();
	std::string meaning;

	if (v == 0x00) meaning = "OFF/Idle";
	else if (v == 0xFF) meaning = "ON";
	else meaning = std::format("{}%", (v * 100) / 255);

	return std::format("0x{:02X} ({})", v, meaning);
}

std::string NodeInfoFormatter::ToTextValues() const
{
	std::string s = "=========== Values ===========\n";

	s += "Battery Level       : " + OptionalByteText(ni.batteryLevel) + "\n";
	s += "Basic Value         : ";
	for (auto v : ni.basicValue)
	{
		s += std::to_string(v.second);
		s += ' ';
	}
	s += "\n";
	s += "Switch Binary Value : " + OptionalByteText(ni.switchBinaryValue) + "\n";
	s += "Switch Multi Value  : " + OptionalByteText(ni.switchMultilevelValue) + "\n";
	s += "Sensor Binary Value : " + OptionalByteText(ni.sensorBinaryValue) + "\n";
	s += "Protection State    : " + OptionalByteText(ni.protectionState) + "\n";

	if (ni.meterInfo.hasValue)
	{
		s += std::format("Meter               : type=0x{:02X} value={}\n",
						 ni.meterInfo.meterType,
						 ni.meterInfo.value);
	}
	else
	{
		s += "Meter               : n/a\n";
	}

	return s;
}

std::string NodeInfoFormatter::ToTextHeader() const
{
	return std::format(
		"======= Node {} - {} / {}\n",
		ni.nodeId,
		GenericDeviceClassName(),
		SpecificDeviceClassName()
	);
}

std::string NodeInfoFormatter::ToTextLocation() const
{
	return std::format("Location            : {} / {}\n", ni.Floor, ni.Room);
}

std::string NodeInfoFormatter::ToTextState() const
{
	std::string state =
		ni.GetState() == NodeInfo::eNodeState::Awake ? "Awake" :
		ni.GetState() == NodeInfo::eNodeState::Sleepy ? "Sleepy" : "New";

	return std::format("Node State          : {}\n", state);
}
std::string NodeInfoFormatter::ToTextInterview() const
{
	using S = NodeInfo::eInterviewState;

	const char* name = "Unknown";
	switch (ni.GetInterviewState())
	{
	case S::NotInterviewed:        name = "NotInterviewed"; break;
	case S::ProtocolInfoPending:   name = "ProtocolInfoPending"; break;
	case S::ProtocolInfoDone:      name = "ProtocolInfoDone"; break;
	case S::NodeInfoPending:       name = "NodeInfoPending"; break;
	case S::NodeInfoDone:          name = "NodeInfoDone"; break;
	case S::CCVersionPending:      name = "CCVersionPending"; break;
	case S::CCVersionDone:         name = "CCVersionDone"; break;
	case S::CCMnfcSpecPending:     name = "CCMnfcSpecPending"; break;
	case S::CCMnfcSpecDone:        name = "CCMnfcSpecDone"; break;
	case S::CCMultiChannelPending: name = "CCMultiChannelPending"; break;
	case S::CCMultiChannelDone:    name = "CCMultiChannelDone"; break;
	case S::InterviewDone:         name = "InterviewDone"; break;
	}

	return std::format("Interview State     : {}\n", name);
}

std::string NodeInfoFormatter::ToTextAssociations() const
{
	std::string s = "======== Associations ========\n";

	if (ni.multiChannelAssociationGroups.empty())
	{
		s += "No association groups\n";
		return s;
	}

	for (const auto& grp : ni.multiChannelAssociationGroups)
	{
		s += std::format("Group {} (max {}){}\n",
						 grp.groupId,
						 grp.maxNodes,
						 grp.hasLastReport ? "" : " (pending)");

		for (const auto& m : grp.members)
		{
			if (!m.valid) continue;

			s += std::format("  Node {} EP {}\n",
							 m.nodeId,
							 m.endpointId);
		}
	}

	return s;
}

std::string NodeInfoFormatter::ToTextConfiguration() const
{
	std::string s = "======= Configuration ========\n";

	bool any = false;
	for (const auto& cfg : ni.configurationInfo)
	{
		if (!cfg.valid) continue;
		any = true;

		s += std::format("Param {:3}  Size={}  Value={}\n",
						 cfg.paramNumber,
						 cfg.size,
						 cfg.value);
	}

	if (!any)
		s += "No configuration parameters\n";

	return s;
}

std::string NodeInfoFormatter::ToTextMeter() const
{
	std::string s = "============ Meter ===========\n";

	if (!ni.meterInfo.hasValue)
	{
		s += "Meter               : n/a\n";
		return s;
	}

	s += std::format("Meter Type          : 0x{:02X}\n", ni.meterInfo.meterType);
	s += std::format("Meter Value         : {}\n", ni.meterInfo.value);

	return s;
}

std::string NodeInfoFormatter::ToText() const
{
	std::string s;

	s += ToTextHeader();
	s += ToTextLocation();
	s += ToTextState();
	s += ToTextInterview();
	s += ToTextProtocol();
	s += ToTextManufacturer();
	s += ToTextCommandClass();
	s += ToTextMultiChannel();
	s += ToTextAssociations();
	s += ToTextConfiguration();
	s += ToTextWakeUp();
	s += ToTextValues();
	s += ToTextMeter();

	s += "\n";
	return s;
}

std::string NodeInfoFormatter::ToSummary() const
{
	const auto& p = ni.protocolInfo;

	std::string s;

	// Header
	s += std::format("Node {} — {} ({})\n",
					 ni.nodeId,
					 GenericDeviceClassName(),
					 SpecificDeviceClassName());

	// State
	s += std::format("State: {}, {}\n",
					 (ni.GetState() == NodeInfo::eNodeState::Awake ? "Awake" :
					  ni.GetState() == NodeInfo::eNodeState::Sleepy ? "Sleepy" : "New"),
					 (ni.GetInterviewState() == NodeInfo::eInterviewState::InterviewDone ?
					  "InterviewDone" : "InterviewPending"));

	// Protocol
	s += std::format("Protocol: {} @ {}\n",
					 ProtocolVersionName(),
					 SupportedSpeedName());

	s += std::format("Listening: {}, Routing: {}\n",
					 p.IsListening() ? "Yes" : "No",
					 p.IsRouting() ? "Yes" : "No");

	// Manufacturer
	if (ni.manufacturerInfo.hasManufacturerData)
	{
		s += std::format("Manufacturer: 0x{:04X}, Type 0x{:04X}, Id 0x{:04X}\n",
						 ni.manufacturerInfo.mfgId,
						 ni.manufacturerInfo.prodType,
						 ni.manufacturerInfo.prodId);
	}

	// Key CCs
	s += "CCs: ";
	bool first = true;
	for (auto cc : ni.GetSupportedCCs())
	{
		// Only show functional CCs
		switch (cc)
		{
		case eCommandClass::BASIC:
		case eCommandClass::SWITCH_BINARY:
		case eCommandClass::SWITCH_MULTILEVEL:
		case eCommandClass::SENSOR_BINARY:
		case eCommandClass::METER:
		case eCommandClass::BATTERY:
		case eCommandClass::WAKE_UP:
		case eCommandClass::MULTI_CHANNEL:
			if (!first) s += ", ";
			s += CommandClassToString(cc);
			first = false;
			break;
		default:
			break;
		}
	}
	if (first) s += "None";
	s += "\n";

	// Key values
	auto opt = [](const std::optional<uint8_t>& v)
		{
			return v.has_value() ? std::format("0x{:02X}", v.value()) : std::string("n/a");
		};

	std::string sBV;
	for (auto v : ni.basicValue)
	{
		sBV += std::to_string(v.second);
		sBV += ' ';
	}

	s += std::format("Values: Basic={}, Switch={}, Sensor={}, Battery={}\n",
					 sBV,
					 opt(ni.switchBinaryValue),
					 opt(ni.sensorBinaryValue),
					 opt(ni.batteryLevel));

	// Endpoints
	if (ni.multiChannel.hasEndpointReport)
	{
		s += std::format("Endpoints: {} total\n", ni.multiChannel.endpointCount);
		for (const auto& ep : ni.multiChannel.endpoints)
		{
			s += std::format("  EP{}: generic=0x{:02X}, specific=0x{:02X}, CCs={}\n",
							 ep.endpointId,
							 ep.generic,
							 ep.specific,
							 ep.supportedCCs.size());
		}
	}

	return s;
}
