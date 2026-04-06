#include "SerialPort.h"

#if defined(__linux__)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static constexpr std::intptr_t InvalidSerialHandle = -1;

static std::string NormalizeDevicePath(const std::string& port)
{
	if (port.rfind("/dev/", 0) == 0)
		return port;

	return "/dev/" + port;
}

static void EnableModemLines(int fd)
{
	int status = 0;
	if (ioctl(fd, TIOCMGET, &status) != 0)
		return;

	status |= TIOCM_DTR;
	status |= TIOCM_RTS;
	ioctl(fd, TIOCMSET, &status);
}

SerialPort::SerialPort()
	: serialHandle(InvalidSerialHandle)
{
}

SerialPort::~SerialPort()
{
	Close();
}

bool SerialPort::Open(const std::string& portname)
{
	this->portName = portname;

	if (IsOpen())
		return true;

	const auto fullName = NormalizeDevicePath(portname);
	const auto fd = open(fullName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0 || !isatty(fd))
		return false;

	termios ttyOptions{};
	if (tcgetattr(fd, &ttyOptions) != 0)
	{
		close(fd);
		return false;
	}

	//
	// Get the current configuration of the serial interface
	//
	if (tcgetattr(fd, &ttyOptions) < 0) 
	{
		close(fd);
		return false;
	}

	//
	// Input flags - Turn off input processing
	//
	// convert break to null byte, no CR to NL translation,
	// no NL to CR translation, don't mark parity errors or breaks
	// no input parity check, don't strip high bit off,
	// no XON/XOFF software flow control
	//
	//ttyOptions.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
		//				INLCR | PARMRK | INPCK | ISTRIP | IXON);
	ttyOptions.c_iflag = 0;

	//
	// Output flags - Turn off output processing
	//
	// no CR to NL translation, no NL to CR-NL translation,
	// no NL to CR translation, no column 0 CR suppression,
	// no Ctrl-D suppression, no fill characters, no case mapping,
	// no local output processing
	//
	// config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
	//                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
	ttyOptions.c_oflag = 0;

	//
	// No line processing
	//
	// echo off, echo newline off, canonical mode off,
	// extended input processing off, signal chars off
	//
	ttyOptions.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

	//
	// Turn off character processing
	//
	// clear current char size mask, no parity checking,
	// no output processing, force 8 bit input
	//
	ttyOptions.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	ttyOptions.c_cflag |= CS8;

	//
	// One input byte is enough to return from read()
	// Inter-character timer off
	//
	ttyOptions.c_cc[VMIN] = 0;
	ttyOptions.c_cc[VTIME] = 0;

	//
	// Communication speed (simple version, using the predefined
	// constants)
	//
	if (cfsetispeed(&ttyOptions, B115200) < 0 || cfsetospeed(&ttyOptions, B115200) < 0)
	{
		close(fd);
		return false;
	}

	//
	// Finally, apply the configuration
	//
	if (tcsetattr(fd, TCSAFLUSH, &ttyOptions) < 0) 
	{
		close(fd);
		return false;
	}
	/*
	if (ioctl(fd, TIOCEXCL, NULL) < 0)
	{
		close(fd);
		return false;
	}
	*/
	/*
	cfmakeraw(&ttyOptions);
	cfsetospeed(&ttyOptions, B115200);
	cfsetispeed(&ttyOptions, B115200);

	config.c_iflag &= ~ // Disable all input modes
		( IGNBRK // Ignore break condition
		| BRKINT // Signal break condition
		| ICRNL // Map CR to NL on input
		| INLCR // Map NL to CR on input
		| PARMRK // Mark parity errors
		| INPCK // Enable parity checking
		| ISTRIP // Strip 8th bit off characters
		| IXON // Enable XON/XOFF flow control
		);

	ttyOptions.c_cflag &= ~CSIZE;
	ttyOptions.c_cflag |= CS8;
	ttyOptions.c_cflag |= CLOCAL | CREAD;
	ttyOptions.c_cflag &= ~(PARENB | PARODD);
	ttyOptions.c_cflag &= ~CSTOPB;
	ttyOptions.c_cflag &= ~CRTSCTS;
	ttyOptions.c_iflag &= ~(IXON | IXOFF | IXANY);
	ttyOptions.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);

	ttyOptions.c_cc[VMIN] = 0;
	ttyOptions.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSANOW, &ttyOptions) != 0)
	{
		close(fd);
		return false;
	}
	*/
	EnableModemLines(fd);
	serialHandle = fd;
	tcflush(fd, TCIOFLUSH);

	return true;
}

void SerialPort::Close()
{
	const auto fd = static_cast<int>(serialHandle);
	if (fd >= 0)
	{
		close(fd);
		serialHandle = InvalidSerialHandle;
	}
}

bool SerialPort::IsOpen() const
{
	return serialHandle >= 0;
}

int SerialPort::Read(std::vector<uint8_t>& buffer)
{
	return Read(buffer.data(), static_cast<int>(buffer.size()));
}

int SerialPort::Read(uint8_t* buffer, int size)
{
	if (!IsOpen() || !buffer || size <= 0)
		return -1;

	const auto bytesRead = read(static_cast<int>(serialHandle), buffer, static_cast<size_t>(size));
	if (bytesRead < 0 && errno == EAGAIN)
		return 0;

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

	const auto fd = static_cast<int>(serialHandle);
	const auto bytesWritten = write(fd, buffer, static_cast<size_t>(size));
	if (bytesWritten < 0)
		return -1;

	tcdrain(fd);

	return static_cast<int>(bytesWritten);
}

#elif defined(_WIN32)

// Prevent `windows.h` from including the legacy `winsock.h`.
// This project uses Winsock2 (`winsock2.h`) elsewhere (see `TcpServer.h`).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>

static HANDLE ToHandle(std::intptr_t handle)
{
	return reinterpret_cast<HANDLE>(handle);
}

static std::intptr_t FromHandle(HANDLE handle)
{
	return reinterpret_cast<std::intptr_t>(handle);
}

SerialPort::SerialPort()
	: serialHandle(FromHandle(INVALID_HANDLE_VALUE))
{
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
	auto hSerial = CreateFileA(
		fullName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr);

	if (hSerial == INVALID_HANDLE_VALUE)
		return false;

	DCB dcbSerialParams{};
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (!GetCommState(hSerial, &dcbSerialParams))
	{
		CloseHandle(hSerial);
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
		CloseHandle(hSerial);
		return false;
	}

	// Non-blocking-ish reads with small timeouts.
	COMMTIMEOUTS timeouts{};
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(hSerial, &timeouts))
	{
		CloseHandle(hSerial);
		return false;
	}

	serialHandle = FromHandle(hSerial);
	PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
	return true;
}

void SerialPort::Close()
{
	auto hSerial = ToHandle(serialHandle);
	if (hSerial != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hSerial);
		serialHandle = FromHandle(INVALID_HANDLE_VALUE);
	}
}

bool SerialPort::IsOpen() const
{
	return ToHandle(serialHandle) != INVALID_HANDLE_VALUE;
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
	if (!ReadFile(ToHandle(serialHandle), buffer, static_cast<DWORD>(size), &bytesRead, nullptr))
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
	if (!WriteFile(ToHandle(serialHandle), buffer, static_cast<DWORD>(size), &bytesWritten, nullptr))
		return -1;

	return static_cast<int>(bytesWritten);
}

#else
#pragma message("Platform not supported")
#endif
