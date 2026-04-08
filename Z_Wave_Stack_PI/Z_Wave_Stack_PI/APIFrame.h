#pragma once

#include <vector>
#include <string>
#include <cassert>
#include <functional>
#include <cstdint>
#include <string_view>

#include "APICommands.h"
#include "NodeId_t.h"

// Prefer `std::uint8_t` explicitly in this header to avoid any reliance on
// global typedefs or toolchain-specific behavior.

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

class APIFrame
{
public:
	enum class eFrameTypes : std::uint8_t
	{
		REQ = 0x00,
		RES = 0x01
	};
	eFrameTypes Type() const { return type; } // Get frame type
private:

	// Calculate checksum
	inline std::uint8_t CheckSum(const std::vector<std::uint8_t>& frame) const
	{
		std::uint8_t checksum = 0xFF;
		for (size_t i = 1; i < frame.size(); ++i)
			checksum ^= frame[i];
		return checksum;
	}

	eFrameTypes type = eFrameTypes::REQ;

public:
	APICommand APICmd = APICommands[0];  // Default; will be overwritten for decoded frames
	class PayLoad : private std::vector<std::uint8_t>
	{
	public:
		using base = std::vector<std::uint8_t>;
		using container_type = base;
		using value_type = base::value_type;
		using iterator = base::iterator;
		using const_iterator = base::const_iterator;

		using base::empty;
		using base::size;
		using base::clear;
		using base::push_back;
		using base::operator[];
		using base::begin;
		using base::end;
		using base::assign;
		using base::insert;
		using base::data;
		using base::back;

		// Hex dump of payload
		std::string ToString() const
		{
			return ToString(*this);
		}
		static std::string ToString(const std::vector<std::uint8_t>& data)
		{
			if (data.empty()) return std::string("(0)");
			std::string out = "(" + std::to_string(data.size()) + ")";
			out.reserve(data.size() * 3);
			for (auto c : data)
				out += " " + FormatCompat::HexValue(c);
			return out;
		};

	} payload = {};

	std::vector<std::uint8_t> Encode_Frame() const
	{
		// frame = [0] SOF (0x01) + [1] LEN + [2] TYPE + [3] FUNC_ID + [4..] PARAMS + [last] CHECKSUM
		std::vector<std::uint8_t> frame;
		frame.push_back(0x01); // SOF
		// LEN counts: TYPE + FUNC_ID + PARAMS + CHECKSUM
		std::uint8_t length = static_cast<std::uint8_t>(3 + payload.size());
		frame.push_back(length);
		frame.push_back(static_cast<std::uint8_t>(type)); // TYPE
		frame.push_back(static_cast<std::uint8_t>(APICmd.CmdId)); // 

		for (const auto& param : payload)
			frame.push_back(param); // PARAMS

		// Calculate checksum
		frame.push_back(CheckSum(frame));
		return frame;
	}

	const int Decode_Frame(const std::vector<std::uint8_t>& frame)
	{
		// frame = [0] SOF (0x01) + [1] LEN + [2] TYPE + [3] FUNC_ID + [4..] PARAMS + [last] CHECKSUM
		if (frame.size() < 5)
			return -1;

		if (frame[0] != 0x01)
			return -2;

		std::uint8_t length = frame[1];

		// Total frame size must be SOF + LEN + length bytes
		if (frame.size() != (size_t)(length + 2))
			return -3;

		// Valid frame if XOR of bytes [1..last] == 0
		if (CheckSum(frame) != 0x00)
			return -4;

		type = static_cast<eFrameTypes>(frame[2]);
		APICmd = APICommands[frame[3]];
		assert(static_cast<std::uint8_t>(APICmd.CmdId) != 0);
		// Payload is everything except SOF, LEN, TYPE, FUNC_ID, CHECKSUM
		payload.assign(frame.begin() + 4, frame.end() - 1);
		return 1;
	}

public:
	std::string Info() const
	{
#ifndef NDEBUG
		std::string typeStr =
			(type == eFrameTypes::REQ) ? "REQ" :
			((type == eFrameTypes::RES) ? "RES" :
			 std::to_string((int)type));

		std::string name =
			APICmd.Name.empty() ? "UNKNOWN" : APICmd.Name;

		std::string extra;

		// ----------------------------------------------------
		// 0x49 ZW_API_APPLICATION_UPDATE
		// ----------------------------------------------------
		if (APICmd.CmdId == eCommandIds::ZW_API_APPLICATION_UPDATE)
		{
			if (payload.size() > 2)
			{
				auto ev = static_cast<ApplicationUpdateEvent>(payload[0]);
				nodeid_t nodeid{ payload[1] };
				extra = " | event=" + ToString(ev);
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
			if (type == eFrameTypes::REQ && payload.size() >= 4)
			{
				nodeid_t nodeId{ payload[0] };
				size_t rfLen = payload[1];
				uint8_t callbackId = payload[payload.size() - 2];
				uint8_t txOptions = payload[payload.size() - 1];

				// Extract RF payload
				std::vector<uint8_t> rf;
				if (payload.size() >= 2 + rfLen)
					rf.assign(payload.begin() + 2, payload.begin() + 2 + rfLen);

				extra = " | node=0x" + FormatCompat::HexValue(nodeId.Value()) +
					" rf=[" + PayLoad::ToString(rf) + "] cb=0x" +
					FormatCompat::HexValue(callbackId) + " txOpt=0x" +
					FormatCompat::HexValue(txOptions);
			}
			else if (type == eFrameTypes::REQ && payload.size() >= 2)
			{
				uint8_t sesionsid = payload[0];
				uint8_t txStatus = payload[1]; // TODO: decode status
				extra = " | sessionid=0x" + FormatCompat::HexValue(sesionsid) +
					" tx status 0x" + FormatCompat::HexValue(txStatus);
			}
			else if (type == eFrameTypes::RES && payload.size() == 1)
			{
				uint8_t rspStatus = payload[payload.size() - 1];
				extra = " | rsp status 0x" + FormatCompat::HexValue(rspStatus);
			}
			else
			{
				extra = " | malformed SEND_DATA payload";
			}
		}

		return std::format(
			"Type = {} cmdId = 0x{:02X} {} | payload = {} {}",
			typeStr,
			(int)APICmd.CmdId,
			name,
			payload.ToString(),
			extra
		);
#else
    return std::format("0x{:02X}", static_cast<std::uint8_t>(APICmd.CmdId));
#endif
	}

	std::string DVC() const
	{
		return Info();
	}

	eFlowType Flow() const
	{
		return APICmd.Flow;
	}

	void Make(const eCommandIds cmd, const std::vector<std::uint8_t>& params = {})
	{
		APICmd = APICommands[static_cast<std::uint8_t>(cmd)];
		assert(APICmd.CmdId == cmd);
		type = eFrameTypes::REQ;
		payload.assign(params.begin(), params.end());
	}

	// Make a request for a specific node (base type 8-bit)
	void Make(const eCommandIds cmd, const nodeid_t nodeId, const std::vector<std::uint8_t>& params = {})
	{
		APICmd = APICommands[static_cast<std::uint8_t>(cmd)];
		assert(APICmd.CmdId == cmd);
		type = eFrameTypes::REQ;
		payload.clear();
		payload.push_back(nodeId.Value());
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
	void MakeSendData(const nodeid_t nodeId, const std::uint8_t callbackId, const std::vector<std::uint8_t>& params = {})
	{
		APICmd = APICommands[static_cast<std::uint8_t>(eCommandIds::ZW_API_CONTROLLER_SEND_DATA)];
		assert(APICmd.CmdId == eCommandIds::ZW_API_CONTROLLER_SEND_DATA);
		type = eFrameTypes::REQ;
		payload.clear();
		payload.push_back(nodeId.Value()); // NodeID
		payload.push_back(static_cast<std::uint8_t>(params.size())); // Payload length 
		payload.insert(payload.end(), params.begin(), params.end()); // RF payload 
		payload.push_back(static_cast<std::uint8_t>(callbackId));
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

using EnqueueFn = std::function<void(const APIFrame&)>;
