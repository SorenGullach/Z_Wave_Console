#include "Logging.h"
#include "Interface.h"

ZW_Interface::ZW_Interface() {}

bool ZW_Interface::OpenPort(const std::string& portname)
{
    std::scoped_lock lock(serialMutex);
    if (!serial.Open(portname))
    {
        Log.AddL(eLogTypes::INFO, MakeTag(), "OpenPort failed: port='{}'", portname);
        return false;
    }

    Log.AddL(eLogTypes::INFO, MakeTag(), "OpenPort ok: port='{}'", portname);
    Start();
    return true;
}

void ZW_Interface::ClosePort()
{
    Stop();
    std::scoped_lock lock(serialMutex);
    serial.Close();
    Log.AddL(eLogTypes::INFO, MakeTag(), "ClosePort: port closed");
}

void ZW_Interface::Enqueue(const ZW_APIFrame& frame)
{
    std::scoped_lock lock(queueMutex);
    Command cmd;
    cmd.cmd = frame;
    cmd.attempts = 0;
    cmd.FrameBytes.clear();
    cmdQueue.push(cmd);
}

bool ZW_Interface::OnFrameReceived(const ZW_APIFrame& frame)
{
    return false; // not handled
}
bool ZW_Interface::OnFrameReceivedTimeout(const ZW_APIFrame& frame)
{
	return false; // not handled
}


void ZW_Interface::Start()
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
                Run();
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

void ZW_Interface::Stop()
{
    running.store(false);
    if (worker.joinable())
        worker.join();
}

bool ZW_Interface::TryDequeueNext(Command& out)
{
    std::scoped_lock lock(queueMutex);
    if (cmdQueue.empty())
        return false;
    out = cmdQueue.front();
    cmdQueue.pop();
    return true;
}

std::chrono::milliseconds ZW_Interface::BackoffDelay(int attempt) const
{
    int n = attempt; // 0-based
    return std::chrono::milliseconds(100 + n * 1000);
}

void ZW_Interface::SendCommand(Command& cmd)
{
    cmd.FrameBytes = cmd.cmd.Encode_Frame();
    LastCmd = cmd;
    LastCmd.attempts++;

    Log.AddL(eLogTypes::DBG, MakeTag(), ">> Send: cmdId=0x{:02X} name={} flow={} attempt={}",
        static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
        ::ToString(LastCmd.cmd.APICmd.CmdId),
        static_cast<uint8_t>(LastCmd.cmd.APICmd.Flow),
        LastCmd.attempts);
    Log.AddL(eLogTypes::DBG, MakeTag(), "       : bytesLen={} bytes=[{}]",
             LastCmd.FrameBytes.size(),
             ToString(LastCmd.FrameBytes));
    SerialWrite(LastCmd.FrameBytes);

    if (LastCmd.cmd.APICmd.Flow == eFlowType::Unacknowledged)
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

bool ZW_Interface::IsSerialOpen() const
{
    std::scoped_lock lock(serialMutex);
    return serial.IsOpen();
}

int ZW_Interface::SerialRead(uint8_t* buffer, int size)
{
    std::scoped_lock lock(serialMutex);
    return serial.Read(buffer, size);
}

int ZW_Interface::SerialWrite(const uint8_t* buffer, int size)
{
    std::scoped_lock lock(serialMutex);
    return serial.Write(buffer, size);
}

int ZW_Interface::SerialWrite(const std::vector<uint8_t>& buffer)
{
    std::scoped_lock lock(serialMutex);
    return serial.Write(buffer);
}

const std::string ZW_Interface::ToString(const std::vector<uint8_t>& buffer) const
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

void ZW_Interface::HandleTimeouts()
{
    auto now = std::chrono::steady_clock::now();
    if (state == SendState::Idle)
        return;

    if (now <= stateDeadline)
        return;

    switch (state)
    {
    case SendState::WaitingForAck:
        if (LastCmd.cmd.APICmd.Flow == eFlowType::Unacknowledged)
        {
            state = SendState::Idle;
            break;
        }

        if (LastCmd.attempts < maxRetries)
        {
			Log.AddL(eLogTypes::ERR, MakeTag(), "ACK timeout: cmdId=0x{:02X} name={} attempt={}/{} -> retransmit",
                static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
                ::ToString(LastCmd.cmd.APICmd.CmdId),
                LastCmd.attempts,
                maxRetries);
            SendCommand(LastCmd);
        }
        else
        {
			Log.AddL(eLogTypes::ERR, MakeTag(), "ACK timeout: cmdId=0x{:02X} name={} attempt={}/{} -> give up",
                static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
                ::ToString(LastCmd.cmd.APICmd.CmdId),
                LastCmd.attempts,
                maxRetries);
            state = SendState::Idle;
            nextSendTime = now + coolDown;
        }
        break;

    case SendState::WaitingForResponse:
    case SendState::WaitingForCallback:
        //logger.AddLogLock(MakeTag(), "Response/Callback timeout");
        if(!OnFrameReceivedTimeout(LastCmd.cmd))
			Log.AddL(eLogTypes::ERR, MakeTag(), "Frame timeout: cmdId=0x{:02X} name={} state={} info={}",
                static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
                ::ToString(LastCmd.cmd.APICmd.CmdId),
                static_cast<int>(state),
                LastCmd.cmd.Info());
        state = SendState::Idle;
        nextSendTime = now + coolDown;
        break;

    default:
        break;
    }
}

void ZW_Interface::HandleAck()
{
//    logger.AddLogLock(MakeTag(), "<< ACK");
    LastCmd.attempts = 0;

    switch (LastCmd.cmd.APICmd.Flow)
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

void ZW_Interface::HandleNakOrCan(bool isNak)
{
    Log.AddL(eLogTypes::INFO, MakeTag(), "<< {}: cmdId=0x{:02X} name={} flow={} attempt={}/{}",
        isNak ? "NAK" : "CAN",
        static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
        ::ToString(LastCmd.cmd.APICmd.CmdId),
        static_cast<uint8_t>(LastCmd.cmd.APICmd.Flow),
        LastCmd.attempts,
        maxRetries);

    if (LastCmd.cmd.APICmd.Flow == eFlowType::Unacknowledged)
    {
        if (LastCmd.attempts < maxRetries)
        {
            Log.AddL(eLogTypes::INFO, MakeTag(), "Unacknowledged frame NAK/CAN -> retransmit with backoff");
            SendCommand(LastCmd);
        }
        else
        {
            Log.AddL(eLogTypes::INFO, MakeTag(), "Unacknowledged frame NAK/CAN -> max retries reached");
            state = SendState::Idle;
            nextSendTime = std::chrono::steady_clock::now() + coolDown;
        }
        return;
    }

    if (LastCmd.attempts < maxRetries)
    {
        Log.AddL(eLogTypes::INFO, MakeTag(), "NAK/CAN: cmdId=0x{:02X} name={} -> retransmit",
            static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
            ::ToString(LastCmd.cmd.APICmd.CmdId));
        SendCommand(LastCmd);
    }
    else
    {
        Log.AddL(eLogTypes::INFO, MakeTag(), "NAK/CAN: cmdId=0x{:02X} name={} -> max retries reached, give up",
            static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
            ::ToString(LastCmd.cmd.APICmd.CmdId));
        state = SendState::Idle;
        nextSendTime = std::chrono::steady_clock::now() + coolDown;
    }
}

bool ZW_Interface::ReadFrameWithTimeout(std::vector<uint8_t>& buffer)
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
            Log.AddL(eLogTypes::INFO, MakeTag(), "ReadFrameWithTimeout: timeout waiting for payload (>1500ms after SOF)");
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

void ZW_Interface::ProcessValidFrame(const std::vector<uint8_t>& buffer)
{
    ZW_APIFrame frame;
    frame.Decode_Frame(buffer);
    Log.AddL(eLogTypes::DBG, MakeTag(), "<< Recv: bytesLen={} bytes=[{}]",
             buffer.size(),
             ToString(buffer));
    Log.AddL(eLogTypes::DBG, MakeTag(), "       : cmdId=0x{:02X} name={} payloadLen={}",
        static_cast<uint8_t>(frame.APICmd.CmdId),
        ::ToString(frame.APICmd.CmdId),
        frame.payload.size());

    uint8_t ack = static_cast<uint8_t>(AckTypes::ACK);
//    logger.AddLogLock(MakeTag(), ">> ACK");
    SerialWrite(&ack, 1);

    consecutiveChecksumErrors = 0;

    if(!OnFrameReceived(frame))
        Log.AddL(eLogTypes::INFO, MakeTag(), "Unhandled frame: cmdId=0x{:02X} name={} payloadLen={}",
            static_cast<uint8_t>(frame.APICmd.CmdId),
            ::ToString(frame.APICmd.CmdId),
            frame.payload.size());

    if (state == SendState::WaitingForResponse)
    {
        if (LastCmd.cmd.APICmd.Flow == eFlowType::AckWithResponse)
        {
            state = SendState::Idle;
            nextSendTime = std::chrono::steady_clock::now() + coolDown;
        }
        else if (LastCmd.cmd.APICmd.Flow == eFlowType::AckWithResponseCallback)
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

void ZW_Interface::Run()
{
    HandleTimeouts();

    if (state == SendState::Idle)
    {
        auto now = std::chrono::steady_clock::now();
        if (now >= nextSendTime)
        {
            Command next;
            if (TryDequeueNext(next))
            {
                SendCommand(next);
                if (LastCmd.cmd.APICmd.Flow != eFlowType::Unacknowledged)
                    return;
            }
        }
    }

    uint8_t b = 0;
    int n = SerialRead(&b, 1);
    if (n <= 0) {
        std::this_thread::yield();
        return;
    }

    if (b == static_cast<uint8_t>(AckTypes::ACK))
    {
        if (state != SendState::WaitingForAck)
        {
            Log.AddL(eLogTypes::INFO, MakeTag(), "<< Unexpected ACK: state={} lastCmdId=0x{:02X} name={}",
                static_cast<int>(state),
                static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
                ::ToString(LastCmd.cmd.APICmd.CmdId));
            return;
        }
        HandleAck();
        return;
    }

    if (b == static_cast<uint8_t>(AckTypes::NAK))
    {
        if (state != SendState::WaitingForAck && LastCmd.cmd.APICmd.Flow != eFlowType::Unacknowledged)
        {
            Log.AddL(eLogTypes::INFO, MakeTag(), "<< Unexpected NAK: state={} lastCmdId=0x{:02X} name={}",
                static_cast<int>(state),
                static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
                ::ToString(LastCmd.cmd.APICmd.CmdId));
            return;
        }
        HandleNakOrCan(true);
        return;
    }

    if (b == static_cast<uint8_t>(AckTypes::CAN))
    {
        if (state != SendState::WaitingForAck && LastCmd.cmd.APICmd.Flow != eFlowType::Unacknowledged)
        {
            Log.AddL(eLogTypes::INFO, MakeTag(), "<< Unexpected CAN: state={} lastCmdId=0x{:02X} name={}",
                static_cast<int>(state),
                static_cast<uint8_t>(LastCmd.cmd.APICmd.CmdId),
                ::ToString(LastCmd.cmd.APICmd.CmdId));
            return;
        }
        HandleNakOrCan(false);
        return;
    }

    if (b != static_cast<uint8_t>(AckTypes::SOF))
    {
        Log.AddL(eLogTypes::INFO, MakeTag(), "<< Unexpected byte: 0x{:02X} (state={})", static_cast<int>(b), static_cast<int>(state));
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
        Log.AddL(eLogTypes::INFO, MakeTag(), "Bad checksum: recvFrameLen={} -> send NAK (consecutiveErrors={})", buffer.size(), consecutiveChecksumErrors + 1);
        uint8_t nak = static_cast<uint8_t>(AckTypes::NAK);
        SerialWrite(&nak, 1);

        consecutiveChecksumErrors++;
        if (consecutiveChecksumErrors >= 3)
            Log.AddL(eLogTypes::INFO, MakeTag(), "Checksum errors: {} consecutive -> reset recommended", consecutiveChecksumErrors);

        stateDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        return;
    }

    ProcessValidFrame(buffer);
}
