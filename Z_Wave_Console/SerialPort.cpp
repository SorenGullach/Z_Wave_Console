#include "SerialPort.h"

SerialPort::SerialPort()
	: hSerial(INVALID_HANDLE_VALUE)
{
	SecureZeroMemory(&dcbSerialParams, sizeof(dcbSerialParams));
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	SecureZeroMemory(&timeouts, sizeof(timeouts));
}

SerialPort::~SerialPort()
{
	Close();
}

static std::string NormalizeComPath(const std::string& port)
{
	// Win32 requires the "\\\\.\\COM10" form for COM10+.
	if (port.rfind("\\\\.\\", 0) == 0)
		return port;
	return "\\\\.\\" + port;
}

bool SerialPort::Open(const std::string& portname)
{
	this->portName = portname;

	if (IsOpen())
		return true;

	const auto fullName = NormalizeComPath(portname);
	hSerial = CreateFileA(
		fullName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr);

	if (hSerial == INVALID_HANDLE_VALUE)
		return false;

	if (!GetCommState(hSerial, &dcbSerialParams))
	{
		Close();
		return false;
	}

	// Reasonable defaults; adjust as needed.
	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
	dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;

	if (!SetCommState(hSerial, &dcbSerialParams))
	{
		Close();
		return false;
	}

	// Non-blocking-ish reads with small timeouts.
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(hSerial, &timeouts))
	{
		Close();
		return false;
	}

	PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
	return true;
}

void SerialPort::Close()
{
	if (hSerial != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hSerial);
		hSerial = INVALID_HANDLE_VALUE;
	}
}

bool SerialPort::IsOpen() const
{
	return hSerial != INVALID_HANDLE_VALUE;
}

int SerialPort::Read(std::vector<uint8_t>& buffer)
{
	return Read(buffer.data(), static_cast<int>(buffer.size()));
}

int SerialPort::Read(uint8_t* buffer, int size)
{
	if (!IsOpen() || !buffer || size <= 0)
		return -1;

	DWORD bytesRead = 0;
	if (!ReadFile(hSerial, buffer, static_cast<DWORD>(size), &bytesRead, nullptr))
		return -1;

	return static_cast<int>(bytesRead);
}

int SerialPort::Write(const std::vector<uint8_t>& buffer)
{
	return Write(buffer.data(), static_cast<int>(buffer.size()));
}

int SerialPort::Write(const uint8_t* buffer, int size)
{
	if (!IsOpen() || !buffer || size <= 0)
		return -1;

	DWORD bytesWritten = 0;
	if (!WriteFile(hSerial, buffer, static_cast<DWORD>(size), &bytesWritten, nullptr))
		return -1;

	return static_cast<int>(bytesWritten);
}
