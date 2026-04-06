#include "SerialPort.h"
#include "Logging.h"

#if defined(__linux__)
#include <chrono>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <thread>
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

static void DisableModemLines(int fd)
{
	int status = 0;
	if (ioctl(fd, TIOCMGET, &status) != 0)
		return;

	status &= ~TIOCM_DTR;
	status &= ~TIOCM_RTS;
	ioctl(fd, TIOCMSET, &status);
}

static void CleanStartAfterReopen(int fd)
{
	tcflush(fd, TCIOFLUSH);
	DisableModemLines(fd);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	tcflush(fd, TCIOFLUSH);
	EnableModemLines(fd);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	tcflush(fd, TCIOFLUSH);

	// Toggle DTR to reset Z-Wave chip
	int flags = TIOCM_DTR;
	ioctl(fd, TIOCMBIC, &flags); // DTR low
	usleep(100000);
	ioctl(fd, TIOCMBIS, &flags); // DTR high
	usleep(100000);
	tcflush(fd, TCIOFLUSH);
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

    // Open the device in non-blocking mode and make sure it is a TTY.
	const auto fullName = NormalizeDevicePath(portname);
	const auto fd = open(fullName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
   if (fd < 0)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: open port='{}'", fullName);
		return false;
	}

	if (!isatty(fd))
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: device is not a tty port='{}'", fullName);
       close(fd);
		return false;
	}

	termios ttyOptions{};
  // Read the current termios state before applying our raw serial settings.
	if (tcgetattr(fd, &ttyOptions) != 0)
	{
        Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: tcgetattr initial port='{}'", fullName);
		close(fd);
		return false;
	}

	// Preserve the previous setup flow by re-reading the active configuration.
	if (tcgetattr(fd, &ttyOptions) < 0)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: tcgetattr reread port='{}'", fullName);
		close(fd);
		return false;
	}

    // Disable input processing and software flow control.
	ttyOptions.c_iflag = 0;

    // Disable output post-processing.
	ttyOptions.c_oflag = 0;

   // Disable canonical mode, echo, signals, and other line-oriented handling.
	ttyOptions.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

    // Configure 8 data bits, no parity, and one stop bit.
	ttyOptions.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	ttyOptions.c_cflag |= CS8;


   // Keep reads non-blocking with no inter-character timeout.
	ttyOptions.c_cc[VMIN] = 0;
	ttyOptions.c_cc[VTIME] = 0;

  // Use the fixed 115200 baud rate required by the interface.
	if (cfsetispeed(&ttyOptions, B115200) < 0 || cfsetospeed(&ttyOptions, B115200) < 0)
	{
      Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: set baud rate port='{}'", fullName);
		close(fd);
		return false;
	}

  // Apply the updated serial settings and flush pending input/output.
	if (tcsetattr(fd, TCSAFLUSH, &ttyOptions) < 0) 
	{
      Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: tcsetattr port='{}'", fullName);
		close(fd);
		return false;
	}

	flock(fd, LOCK_EX | LOCK_NB);
/*
	if (ioctl(fd, TIOCEXCL, NULL) < 0)
	{
		Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: TIOCEXCL port='{}'", fullName);
		close(fd);
		return false;
	}
*/
   // Force a clean controller/adapter state on every open, even if the previous
	// debug session ended without running normal shutdown.
	CleanStartAfterReopen(fd);
	serialHandle = fd;

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
   {
		Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: CreateFileA port='{}'", fullName);
		return false;
	}

	DCB dcbSerialParams{};
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (!GetCommState(hSerial, &dcbSerialParams))
	{
       Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: GetCommState port='{}'", fullName);
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
       Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: SetCommState port='{}'", fullName);
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
       Log.AddL(eLogTypes::ERR, MakeTag(), "SerialPort::Open failed: SetCommTimeouts port='{}'", fullName);
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
