#include "Logging.h"
#include "Initialize.h"
#include "APICommands.h"
#include "ControllerInfo.h"
#include "Notify.h"

Initialize::Initialize(EnqueueFn enqueue, ControllerInfo& controllerinfo)
	: enqueue(std::move(enqueue)), controllerInfo(controllerinfo)
{
}

void Initialize::Start()
{
	controllerInfo.InitializationState = ControllerInfo::eInitializationState::NotInitialized;
	currentStep = 0;
	Continue();
}

bool Initialize::Done()
{
	return controllerInfo.InitializationState == ControllerInfo::eInitializationState::Initialized;
}

bool Initialize::HandleFrame(const APIFrame& frame)
{
	const size_t count = std::size(InitSequence);
	if (currentStep >= count)
		return false;

	const auto& expectedStep = InitSequence[currentStep];
	if (frame.APICmd.CmdId != expectedStep.CmdId)
	{
		// Late/duplicate/out-of-order frames shouldn't rewind initialization.
		Log.AddL(eLogTypes::ERR, MakeTag(), "Initialization out of order: {}", frame.Info());
		return true;
	}

	if (!expectedStep.DecodeAction)
		return false;

	(this->*expectedStep.DecodeAction)(frame.payload);
	controllerInfo.InitializationState = expectedStep.DoneState;
	currentStep++;
	Continue();
	return true;
}

bool Initialize::HandleFrameTimeout(const APIFrame& frame)
{
	Log.AddL(eLogTypes::ERR, MakeTag(), "Initialization timed out: {}", frame.Info());
	const size_t count = std::size(InitSequence);
	if (currentStep >= count || frame.APICmd.CmdId != InitSequence[currentStep].CmdId)
	{
		Log.AddL(eLogTypes::ITZ, MakeTag(), "<< Unhandled Initialization command: {}", frame.Info());
		return false;
	}

	controllerInfo.InitializationState = ControllerInfo::eInitializationState::Paused;
	return true;
}

void Initialize::Continue()
{
	const size_t count = std::size(InitSequence);
	while (currentStep < count)
	{
		const auto& step = InitSequence[currentStep];

		if (!controllerInfo.HasAPICommand(step.CmdId))
		{
			if (step.Mandatory)
				Log.AddL(eLogTypes::ERR, MakeTag(), "Initialization failed Mandatory: cmdId={}", ToString(step.CmdId));
			else
				Log.AddL(eLogTypes::ERR, MakeTag(), "Initialization not supported: cmdId={}", ToString(step.CmdId));
			currentStep++;
			continue;
		}

		controllerInfo.InitializationState = step.PendingState;
		(this->*step.Action)();
		return;
	}
	controllerInfo.InitializationState = ControllerInfo::eInitializationState::Initialized;
}

/////////////////////////////////////////////////
// GetInitData
/////////////////////////////////////////////////
void Initialize::GetInitData()
{
	APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_INIT_DATA);
	Log.AddL(eLogTypes::ITZ, MakeTag(), ">> GetInitData: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_INIT_DATA));
	enqueue(frame);
}

void Initialize::DecodeInitData(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id, which is in the frame function id):
	// [0] Z-Wave API Version
	// [1] Z-Wave API Capabilities
	// [2] Z-Wave Node List Length (N)
	// [3..3+N-1] Z-Wave Node List (bitmask)
	// [3+N] Chip Type
	// [3+N+1] Chip Version
#pragma pack(push, 1)
	struct PayloadStruct
	{
		uint8_t ApiVersion;
		uint8_t ApiCapabilities;
		uint8_t NodeListLength;
	};
#pragma pack(pop)

	if (payload.size() < sizeof(PayloadStruct))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "Initialization failed: invalid payload size");
		return;
	}

	const auto unpackBitmask = []<typename T>(const uint8_t * data, size_t len, std::vector<T>&out)
	{
		out.clear();
		for (size_t i = 0; i < len; ++i)
			for (size_t j = 0; j < ControllerInfo::BitsPerByte; ++j)
				if ((data[i] & (1u << j)) != 0)
					out.push_back(T{ static_cast<uint8_t>(i * ControllerInfo::BitsPerByte + j + 1) });
	};

	const auto* p = reinterpret_cast<const PayloadStruct*>(payload.data());
	controllerInfo.ApiVersion = p->ApiVersion;
	controllerInfo.ApiCapabilities = p->ApiCapabilities;

	controllerInfo.NodeListLength = p->NodeListLength;

	const size_t nodeListOffset = sizeof(PayloadStruct);
	if (payload.size() < nodeListOffset + controllerInfo.NodeListLength)
		return;

	// extract the node list, from an array of bits (byte,bit) 0,0 = 1  0,1 = 2  1,0 = 9
	unpackBitmask(payload.data() + nodeListOffset, controllerInfo.NodeListLength, controllerInfo.NodeIds);

#pragma pack(push, 1)
	struct ChipStruct
	{
		uint8_t ChipType;
		uint8_t ChipVersion;
	};
#pragma pack(pop)

	auto* pC = reinterpret_cast<const ChipStruct*>(payload.data() + nodeListOffset + controllerInfo.NodeListLength);
	const size_t chipOffset = nodeListOffset + controllerInfo.NodeListLength;
	if (payload.size() >= chipOffset + sizeof(ChipStruct))
	{
		controllerInfo.ChipType = (ControllerInfo::eChipTypes)pC->ChipType;
		controllerInfo.ChipVersion = pC->ChipVersion;
	}

	Log.AddL(eLogTypes::ITZ, MakeTag(), "<< GetInitData ApiVersion={} Caps={} NodeListLen={} ChipType={} ChipVer={} ",
			 controllerInfo.ApiVersion,
			 controllerInfo.ApiCapabilities,
			 controllerInfo.NodeListLength,
			 (uint8_t)controllerInfo.ChipType,
			 controllerInfo.ChipVersion);
	Log.AddL(eLogTypes::ITZ, MakeTag(), "<< GetInitData: parsed nodeIds={} (first={}, last={})", controllerInfo.NodeIds.size(), controllerInfo.NodeIds.empty() ? nodeid_t{} : controllerInfo.NodeIds.front(), controllerInfo.NodeIds.empty() ? nodeid_t{} : controllerInfo.NodeIds.back());
	NotifyUI(UINotify::ControllerChanged);

#ifndef NDEBUG
	{
		assert(controllerInfo.ApiVersion == 5);
		assert(controllerInfo.ApiCapabilities == 8);
		assert(controllerInfo.NodeListLength == 29);

		assert(controllerInfo.NodeIds.size() == 7);
		assert(controllerInfo.NodeIds.front().Value() == 1);
		assert(controllerInfo.NodeIds.back().Value() == 26);

		assert((uint8_t)controllerInfo.ChipType == 3);
		assert(controllerInfo.ChipVersion == 1);
	}
#endif

}

/////////////////////////////////////////////////
// GetControllerCapabilities	
/////////////////////////////////////////////////
void Initialize::GetControllerCapabilities()
{
	APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES);
	Log.AddL(eLogTypes::ITZ, MakeTag(), ">> GetControllerCapabilities: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_CONTROLLER_CAPABILITIES));
	enqueue(frame);
}

void Initialize::DecodeControllerCapabilities(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id):
	// [0] Controller capabilities bitmask
#pragma pack(push, 1)
	struct PayloadStruct
	{
		uint8_t ControllerCapabilities;
	};
#pragma pack(pop)

	if (payload.size() < sizeof(PayloadStruct))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "GetControllerCapabilities failed: invalid payload size");
		return;
	}

	const auto* p = reinterpret_cast<const PayloadStruct*>(payload.data());
	controllerInfo.ControllerCapabilities = p->ControllerCapabilities;

	Log.AddL(eLogTypes::ITZ,
			 MakeTag(),
			 "<< GetControllerCapabilities: caps=0x{:02X} secondary={} otherNet={} sisPresent={} sucEnabled={} noNodesIncluded={} (payloadLen={})",
			 controllerInfo.ControllerCapabilities,
			 controllerInfo.IsSecondaryController() ? "yes" : "no",
			 controllerInfo.IsOtherNetwork() ? "yes" : "no",
			 controllerInfo.IsSISPresent() ? "yes" : "no",
			 controllerInfo.IsSUCEnabled() ? "yes" : "no",
			 controllerInfo.IsNoNodesIncluded() ? "yes" : "no",
			 payload.size());

	NotifyUI(UINotify::ControllerChanged);

#ifndef NDEBUG
	{
		assert(controllerInfo.IsSecondaryController() == false);
		assert(controllerInfo.IsOtherNetwork() == false);
		assert(controllerInfo.IsSISPresent() == true);
		assert(controllerInfo.IsSUCEnabled() == true);
		assert(controllerInfo.IsNoNodesIncluded() == false);
	}
#endif

}

/////////////////////////////////////////////////
// GetProtocolVersion
/////////////////////////////////////////////////
void Initialize::GetProtocolVersion()
{
	APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION);
	Log.AddL(eLogTypes::ITZ, MakeTag(), ">> GetProtocolVersion: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_PROTOCOL_VERSION));
	enqueue(frame);
}

void Initialize::DecodeProtocolVersion(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id):
	// [0] Protocol Type
	// [1] Protocol Major Version
	// [2] Protocol Minor Version
	// [3] Protocol Revision Version
	// [4..5] Application Framework Build Number (MSB, LSB)
	// [6..21] Git commit hash (16 bytes) (optional)
#pragma pack(push, 1)
	struct PayloadStruct
	{
		uint8_t ProtocolType = 0xFF; // ff not found
		uint8_t ProtocolMajorVersion;
		uint8_t ProtocolMinorVersion;
		uint8_t ProtocolRevisionVersion;
		uint8_t AppFrameworkBuildNumberMSB;
		uint8_t AppFrameworkBuildNumberLSB;
	};
#pragma pack(pop)

	if (payload.size() < sizeof(PayloadStruct))
		return;

	controllerInfo.ProtocolGitCommitHash.clear();

	const auto* p = reinterpret_cast<const PayloadStruct*>(payload.data());
	controllerInfo.ProtocolType = p->ProtocolType;
	controllerInfo.ProtocolMajorVersion = p->ProtocolMajorVersion;
	controllerInfo.ProtocolMinorVersion = p->ProtocolMinorVersion;
	controllerInfo.ProtocolRevisionVersion = p->ProtocolRevisionVersion;
	controllerInfo.AppFrameworkBuildNumber = (static_cast<uint16_t>(p->AppFrameworkBuildNumberMSB) << 8) | p->AppFrameworkBuildNumberLSB;

	const size_t gitCommitOffset = sizeof(PayloadStruct);

	if (payload.size() >= gitCommitOffset + ControllerInfo::ProtocolGitCommitHashLength)
		controllerInfo.ProtocolGitCommitHash.assign(payload.begin() + gitCommitOffset, payload.begin() + gitCommitOffset + ControllerInfo::ProtocolGitCommitHashLength);
	else if (payload.size() > gitCommitOffset)
		controllerInfo.ProtocolGitCommitHash.assign(payload.begin() + gitCommitOffset, payload.end());

	Log.AddL(eLogTypes::ITZ,
			 MakeTag(),
			 "<< GetProtocolVersion: type=0x{:02X} ver={}.{}.{} build={} hashLen={} (payloadLen={})",
			 controllerInfo.ProtocolType,
			 controllerInfo.ProtocolMajorVersion,
			 controllerInfo.ProtocolMinorVersion,
			 controllerInfo.ProtocolRevisionVersion,
			 controllerInfo.AppFrameworkBuildNumber,
			 controllerInfo.ProtocolGitCommitHash.size(),
			 payload.size());

	NotifyUI(UINotify::ControllerChanged);
}

//////////////////////////////////////////////////
// GetCapabilities
//////////////////////////////////////////////////
void Initialize::GetCapabilities()
{
	APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_CAPABILITIES);
	Log.AddL(eLogTypes::ITZ, MakeTag(), ">> GetCapabilities: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_CAPABILITIES));
	enqueue(frame);
}

void Initialize::DecodeCapabilities(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id):
	// [0] API version
	// [1] API revision
	// [2..3] Manufacturer ID (MSB, LSB)
	// [4..5] Product Type (MSB, LSB)
	// [6..7] Product ID (MSB, LSB)
	// [8..] Supported API command bitmask (N bytes)
#pragma pack(push, 1)
	struct PayloadStruct
	{
		uint8_t AppVersion;
		uint8_t AppRevision;
		uint8_t ManufacturerIdMSB;
		uint8_t ManufacturerIdLSB;
		uint8_t ProductTypeMSB;
		uint8_t ProductTypeLSB;
		uint8_t ProductIdMSB;
		uint8_t ProductIdLSB;
	};
#pragma pack(pop)

	if (payload.size() < sizeof(PayloadStruct))
	{
		Log.AddL(eLogTypes::ITZ, MakeTag(), "<< GetCapabilities: failed (payloadLen={})", payload.size());
		return;
	}

	const auto* p = reinterpret_cast<const PayloadStruct*>(payload.data());
	controllerInfo.AppVersion = p->AppVersion;
	controllerInfo.AppRevision = p->AppRevision;
	controllerInfo.ManufacturerId = (static_cast<uint16_t>(p->ManufacturerIdMSB) << 8) | p->ManufacturerIdLSB;
	controllerInfo.ProductType = (static_cast<uint16_t>(p->ProductTypeMSB) << 8) | p->ProductTypeLSB;
	controllerInfo.ProductId = (static_cast<uint16_t>(p->ProductIdMSB) << 8) | p->ProductIdLSB;

	int APICListOffset = static_cast<int>(sizeof(PayloadStruct)), APICListLength = ((int)payload.size() - APICListOffset);
	// extract the APICmdList, from an array of bits (byte,bit) 0,0 = 1  0,1 = 2  1,0 = 9
	controllerInfo.ApiCommands.clear();
	std::ostringstream oss;
	for (int i = 0; i < APICListLength; i++)
	{
		for (int j = 0,b=1; j < static_cast<int>(ControllerInfo::BitsPerByte); j++,b<<=1)
		{
			bool APICExists = (payload[APICListOffset + i] & b) > 0;
			if (APICExists)
			{
				int ApiCmd = (i * 8 + j) + 1;
				controllerInfo.ApiCommands.push_back(static_cast<uint8_t>(ApiCmd));
				oss << ApiCmd << ' ';
			}
		}
	}

	Log.AddL(eLogTypes::ITZ, MakeTag(), "<< GetCapabilities: appVer={}.{} mfg=0x{:04X} prodType=0x{:04X} prodId=0x{:04X} cmdCount={} cmdList=[{}] (payloadLen={})",
			 controllerInfo.AppVersion,
			 controllerInfo.AppRevision,
			 controllerInfo.ManufacturerId,
			 controllerInfo.ProductType,
			 controllerInfo.ProductId,
			 controllerInfo.ApiCommands.size(),
			 oss.str(),
			 payload.size());

	NotifyUI(UINotify::ControllerChanged);

#ifndef NDEBUG
	{
		assert(controllerInfo.AppVersion == 3);
		assert(controllerInfo.AppRevision == 7);

		assert(controllerInfo.ManufacturerId == 0x0086);
		assert(controllerInfo.ProductType == 0x0002);
		assert(controllerInfo.ProductId == 0x0001);

		assert(controllerInfo.ApiCommands.size() == 47);
	}
#endif

}

/////////////////////////////////////////////////
// GetNetworkIdsFromMemory
/////////////////////////////////////////////////
void Initialize::GetNetworkIdsFromMemory()
{
	APIFrame frame;
	frame.Make(eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY);
	Log.AddL(eLogTypes::ITZ, MakeTag(), ">> GetNetworkIdsFromMemory: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::ZW_API_GET_NETWORK_IDS_FROM_MEMORY));
	enqueue(frame);
}

void Initialize::DecodeNetworkIdsFromMemory(const APIFrame::PayLoad& payload)
{
	// Response payload format (excluding command id):
	// [0..3] HomeID (4 bytes)
	// [4]    NodeID (8-bit) OR [4..5] NodeID (16-bit)
#pragma pack(push, 1)
	struct PayloadStruct
	{
		uint32_t HomeIdBE;   // Big-endian HomeID

		union
		{
			uint16_t NodeId16BE; // Big-endian 16-bit NodeID (newer firmwares)
			uint8_t  NodeId8BE;  // Big-endian 8-bit NodeID (older firmwares)
		};
	};
#pragma pack(pop)

	if (payload.size() < ControllerInfo::MinNetworkIdsPayloadSize)
	{
		Log.AddL(eLogTypes::ITZ, MakeTag(),
				 "<< GetNetworkIdsFromMemory: failed (payloadLen={})",
				 payload.size());
		return;
	}

	const auto* p = reinterpret_cast<const PayloadStruct*>(payload.data());

	const auto ByteSwap32 = [](uint32_t value)
		{
			return ((value & 0xFF000000u) >> 24) |
				((value & 0x00FF0000u) >> 8) |
				((value & 0x0000FF00u) << 8) |
				((value & 0x000000FFu) << 24);
		};

	const auto ByteSwap16 = [](uint16_t value)
		{
			return static_cast<uint16_t>(
				((value & 0xFF00u) >> 8) |
				((value & 0x00FFu) << 8)
				);
		};

	// Convert big-endian HomeID to host order
	controllerInfo.HomeId = ByteSwap32(p->HomeIdBE);

	// NodeID: 8-bit or 16-bit depending on payload length
	if (payload.size() >= sizeof(PayloadStruct))
	{
		// 16-bit NodeID (newer firmwares)
		controllerInfo.NodeId = ByteSwap16(p->NodeId16BE);
	}
	else
	{
		// 8-bit NodeID (older firmwares)
		controllerInfo.NodeId = p->NodeId8BE;
	}

	Log.AddL(eLogTypes::ITZ, MakeTag(), "<< GetNetworkIdsFromMemory: homeId=0x{:08X} nodeId={} (payloadLen={})", controllerInfo.HomeId, controllerInfo.NodeId, payload.size());

	NotifyUI(UINotify::ControllerChanged);

#ifndef NDEBUG
	{
		assert(controllerInfo.HomeId == 0x014CE451);
		assert(controllerInfo.NodeId == 1);
	}
#endif

}

//////////////////////////////////////////////////
// GetLibraryVersion
//////////////////////////////////////////////////
void Initialize::GetLibraryVersion()
{
	APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_LIBRARY_VERSION);
	Log.AddL(eLogTypes::ITZ, MakeTag(), ">> GetLibraryVersion: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_LIBRARY_VERSION));
	enqueue(frame);
}

void Initialize::DecodeLibraryVersion(const APIFrame::PayLoad& payload)
{
	// Spec (4.3.11): 12-byte version string (may not be NUL-terminated, padded with NUL)
	// followed by 1 byte library type.
	controllerInfo.LibraryVersion.clear();
	controllerInfo.ProtocolMajor = 0;
	controllerInfo.ProtocolMinor = 0;

#pragma pack(push, 1)
	struct PayloadStruct
	{
		char LibraryVersion[ControllerInfo::LibraryVersionTextLength];
		uint8_t libraryType;
	};
#pragma pack(pop)

	if (payload.size() < sizeof(PayloadStruct))
	{
		Log.AddL(eLogTypes::ITZ, MakeTag(), "<< GetLibraryVersion: invalid payload (payloadLen={})", payload.size());
		return;
	}

	const auto* p = reinterpret_cast<const PayloadStruct*>(payload.data());
	controllerInfo.LibraryVersion.assign(p->LibraryVersion, std::find(p->LibraryVersion, p->LibraryVersion + ControllerInfo::LibraryVersionTextLength, '\0'));
	controllerInfo.libraryType = static_cast<ControllerInfo::eLibraryType>(p->libraryType);

	// Expected format: "Z-Wave x.y" where x/y are 1-2 digits.
	const std::string prefix = "Z-Wave ";
	size_t pos = controllerInfo.LibraryVersion.find(prefix);
	if (pos != std::string::npos)
	{
		std::string num = controllerInfo.LibraryVersion.substr(pos + prefix.size());
		size_t dot = num.find('.');
		if (dot != std::string::npos)
		{
			try
			{
				controllerInfo.ProtocolMajor = std::stoi(num.substr(0, dot));
				controllerInfo.ProtocolMinor = std::stoi(num.substr(dot + 1));
			}
			catch (...) {}
		}
	}

	Log.AddL(eLogTypes::ITZ, MakeTag(), "<< GetLibraryVersion: verStr='{}' libType=0x{:02X} protoParsed={}.{} (payloadLen={})", controllerInfo.LibraryVersion, static_cast<uint8_t>(controllerInfo.libraryType), controllerInfo.ProtocolMajor, controllerInfo.ProtocolMinor, payload.size());

	NotifyUI(UINotify::ControllerChanged);

#ifndef NDEBUG
	{
		assert(controllerInfo.LibraryVersion == "Z-Wave 2.78");
		assert((uint8_t)controllerInfo.libraryType == 0x01);
		assert(controllerInfo.ProtocolMajor == 2);
		assert(controllerInfo.ProtocolMinor == 78);
	}
#endif

}

//////////////////////////////////////////////////
// GetLibraryType
//////////////////////////////////////////////////
void Initialize::GetLibraryType()
{
	APIFrame frame;
	frame.Make(eCommandIds::FUNC_ID_GET_LIBRARY_TYPE);
	Log.AddL(eLogTypes::ITZ, MakeTag(), ">> GetLibraryType: cmdId=0x{:02X}", static_cast<uint8_t>(eCommandIds::FUNC_ID_GET_LIBRARY_TYPE));
	enqueue(frame);
}

void Initialize::DecodeLibraryType(const APIFrame::PayLoad& payload)
{
#pragma pack(push, 1)
	struct PayloadStruct
	{
		uint8_t libraryType;
	};
#pragma pack(pop)

	if (payload.size() < sizeof(PayloadStruct))
	{
		Log.AddL(eLogTypes::ITZ, MakeTag(), "<< GetLibraryType: failed (payloadLen={})", payload.size());
		return;
	}

	const auto* p = reinterpret_cast<const PayloadStruct*>(payload.data());
	controllerInfo.libraryType = static_cast<ControllerInfo::eLibraryType>(p->libraryType);

	std::string typeStr;
	switch (controllerInfo.libraryType)
	{
	case ControllerInfo::eLibraryType::StaticController: typeStr = "Static Controller"; break;
	case ControllerInfo::eLibraryType::BridgeController: typeStr = "Bridge Controller"; break;
	case ControllerInfo::eLibraryType::PortableController: typeStr = "Portable Controller"; break;
	case ControllerInfo::eLibraryType::EnhancedSlave: typeStr = "Enhanced Slave"; break;
	case ControllerInfo::eLibraryType::EnhancedController: typeStr = "Enhanced Controller"; break;
	default:
		typeStr = std::format("Unknown (0x{:02X})", static_cast<uint8_t>(controllerInfo.libraryType));
		break;
	}

	Log.AddL(eLogTypes::ITZ, MakeTag(), "<< GetLibraryType: type=0x{:02X} ({}) (payloadLen={})", static_cast<uint8_t>(controllerInfo.libraryType), typeStr, payload.size());

	NotifyUI(UINotify::ControllerChanged);
}
