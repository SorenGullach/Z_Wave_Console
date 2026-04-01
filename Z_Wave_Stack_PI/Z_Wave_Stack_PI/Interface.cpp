
#include "Logging.h"
#include "Interface.h"

Interface::Interface() {}

bool Interface::OpenPort(const std::string& portname)
{
	std::scoped_lock lock(serialMutex);
	if (!serial.Open(portname))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "OpenPort failed: port='{}'", portname);
		return false;
	}

	Log.AddL(eLogTypes::ITF, MakeTag(), "OpenPort ok: port='{}'", portname);
	Start();
	return true;
}

void Interface::ClosePort()
{
	Stop();
	std::scoped_lock lock(serialMutex);
	serial.Close();
	Log.AddL(eLogTypes::ITF, MakeTag(), "ClosePort: port closed");
}

bool Interface::IsSerialOpen() const
{
	std::scoped_lock lock(serialMutex);
	return serial.IsOpen();
}

void Interface::Enqueue(const APIFrame& frame)
{
	std::scoped_lock lock(queueMutex);
	CommandFrame cmd;
	cmd.cmd = frame;
	cmd.attempts = 0;
	cmd.FrameBytes.clear();
	cmdQueue.push(cmd);
}

bool Interface::OnFrameReceived(const APIFrame& frame)
{
	return false; // not handled
}
bool Interface::OnFrameReceivedTimeout(const APIFrame& frame)
{
	return false; // not handled
}


void Interface::Start()
{
	bool expected = false;
	if (!running.compare_exchange_strong(expected, true))
		return;

	worker = std::thread([this]()
						 {
							 try
							 {
								 while (running.load())
								 {
									 if (!IsSerialOpen())
										 break;
									 ProcessSerialCommunication();
								 }
							 }
							 catch (const std::exception& ex)
							 {
								 Log.AddL(eLogTypes::ERR, MakeTag(), "Unhandled exception in worker thread: {}", ex.what());
							 }
							 catch (...)
							 {
								 Log.AddL(eLogTypes::ERR, MakeTag(), "Unknown exception in worker thread.");
							 }
						 });
}

void Interface::Stop()
{
	running.store(false);
	if (worker.joinable())
		worker.join();
}

bool Interface::TryDequeueNext(CommandFrame& out)
{
	std::scoped_lock lock(queueMutex);
	if (cmdQueue.empty())
		return false;
	out = cmdQueue.front();
	cmdQueue.pop();
	return true;
}

std::chrono::milliseconds Interface::BackoffDelay(int attempt) const
{
	int n = attempt; // 0-based
	return std::chrono::milliseconds(100 + n * 1000);
}

void Interface::SendCommand(CommandFrame& cmd)
{
	cmd.FrameBytes = cmd.cmd.Encode_Frame();
	LastCmd = cmd;
	LastCmd.attempts++;

	Log.AddL(eLogTypes::DBG, MakeTag(), ">> Send: attempt={} frame={}", LastCmd.attempts, LastCmd.cmd.Info());
	Log.AddL(eLogTypes::DBG, MakeTag(), ">> FrameBytes: len={} bytes=[{}]",
			 LastCmd.FrameBytes.size(),
			 ToString(LastCmd.FrameBytes));

	SerialWrite(LastCmd.FrameBytes);

	if (LastCmd.Flow() == eFlowType::Unacknowledged)
	{
		state = SendState::Idle;
		nextSendTime = std::chrono::steady_clock::now() + coolDown;
	}
	else
	{
		state = SendState::WaitingForAck;
		stateDeadline = std::chrono::steady_clock::now() + BackoffDelay(LastCmd.attempts - 1);
	}
}

int Interface::SerialRead(uint8_t* buffer, int size)
{
	std::scoped_lock lock(serialMutex);
	return serial.Read(buffer, size);
}

int Interface::SerialWrite(const uint8_t* buffer, int size)
{
	std::scoped_lock lock(serialMutex);
	return serial.Write(buffer, size);
}

int Interface::SerialWrite(const std::vector<uint8_t>& buffer)
{
	std::scoped_lock lock(serialMutex);
	return serial.Write(buffer);
}

const std::string Interface::ToString(const std::vector<uint8_t>& buffer) const
{
	std::string result;
	for (size_t i = 0; i < buffer.size(); i++)
	{
		char hex[4] = { 0 };
		std::snprintf(hex, sizeof(hex), "%02X ", buffer[i]);
		result += hex;
	}
	return result;
}

void Interface::HandleTimeouts()
{
	auto now = std::chrono::steady_clock::now();
	if (state == SendState::Idle)
		return;

	if (now <= stateDeadline)
		return;

	switch (state)
	{
	case SendState::WaitingForAck:
		if (LastCmd.Flow() == eFlowType::Unacknowledged)
		{
			state = SendState::Idle;
			break;
		}

		if (LastCmd.attempts < maxRetries)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "ACK timeout: -> retransmit attempt={}/{} frame={}",
					 LastCmd.attempts, maxRetries, LastCmd.cmd.Info());
			SendCommand(LastCmd);
		}
		else
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "ACK timeout: -> give up attempt={}/{} frame={}",
					 LastCmd.attempts, maxRetries, LastCmd.cmd.Info());
			state = SendState::Idle;
			nextSendTime = now + coolDown;
		}
		break;

	case SendState::WaitingForResponse:
	case SendState::WaitingForCallback:
		//logger.AddLogLock(MakeTag(), "Response/Callback timeout");
		if (!OnFrameReceivedTimeout(LastCmd.cmd))
			Log.AddL(eLogTypes::ERR, MakeTag(), "Frame timeout: state={} INF0={}",
					 static_cast<int>(state), LastCmd.cmd.Info());
		state = SendState::Idle;
		nextSendTime = now + coolDown;
		break;

	default:
		break;
	}
}

void Interface::HandleAck()
{
	//    logger.AddLogLock(MakeTag(), "<< ACK");
	LastCmd.attempts = 0;

	switch (LastCmd.Flow())
	{
	case eFlowType::AckOnly:
		state = SendState::Idle;
		nextSendTime = std::chrono::steady_clock::now() + coolDown;
		break;

	case eFlowType::AckWithResponse:
	case eFlowType::AckWithResponseCallback:
		state = SendState::WaitingForResponse;
		stateDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1600);
		break;

	case eFlowType::AckWithCallback:
		state = SendState::WaitingForCallback;
		stateDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1600);
		break;

	default:
		state = SendState::Idle;
		break;
	}
}

void Interface::HandleNakOrCan(bool isNak)
{
	Log.AddL(eLogTypes::ERR, MakeTag(), "<< {}: flow={} attempt={}/{} frame={}",
			 isNak ? "NAK" : "CAN",
			 static_cast<uint8_t>(LastCmd.Flow()),
			 LastCmd.attempts, maxRetries, LastCmd.cmd.Info());

	if (LastCmd.Flow() == eFlowType::Unacknowledged)
	{
		if (LastCmd.attempts < maxRetries)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Unacknowledged frame NAK/CAN -> retransmit with backoff");
			SendCommand(LastCmd);
		}
		else
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "Unacknowledged frame NAK/CAN -> max retries reached");
			state = SendState::Idle;
			nextSendTime = std::chrono::steady_clock::now() + coolDown;
		}
		return;
	}

	if (LastCmd.attempts < maxRetries)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "NAK/CAN: -> retransmit attempt={}/{} frame={}",
				 LastCmd.attempts, maxRetries, LastCmd.cmd.Info());
		SendCommand(LastCmd);
	}
	else
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "NAK/CAN: -> max retries reached, give up frame={}",
				 LastCmd.cmd.Info());
		state = SendState::Idle;
		nextSendTime = std::chrono::steady_clock::now() + coolDown;
	}
}

bool Interface::ReadFrameWithTimeout(std::vector<uint8_t>& buffer)
{
	auto start = std::chrono::steady_clock::now();

	uint8_t len = 0;
	if (SerialRead(&len, 1) != 1)
		return false;

	buffer.resize(static_cast<size_t>(2 + len));
	buffer[0] = static_cast<uint8_t>(AckTypes::SOF);
	buffer[1] = len;

	size_t remaining = len;
	uint8_t* dst = buffer.data() + 2;

	while (remaining > 0)
	{
		auto now = std::chrono::steady_clock::now();
		if (now - start > std::chrono::milliseconds(1500))
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "ReadFrameWithTimeout: timeout waiting for payload (>1500ms after SOF)");
			return false;
		}

		int n = SerialRead(dst, static_cast<int>(remaining));
		if (n <= 0)
			continue;

		dst += n;
		remaining -= static_cast<size_t>(n);
	}

	return true;
}

void Interface::ProcessValidFrame(const std::vector<uint8_t>& buffer)
{
	APIFrame frame;
	if (frame.Decode_Frame(buffer) != 1)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< Decode Invalid frame: {}", ToString(buffer));
		return;
	}

	Log.AddL(eLogTypes::DBG, MakeTag(), "<< Recv: bytesLen={} bytes=[{}]",
			 buffer.size(), ToString(buffer));

	Log.AddL(eLogTypes::DBG, MakeTag(), "<< Recv frame {}", frame.Info());

	uint8_t ack = static_cast<uint8_t>(AckTypes::ACK);
	//    logger.AddLogLock(MakeTag(), ">> ACK");
	SerialWrite(&ack, 1);

	consecutiveChecksumErrors = 0;

	if (!OnFrameReceived(frame))
		Log.AddL(eLogTypes::ITF, MakeTag(), "Unhandled frame {}", frame.Info());

	if (state == SendState::WaitingForResponse)
	{
		if (LastCmd.Flow() == eFlowType::AckWithResponse)
		{
			state = SendState::Idle;
			nextSendTime = std::chrono::steady_clock::now() + coolDown;
		}
		else if (LastCmd.Flow() == eFlowType::AckWithResponseCallback)
		{
			state = SendState::WaitingForCallback;
			stateDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1600);
		}
	}
	else if (state == SendState::WaitingForCallback)
	{
		state = SendState::Idle;
		nextSendTime = std::chrono::steady_clock::now() + coolDown;
	}
}

void Interface::ProcessSerialCommunication()
{
	HandleTimeouts();

	if (state == SendState::Idle)
	{
		auto now = std::chrono::steady_clock::now();
		if (now >= nextSendTime)
		{
			CommandFrame next;
			if (TryDequeueNext(next))
			{
				SendCommand(next);
				if (LastCmd.Flow() != eFlowType::Unacknowledged)
					return;
			}
		}
	}

	uint8_t b = 0;
	int n = SerialRead(&b, 1);
	if (n <= 0)
	{
		std::this_thread::yield();
		return;
	}

	if (b == static_cast<uint8_t>(AckTypes::ACK))
	{
		if (state != SendState::WaitingForAck)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "<< Unexpected ACK: state={} frame={}",
					 static_cast<int>(state), LastCmd.cmd.Info());
			return;
		}
		HandleAck();
		return;
	}

	if (b == static_cast<uint8_t>(AckTypes::NAK))
	{
		if (state != SendState::WaitingForAck && LastCmd.Flow() != eFlowType::Unacknowledged)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "<< Unexpected NAK: state={} frame={}",
					 static_cast<int>(state), LastCmd.cmd.Info());
			return;
		}
		HandleNakOrCan(true);
		return;
	}

	if (b == static_cast<uint8_t>(AckTypes::CAN))
	{
		if (state != SendState::WaitingForAck && LastCmd.Flow() != eFlowType::Unacknowledged)
		{
			Log.AddL(eLogTypes::ERR, MakeTag(), "<< Unexpected CAN: state={} frame={}",
					 static_cast<int>(state), LastCmd.cmd.Info());
			return;
		}
		HandleNakOrCan(false);
		return;
	}

	if (b != static_cast<uint8_t>(AckTypes::SOF))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "<< Unexpected byte: 0x{:02X} (state={})", static_cast<int>(b), static_cast<int>(state));
		return;
	}

	std::vector<uint8_t> buffer;
	if (!ReadFrameWithTimeout(buffer))
		return;

	//    logger.AddLogLock(MakeTag(), ">> SOF frame");

	uint8_t checksum = 0xFF;
	for (size_t i = 1; i < buffer.size(); ++i)
		checksum ^= buffer[i];

	if (checksum != 0x00)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "Bad checksum: recvFrameLen={} -> send NAK (consecutiveErrors={})",
				 buffer.size(), consecutiveChecksumErrors + 1);
		uint8_t nak = static_cast<uint8_t>(AckTypes::NAK);
		SerialWrite(&nak, 1);

		consecutiveChecksumErrors++;
		if (consecutiveChecksumErrors >= 3)
			Log.AddL(eLogTypes::ERR, MakeTag(), "Checksum errors: {} consecutive -> reset recommended", consecutiveChecksumErrors);

		stateDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
		return;
	}

	ProcessValidFrame(buffer);
}

void Interface::Test()
{
	APIFrame frame;
 frame.Make(eCommandIds::FUNC_ID_GET_INIT_DATA, std::vector<std::uint8_t>{});
	Enqueue(frame);
}
