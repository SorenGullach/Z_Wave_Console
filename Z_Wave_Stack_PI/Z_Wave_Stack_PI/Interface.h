#pragma once

#include <mutex>
#include <queue>
#include <vector>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <cstdint>

#include "SerialPort.h"
#include "APIFrame.h"

// ===============================================================
// Transport + send/receive state machine for Z-Wave Serial API
//
// Responsibilities:
//  - Owns the serial port and a worker thread.
//  - Enqueues outgoing `ZW_APIFrame` commands.
//  - Handles ACK/NAK/CAN, timeouts, retries (backoff), and response waiting.
//  - Decodes incoming frames and forwards them to `OnFrameReceived()`.
//
// NOTE: Derived classes implement `OnFrameReceived()` to route responses.
// ===============================================================

class Interface
{
public:
	Interface();
	~Interface() { ClosePort(); }

	bool OpenPort(const std::string& portname);
	void ClosePort();
	bool IsSerialOpen() const;

	// Enqueue a command to be sent
	void Enqueue(const APIFrame& frame);
	
	void Test();

protected:

	virtual bool OnFrameReceived(const APIFrame& frame);
	virtual bool OnFrameReceivedTimeout(const APIFrame& frame);

private:
	SerialPort serial;
	mutable std::mutex serialMutex;

	std::thread worker;
	std::atomic<bool> running{ false };

	void Start();
	void Stop();

	enum class SendState
	{
		Idle,
		WaitingForAck,
		WaitingForResponse,
		WaitingForCallback
	};

	struct CommandFrame
	{
		APIFrame cmd;
		inline eFlowType Flow() { return cmd.APICmd.Flow; };
		int attempts = 0; // attemts per cmd
		std::vector<uint8_t> FrameBytes;
	};

	SendState state = SendState::Idle;

	std::chrono::steady_clock::time_point stateDeadline{};
	std::chrono::steady_clock::time_point nextSendTime{};
	const std::chrono::milliseconds coolDown{ 100 };

	std::mutex queueMutex;
	std::queue<CommandFrame> cmdQueue;

	const int maxRetries = 3;
	CommandFrame LastCmd;
	int consecutiveChecksumErrors = 0;

	bool TryDequeueNext(CommandFrame& out);

	// Back-off according to Tn = 100 + n * 1000 ms (Section 3.4.1)
	std::chrono::milliseconds BackoffDelay(int attempt) const;

	void SendCommand(CommandFrame& cmd);

	int SerialRead(uint8_t* buffer, int size);
	int SerialWrite(const uint8_t* buffer, int size);
	int SerialWrite(const std::vector<uint8_t>& buffer);

	const std::string ToString(const std::vector<uint8_t>& buffer) const;

	enum class AckTypes : uint8_t
	{
		ACK = 0x06,
		NAK = 0x15,
		CAN = 0x18,
		SOF = 0x01
	};

	void HandleTimeouts();
	void HandleAck();
	void HandleNakOrCan(bool isNak);

	bool ReadFrameWithTimeout(std::vector<uint8_t>& buffer);
	void ProcessValidFrame(const std::vector<uint8_t>& buffer);

	void ProcessSerialCommunication();
};
