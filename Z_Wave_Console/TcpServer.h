#pragma once

#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <mutex>

class TcpServer
{
public:
	explicit TcpServer(int port);
	~TcpServer();

	TcpServer(const TcpServer&) = delete;
	TcpServer& operator=(const TcpServer&) = delete;

	void Run();
	bool SendToClient(const std::string& line);

protected:
	virtual std::string HandleMessage(const std::string& message) = 0;

private:
	bool Init();
	void Cleanup();
	bool SendLine(SOCKET s, const std::string& line);
	bool RecvLine(SOCKET s, std::string& line);
	static std::string TrimLine(std::string s);

	int port;
	SOCKET listenSocket;
	bool socketsInitialized;

	std::mutex clientMutex;
	SOCKET clientSocket;
};
