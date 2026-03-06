#pragma once

#include <vector>
#include <string>
#include <cassert>
#include <functional>
#include <format>

#include "APICommands.h"
#include <cstdint>
#include <string_view>

// ===============================================================
// Serial API frame encoder/decoder
//
// A frame on the wire is:
//   [0]  SOF (0x01)
//   [1]  LEN (bytes from TYPE through CHECKSUM, inclusive)
//   [2]  TYPE (REQ/RES)
//   [3]  FUNC_ID (Z-Wave API command id)
//   [4..]PARAMS (payload)
//   [N]  CHECKSUM (0xFF XOR bytes [1..N-1])
//
// `ZW_APIFrame` contains:
//  - `APICmd` (command id + expected flow)
//  - `type`  (request/response)
//  - `payload` (params excluding FUNC_ID)
// ===============================================================

struct ZW_APIFrame
{
	enum class FrameTypes : uint8_t
	{
		REQ = 0x00,
		RES = 0x01
	};

	APICommand APICmd = APICommands[0];  // Default; will be overwritten for decoded frames
	FrameTypes type = FrameTypes::REQ;
	std::vector<uint8_t> payload = {}; // frame = [0] SOF (0x01) + [1] LEN + [2] TYPE + [3] FUNC_ID + [4..] PARAMS + [last] CHECKSUM

	// [0] SOF (0x01) 
	// [1] LEN  = number of bytes from [2] through checksum (inclusive)
	//           i.e. TYPE + FUNC_ID + PARAMS + CHECKSUM
	// [2] TYPE = REQ (0x00) / RES (0x01) / ...
	// [3] FUNC_ID
	// [4..] PARAMS (optional)
	// [last] CHECKSUM = 0xFF XOR all bytes from LEN ([1]) through last payload byte
	//                   so that XOR of all bytes [1..last] == 0x00

	std::vector<uint8_t> Encode_Frame() const
	{
		std::vector<uint8_t> frame;
		frame.push_back(0x01); // SOF
		// LEN counts: TYPE + FUNC_ID + PARAMS + CHECKSUM
		uint8_t length = static_cast<uint8_t>(3 + payload.size());
		frame.push_back(length);
		frame.push_back(static_cast<uint8_t>(type)); // TYPE
		frame.push_back(static_cast<uint8_t>(APICmd.CmdId)); // 
		for (const auto& param : payload)
			frame.push_back(param); // PARAMS
		// Calculate checksum
		uint8_t checksum = 0xFF;
		for (size_t i = 1; i < frame.size(); ++i)
			checksum ^= frame[i];
		frame.push_back(checksum);
		return frame;
	}

	const int Decode_Frame(const std::vector<uint8_t>& frame)
	{
		if (frame.size() < 5)
			return -1;

		if (frame[0] != 0x01)
			return -2;

		uint8_t length = frame[1];

		// Total frame size must be SOF + LEN + length bytes
		if (frame.size() != (size_t)(length + 2))
			return -3;

		// --- Checksum verification ---
		uint8_t chk = 0xFF;
		for (size_t i = 1; i < frame.size(); ++i)
			chk ^= frame[i];

		// Valid frame if XOR of bytes [1..last] == 0
		if (chk != 0x00)
			return -4;

		type = static_cast<FrameTypes>(frame[2]);
		APICmd = APICommands[frame[3]];
		assert(static_cast<uint8_t>(APICmd.CmdId) != 0);
		// Payload is everything except SOF, LEN, TYPE, FUNC_ID, CHECKSUM
		payload.assign(frame.begin() + 4, frame.end() - 1);

		if ((uint8_t)APICmd.CmdId == 0x00 && payload.size() >= 3)
		{
			uint8_t possibleCC = payload[2];
			if (possibleCC >= 0x20 && possibleCC <= 0x9F)
			{
				// Looks like a CC frame
				APICmd = APICommands[static_cast<uint8_t>(eCommandIds::ZW_API_APPLICATION_COMMAND_HANDLER)];
			}
		}

		return 1;
	}

	std::string Info() const
	{
#ifdef _DEBUG
		const char* typeStr =
			(type == FrameTypes::REQ) ? "REQ" :
			(type == FrameTypes::RES) ? "RES" : "??";

		std::string name =
			APICmd.Name.empty() ? "UNKNOWN" : APICmd.Name;

		// Hex dump of payload
		auto hexDump = [&](const std::vector<uint8_t>& data)
			{
				if (data.empty()) return std::string("—");
				std::string out;
				out.reserve(data.size() * 3);
				for (size_t i = 0; i < data.size(); ++i)
				{
					out += std::format("{:02X}", data[i]);
					if (i + 1 < data.size()) out += ' ';
				}
				return out;
			};

		std::string extra;

		// ----------------------------------------------------
		// 0x49 ZW_API_APPLICATION_UPDATE
		// ----------------------------------------------------
		if (APICmd.CmdId == eCommandIds::ZW_API_APPLICATION_UPDATE)
		{
			if (!payload.empty())
			{
				auto ev = static_cast<ApplicationUpdateEvent>(payload[0]);
				uint8_t nodeId = payload[1];
				extra = std::format(" | event={} ({})",
									ToString(ev), hexDump(payload) );
			}
			else
			{
				extra = " | event=<missing>";
			}
		}

		// ----------------------------------------------------
		// 0x13 ZW_API_CONTROLLER_SEND_DATA
		// ----------------------------------------------------
		else if (APICmd.CmdId == eCommandIds::ZW_API_CONTROLLER_SEND_DATA)
		{
			if (payload.size() >= 4)
			{
				uint8_t nodeId = payload[0];
				uint8_t rfLen = payload[1];
				uint8_t callbackId = payload[payload.size() - 2];
				uint8_t txOptions = payload[payload.size() - 1];

				// Extract RF payload
				std::vector<uint8_t> rf;
				if (payload.size() >= 2 + rfLen)
					rf.assign(payload.begin() + 2, payload.begin() + 2 + rfLen);

				extra = std::format(
					" | node=0x{:02X} rf=[{}] cb=0x{:02X} txOpt=0x{:02X}",
					nodeId,
					hexDump(rf),
					callbackId,
					txOptions
				);
			}
			else
			{
				extra = " | malformed SEND_DATA payload";
			}
		}

		return std::format(
			"[{}] cmdId=0x{:02X} {} | payload={}{}",
			typeStr,
			(int)APICmd.CmdId,
			name,
			hexDump(payload),
			extra
		);
#endif

		return std::format("0x{:02X}", static_cast<uint8_t>(APICmd.CmdId));
	}

	void Make(const eCommandIds cmd, const std::vector<uint8_t>& params = {})
	{
		APICmd = APICommands[static_cast<uint8_t>(cmd)];
		assert(APICmd.CmdId == cmd);
		type = FrameTypes::REQ;
		payload = params;
	}

	// Make a request for a specific node (base type 8-bit)
	void Make(const eCommandIds cmd, const uint8_t nodeId, const std::vector<uint8_t>& params = {})
	{
		APICmd = APICommands[static_cast<uint8_t>(cmd)];
		assert(APICmd.CmdId == cmd);
		type = FrameTypes::REQ;
		payload.clear();
		payload.push_back(nodeId);
		payload.insert(payload.end(), params.begin(), params.end());
	}
	/*
	// Make a request for a specific node (base type 16-bit)
	void Make(const eCommandIds cmd, const uint16_t nodeId, const std::vector<uint8_t>& params = {})
	{
		APICmd = APICommands[static_cast<uint8_t>(cmd)];
		assert(APICmd.CmdId == cmd);
		type = FrameTypes::REQ;
		payload.clear();
		payload.push_back(static_cast<uint8_t>(nodeId)); 
		payload.push_back(static_cast<uint8_t>(nodeId>>8));
		payload.insert(payload.end(), params.begin(), params.end());
	}
	*/
	// Make a request for a specific node (base type 8-bit)
	void MakeSendData(const uint8_t nodeId, uint8_t callbackId, const std::vector<uint8_t>& params = {})
	{
		APICmd = APICommands[static_cast<uint8_t>(eCommandIds::ZW_API_CONTROLLER_SEND_DATA)];
		assert(APICmd.CmdId == eCommandIds::ZW_API_CONTROLLER_SEND_DATA);
		type = FrameTypes::REQ;
		payload.clear();
		payload.push_back(nodeId); // NodeID 
		payload.push_back(static_cast<uint8_t>(params.size())); // Payload length 
		payload.insert(payload.end(), params.begin(), params.end()); // RF payload 
		payload.push_back(static_cast<uint8_t>(callbackId));
		payload.push_back(0x05); // Tx options	
	}
	/*
	// Make a request for a specific node (base type 16-bit)
	void MakeSendData(const uint16_t nodeId, uint8_t callbackId, const std::vector<uint8_t>& params = {})
	{
		APICmd = APICommands[static_cast<uint8_t>(eCommandIds::ZW_API_CONTROLLER_SEND_DATA)];
		assert(APICmd.CmdId == eCommandIds::ZW_API_CONTROLLER_SEND_DATA);
		type = FrameTypes::REQ;
		payload.clear();
		payload.push_back(static_cast<uint8_t>(nodeId));
		payload.push_back(static_cast<uint8_t>(nodeId >> 8));
		payload.push_back(static_cast<uint8_t>(params.size())); // Payload length 
		payload.insert(payload.end(), params.begin(), params.end()); // RF payload 
		payload.push_back(static_cast<uint8_t>(callbackId));
		payload.push_back(0x05); // Tx options	
	}
	*/
};

using EnqueueFn = std::function<void(const ZW_APIFrame&)>;
