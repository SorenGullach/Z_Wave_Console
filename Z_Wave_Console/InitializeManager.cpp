#include "Logging.h"
#include "InitializeManager.h"

#include <sstream>

#include <sstream>

/////////////////////////////////////////////////
// GetInitData
/////////////////////////////////////////////////
void ZW_InitializeManager::GetInitData()
{
	ZW_APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_INIT_DATA);
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> GetInitData: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_INIT_DATA));
	enqueue(frame);
}

void ZW_InitializeManager::DecodeInitData(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id, which is in the frame function id):
	// [0] Z-Wave API Version
	// [1] Z-Wave API Capabilities
	// [2] Z-Wave Node List Length (N)
	// [3..3+N-1] Z-Wave Node List (bitmask)
	// [3+N] Chip Type
	// [3+N+1] Chip Version
	if (payload.size() < 3)
		return;

	module.ApiVersion = payload[0];
	module.ApiCapabilities = payload[1];
	module.IsEndNode = (module.ApiCapabilities & 0x01) > 0;
	module.HasTimerFunctions = (module.ApiCapabilities & 0x02) > 0;
	module.IsPrimarayController = (module.ApiCapabilities & 0x04) > 0;
	module.HasSISFunctions = (module.ApiCapabilities & 0x08) > 0;

	module.NodeListLength = payload[2];

	const size_t nodeListOffset = 3;
	if (payload.size() < nodeListOffset + module.NodeListLength)
		return;

	// extract the node list, from an array of bits (byte,bit) 0,0 = 1  0,1 = 2  1,0 = 9
	module.NodeIds.clear();
	for (size_t i = 0; i < module.NodeListLength; i++)
	{
		for (size_t j = 0; j < 8; j++)
		{
			bool nodeExists = ((payload[nodeListOffset + i] >> j) & 0x01) > 0;
			if (nodeExists)
				module.NodeIds.push_back(node_t(static_cast<uint8_t>((i * 8 + j) + 1)));
		}
	}

	const size_t chipOffset = nodeListOffset + module.NodeListLength;
	if (payload.size() >= chipOffset + 2)
	{
		module.ChipType = (ZW_Module::eChipTypes)payload[chipOffset];
		module.ChipVersion = payload[chipOffset + 1];
	}

	Log.AddL(eLogTypes::DVC, MakeTag(), "<< GetInitData ApiVersion={} Caps={} NodeListLen={} ChipType={} ChipVer={} ",
			 module.ApiVersion,
			 module.ApiCapabilities,
			 module.NodeListLength,
			 module.ChipNames(),
			 module.ChipVersion);
	Log.AddL(eLogTypes::DVC, MakeTag(), "<< GetInitData: parsed nodeIds={} (first={}, last={})", module.NodeIds.size(), module.NodeIds.empty() ? node_t{} : module.NodeIds.front(), module.NodeIds.empty() ? node_t{} : module.NodeIds.back());
	NotifyUI(UINotify::ControllerChanged);
}

/////////////////////////////////////////////////
// GetControllerCapabilities	
/////////////////////////////////////////////////
void ZW_InitializeManager::GetControllerCapabilities()
{
	ZW_APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES);
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> GetControllerCapabilities: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES));
	enqueue(frame);
}

void ZW_InitializeManager::DecodeControllerCapabilities(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id):
	// [0] Controller capabilities bitmask
	if (payload.size() < 1)
		return;

	uint8_t ControllerCapabilities = payload[0];
	module.IsSecondaryController = (ControllerCapabilities & 0x01) != 0;
	module.IsOtherNetwork = (ControllerCapabilities & 0x02) != 0;
	module.IsSISPresent = (ControllerCapabilities & 0x04) != 0;
	module.IsSUCEnabled = (ControllerCapabilities & 0x10) != 0;
	module.IsNoNodesIncluded = (ControllerCapabilities & 0x20) != 0;

	Log.AddL(eLogTypes::DVC,
			 MakeTag(),
			 "<< GetControllerCapabilities: caps=0x{:02X} secondary={} otherNet={} sisPresent={} sucEnabled={} noNodesIncluded={} (payloadLen={})",
			 ControllerCapabilities,
			 module.IsSecondaryController,
			 module.IsOtherNetwork,
			 module.IsSISPresent,
			 module.IsSUCEnabled,
			 module.IsNoNodesIncluded,
			 payload.size());
	
	NotifyUI(UINotify::ControllerChanged);
}

/////////////////////////////////////////////////
// GetProtocolVersion
/////////////////////////////////////////////////
void ZW_InitializeManager::GetProtocolVersion()
{
	ZW_APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION);
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> GetProtocolVersion: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION));
	enqueue(frame);
}

void ZW_InitializeManager::DecodeProtocolVersion(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id):
	// [0] Protocol Type
	// [1] Protocol Major Version
	// [2] Protocol Minor Version
	// [3] Protocol Revision Version
	// [4..5] Application Framework Build Number (MSB, LSB)
	// [6..21] Git commit hash (16 bytes) (optional)
	if (payload.size() < 4)
		return;

	module.ProtocolGitCommitHash.clear();

	module.ProtocolType = payload[0];
	module.ProtocolMajorVersion = payload[1];
	module.ProtocolMinorVersion = payload[2];
	module.ProtocolRevisionVersion = payload[3];

	if (payload.size() >= 6)
		module.AppFrameworkBuildNumber = (static_cast<uint16_t>(payload[4]) << 8) | payload[5];
	else
		module.AppFrameworkBuildNumber = 0;

	if (payload.size() >= 22)
		module.ProtocolGitCommitHash.assign(payload.begin() + 6, payload.begin() + 22);
	else if (payload.size() > 6)
		module.ProtocolGitCommitHash.assign(payload.begin() + 6, payload.end());

	Log.AddL(eLogTypes::DVC,
			 MakeTag(),
			 "<< GetProtocolVersion: type=0x{:02X} ver={}.{}.{} build={} hashLen={} (payloadLen={})",
			 module.ProtocolType,
			 module.ProtocolMajorVersion,
			 module.ProtocolMinorVersion,
			 module.ProtocolRevisionVersion,
			 module.AppFrameworkBuildNumber,
			 module.ProtocolGitCommitHash.size(),
			 payload.size());

	NotifyUI(UINotify::ControllerChanged);
}

//////////////////////////////////////////////////
// GetCapabilities
//////////////////////////////////////////////////
void ZW_InitializeManager::GetCapabilities()
{
	ZW_APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_CAPABILITIES);
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> GetCapabilities: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_CAPABILITIES));
	enqueue(frame);
}

void ZW_InitializeManager::DecodeCapabilities(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id):
	// [0] API version
	// [1] API revision
	// [2..3] Manufacturer ID (MSB, LSB)
	// [4..5] Product Type (MSB, LSB)
	// [6..7] Product ID (MSB, LSB)
	// [8..] Supported API command bitmask (N bytes)
	if (payload.size() < 8)
		return;

	module.AppVersion = payload[0];
	module.AppRevision = payload[1];
	module.ManufacturerId = (static_cast<uint16_t>(payload[2]) << 8) | payload[3];
	module.ProductType = (static_cast<uint16_t>(payload[4]) << 8) | payload[5];
	module.ProductId = (static_cast<uint16_t>(payload[6]) << 8) | payload[7];

	int APICListOffset = 8, APICListLength = ((int)payload.size() - APICListOffset);
	// extract the APICmdList, from an array of bits (byte,bit) 0,0 = 1  0,1 = 2  1,0 = 9
	module.ApiCommands.clear();
	std::ostringstream oss;
	for (int i = 0; i < APICListLength; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			bool APICExists = ((payload[APICListOffset + i] >> j) & 0x01) > 0;
			if (APICExists)
			{
				int ApiCmd = (i * 8 + j) + 1;
				module.ApiCommands.push_back(ApiCmd);
				oss << ApiCmd << ' ';
			}
		}
	}

	Log.AddL(eLogTypes::DVC, MakeTag(), "<< GetCapabilities: appVer={}.{} mfg=0x{:04X} prodType=0x{:04X} prodId=0x{:04X} cmdCount={} cmdList=[{}] (payloadLen={})",
			 module.AppVersion,
			 module.AppRevision,
			 module.ManufacturerId,
			 module.ProductType,
			 module.ProductId,
			 module.ApiCommands.size(),
			 oss.str(),
			 payload.size());

	NotifyUI(UINotify::ControllerChanged);
}

/////////////////////////////////////////////////
// GetNetworkIdsFromMemory
/////////////////////////////////////////////////
void ZW_InitializeManager::GetNetworkIdsFromMemory()
{
	ZW_APIFrame frame;
	frame.Make(eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY);
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> GetNetworkIdsFromMemory: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY));
	enqueue(frame);
}

void ZW_InitializeManager::DecodeNetworkIdsFromMemory(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id):
	// [0..3] HomeID (4 bytes)
	// [4]    NodeID (8-bit) OR [4..5] NodeID (16-bit)
	if (payload.size() < 5)
		return;

	module.HomeId = (static_cast<uint32_t>(payload[0]) << 24) |
		(static_cast<uint32_t>(payload[1]) << 16) |
		(static_cast<uint32_t>(payload[2]) << 8) |
		(static_cast<uint32_t>(payload[3]));

	if (payload.size() >= 6)
		module.NodeId = (static_cast<uint16_t>(payload[4]) << 8) | payload[5];
	else
		module.NodeId = payload[4];

	Log.AddL(eLogTypes::DVC, MakeTag(), "<< GetNetworkIdsFromMemory: homeId=0x{:08X} nodeId={} (payloadLen={})", module.HomeId, module.NodeId, payload.size());

	NotifyUI(UINotify::ControllerChanged);
}

//////////////////////////////////////////////////
// GetLibraryVersion
//////////////////////////////////////////////////
void ZW_InitializeManager::GetLibraryVersion()
{
	ZW_APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_LIBRARY_VERSION);
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> GetLibraryVersion: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_LIBRARY_VERSION));
	enqueue(frame);
}

void ZW_InitializeManager::DecodeLibraryVersion(const APIFrame::PayLoad& payload)
{
	// Spec (4.3.11): 12-byte version string (may not be NUL-terminated, padded with NUL)
	// followed by 1 byte library type.
	module.LibraryVersion.clear();
	module.ProtocolMajor = 0;
	module.ProtocolMinor = 0;

	if (payload.size() < 13)
		return;

	module.LibraryVersion.assign(reinterpret_cast<const char*>(payload.data()), 12);
	while (!module.LibraryVersion.empty() && module.LibraryVersion.back() == '\0')
		module.LibraryVersion.pop_back();

	module.libraryType = payload[12];

	// Expected format: "Z-Wave x.y" where x/y are 1-2 digits.
	const std::string prefix = "Z-Wave ";
	size_t pos = module.LibraryVersion.find(prefix);
	if (pos != std::string::npos)
	{
		std::string num = module.LibraryVersion.substr(pos + prefix.size());
		size_t dot = num.find('.');
		if (dot != std::string::npos)
		{
			try
			{
				module.ProtocolMajor = std::stoi(num.substr(0, dot));
				module.ProtocolMinor = std::stoi(num.substr(dot + 1));
			}
			catch (...) {}
		}
	}

	Log.AddL(eLogTypes::DVC, MakeTag(), "<< GetLibraryVersion: verStr='{}' libType=0x{:02X} protoParsed={}.{} (payloadLen={})", module.LibraryVersion, module.libraryType, module.ProtocolMajor, module.ProtocolMinor, payload.size());

	NotifyUI(UINotify::ControllerChanged);
}

//////////////////////////////////////////////////
// GetLibraryType
//////////////////////////////////////////////////
void ZW_InitializeManager::GetLibraryType()
{
	ZW_APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_LIBRARY_TYPE);
	Log.AddL(eLogTypes::DVC, MakeTag(), ">> GetLibraryType: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_LIBRARY_TYPE));
	enqueue(frame);
}

void ZW_InitializeManager::DecodeLibraryType(const APIFrame::PayLoad& payload)
{
	if (payload.empty())
	{
		Log.AddL(eLogTypes::DVC, MakeTag(), "<< LibraryType (invalid payload)");
		return;
	}

	module.libraryType = payload[0];

	std::string typeStr;
	switch (module.libraryType)
	{
	case 0x01: typeStr = "Static Controller"; break;
	case 0x02: typeStr = "Bridge Controller"; break;
	case 0x03: typeStr = "Portable Controller"; break;
	case 0x07: typeStr = "Enhanced Slave"; break;
	case 0x08: typeStr = "Enhanced Controller"; break;
	default:
		typeStr = std::format("Unknown (0x{:02X})", module.libraryType);
		break;
	}

	Log.AddL(eLogTypes::DVC, MakeTag(), "<< GetLibraryType: type=0x{:02X} ({}) (payloadLen={})", module.libraryType, typeStr, payload.size());

	NotifyUI(UINotify::ControllerChanged);
}
