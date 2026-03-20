#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Prevent `windows.h` from including the legacy `winsock.h`.
// This project uses Winsock2 (`winsock2.h`) elsewhere (see `TcpServer.h`).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>

class SerialPort
{
public:
    SerialPort();
    ~SerialPort();

    bool Open(const std::string& portname);
    void Close();
    bool IsOpen() const;

    int Read(std::vector<uint8_t>& buffer);
    int Read(uint8_t* buffer, int size);
    int Write(const std::vector<uint8_t>& buffer);
    int Write(const uint8_t* buffer, int size);

private:
    std::string portName;
    HANDLE hSerial;
    DCB dcbSerialParams;
    COMMTIMEOUTS timeouts;
};

