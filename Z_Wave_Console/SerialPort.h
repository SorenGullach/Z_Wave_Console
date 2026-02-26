#pragma once
#include <string>
#include <vector>
#include <cstdint>
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

