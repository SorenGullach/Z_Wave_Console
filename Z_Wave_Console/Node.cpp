
#include "Node.h"

std::string ZW_Node::ToString() const
{
    std::ostringstream out;

    out << "=== Node " << std::dec << NodeId
        << " (0x" << std::hex << std::uppercase << std::setw(2)
        << std::setfill('0') << NodeId << ") ===\n";

    //
    // Node state
    //
    out << "State            : ";
    switch (nodeState)
    {
    case eNodeState::Awake:  out << "Awake\n"; break;
    case eNodeState::Sleepy: out << "Sleepy\n"; break;
    default:                 out << "Bad\n"; break;
    }

    //
    // Interview state
    //
    out << "Interview        : ";
    switch (interviewState)
    {
    case eInterviewState::Waitting:            out << "Waiting\n"; break;
    case eInterviewState::WattingProtocolInfo: out << "WaitingProtocolInfo\n"; break;
    case eInterviewState::DoneProtocolInfo:    out << "DoneProtocolInfo\n"; break;
    case eInterviewState::FailProtocolInfo:    out << "FailProtocolInfo\n"; break;
    case eInterviewState::WattingNodeInfo:     out << "WaitingNodeInfo\n"; break;
    case eInterviewState::DoneNodeInfo:        out << "DoneNodeInfo\n"; break;
    case eInterviewState::FailNodeInfo:        out << "FailNodeInfo\n"; break;
    case eInterviewState::WattingCCInfo:       out << "WaitingCCInfo\n"; break;
    case eInterviewState::DoneCCInfo:          out << "DoneCCInfo\n"; break;
    case eInterviewState::FailCCInfo:          out << "FailCCInfo\n"; break;
    default:                                   out << "Unknown\n"; break;
    }

    //
    // Manufacturer
    //
    if (manufacturerInfo.hasManufacturerData)
    {
        out << "Manufacturer     : mfg=0x" << std::hex << std::setw(4)
            << manufacturerInfo.mfgId
            << " prodType=0x" << std::setw(4) << manufacturerInfo.prodType
            << " prodId=0x" << std::setw(4) << manufacturerInfo.prodId << "\n";

        if (manufacturerInfo.hasDeviceId)
        {
            out << "Device ID        : type=0x" << std::setw(2)
                << unsigned(manufacturerInfo.deviceIdType)
                << " format=0x" << std::setw(2)
                << unsigned(manufacturerInfo.deviceIdFormat)
                << " data=";

            for (auto b : manufacturerInfo.deviceIdData)
                out << std::setw(2) << unsigned(b);

            out << "\n";
        }
    }
    else
    {
        out << "Manufacturer     : unknown\n";
    }

    //
    // Battery
    //
    if (batteryLevel.has_value())
        out << "Battery          : " << std::dec << unsigned(*batteryLevel) << "%\n";

    //
    // Device classes
    //
    out << "Device classes   : basic=0x" << std::hex << std::setw(2)
        << unsigned(basic)
        << " generic=0x" << std::setw(2) << unsigned(generic)
        << " specific=0x" << std::setw(2) << unsigned(specific) << "\n";

    //
    // Protocol flags
    //
    out << "Protocol flags   : listening=" << (isListening ? "yes" : "no")
        << " routing=" << (isRouting ? "yes" : "no")
        << " speed=" << std::dec << unsigned(supportedSpeed)
        << " protoVer=" << unsigned(protocolVersion) << "\n";

    //
    // Optional flags
    //
    out << "Optional flags   : optFunc=" << (optionalFunctionality ? "yes" : "no")
        << " sensor1000ms=" << (sensor1000ms ? "yes" : "no")
        << " sensor250ms=" << (sensor250ms ? "yes" : "no")
        << " beam=" << (beamCapable ? "yes" : "no")
        << " routingEndNode=" << (routingEndNode ? "yes" : "no")
        << " specificDevice=" << (specificDevice ? "yes" : "no")
        << " controllerNode=" << (controllerNode ? "yes" : "no")
        << " security=" << (security ? "yes" : "no") << "\n";

    //
    // CC values
    //
    if (basicValue)            out << "Basic            : " << unsigned(*basicValue) << "\n";
    if (switchBinaryValue)     out << "Switch Binary    : " << unsigned(*switchBinaryValue) << "\n";
    if (switchMultilevelValue) out << "Switch Multilevel: " << unsigned(*switchMultilevelValue) << "\n";
    if (sensorBinaryValue)     out << "Sensor Binary    : " << unsigned(*sensorBinaryValue) << "\n";

    if (meterInfo.hasValue)
    {
        out << "Meter            : type=" << unsigned(meterInfo.meterType)
            << " value=" << unsigned(meterInfo.value) << "\n";
    }

    if (protectionState)
        out << "Protection       : " << unsigned(*protectionState) << "\n";

    //
    // CC report structures
    //
    if (configurationInfo.hasLastReport)
        out << "Configuration    : param=" << unsigned(configurationInfo.paramNumber)
        << " bytes=" << configurationInfo.raw.size() << "\n";

	for (const auto& assoc : associationInfo)
	{
        out << "Association      : group=" << unsigned(assoc.groupId);
            for(const auto& node : assoc.nodes)
				out << " Node=" << unsigned(node);
        out << "\n";
	}

    if (multiChannelInfo.hasLastReport)
        out << "MultiChannel     : bytes=" << multiChannelInfo.raw.size() << "\n";

    if (multiChannelAssociationInfo.hasLastReport)
        out << "MC Association   : group=" << unsigned(multiChannelAssociationInfo.groupId)
        << " bytes=" << multiChannelAssociationInfo.raw.size() << "\n";

    //
    // Command Classes
    //
    size_t ccCount = 0;
    for (const auto& cc : ccs)
        if (cc.supported)
            ccCount++;

    out << "Command Classes  : " << std::dec << ccCount << "\n";

    if (ccCount > 0)
    {
        out << "  ";
        int col = 0;

        for (size_t i = 0; i < ccs.size(); ++i)
        {
            if (!ccs[i].supported)
                continue;

            out << "0x" << std::hex << std::setw(2) << std::setfill('0')
                << std::uppercase << i
                << " v=" << std::dec << unsigned(ccs[i].version);

            if (ccs[i].versionOk)
                out << "(ok)";

            col++;
            if (col % 4 == 0)
                out << "\n  ";
            else
                out << "   ";
        }
        out << "\n";
    }

    return out.str();
}
