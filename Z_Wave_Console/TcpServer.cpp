
#include <algorithm>
#include <iostream>

#include "TcpServer.h"
#include "Logging.h"

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

TcpServer::TcpServer(int port)
	: port(port), listenSocket(INVALID_SOCKET), socketsInitialized(false), clientSocket(INVALID_SOCKET)
{
}

TcpServer::~TcpServer()
{
	Cleanup();
}

bool TcpServer::Init()
{
	if (socketsInitialized)
		return true;

	WSADATA wsaData{};
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;
	socketsInitialized = true;

	addrinfo hints{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* result = nullptr;
	const std::string portStr = std::to_string(port);
	if (getaddrinfo(nullptr, portStr.c_str(), &hints, &result) != 0)
		return false;

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		freeaddrinfo(result);
		return false;
	}

	if (bind(listenSocket, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR)
	{
		freeaddrinfo(result);
		closesocket(listenSocket);
		listenSocket = INVALID_SOCKET;
		return false;
	}
	freeaddrinfo(result);

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(listenSocket);
		listenSocket = INVALID_SOCKET;
		return false;
	}

	return true;
}

void TcpServer::Cleanup()
{
	{
		std::scoped_lock lock(clientMutex);
		if (clientSocket != INVALID_SOCKET)
		{
			closesocket(clientSocket);
			clientSocket = INVALID_SOCKET;
		}
	}

	if (listenSocket != INVALID_SOCKET)
	{
		closesocket(listenSocket);
		listenSocket = INVALID_SOCKET;
	}

	if (socketsInitialized)
	{
		WSACleanup();
		socketsInitialized = false;
	}
}

bool TcpServer::SendToClient(const std::string& line)
{
	std::scoped_lock lock(clientMutex);
	if (clientSocket == INVALID_SOCKET)
		return false;
	return SendLine(clientSocket, line);
}

bool TcpServer::SendLine(SOCKET s, const std::string& line)
{
	std::string out = line;
	out += "\n";
	const char* p = out.c_str();
	int remaining = static_cast<int>(out.size());
	while (remaining > 0)
	{
		int sent = send(s, p, remaining, 0);
		if (sent == SOCKET_ERROR || sent == 0)
			return false;
		p += sent;
		remaining -= sent;
	}
	return true;
}

bool TcpServer::RecvLine(SOCKET s, std::string& line)
{
	line.clear();
	char ch = 0;
	for (;;)
	{
		int r = recv(s, &ch, 1, 0);
		if (r == 0)
			return false;
		if (r == SOCKET_ERROR)
			return false;
		if (ch == '\n')
			return true;
		line.push_back(ch);
		if (line.size() > 64 * 1024)
			return false;
	}
}

std::string TcpServer::TrimLine(std::string s)
{
	while (!s.empty() && (s.back() == '\r' || s.back() == '\n'))
		s.pop_back();
	return s;
}

void TcpServer::Run()
{
	if (!Init())
		throw std::runtime_error("Failed to initialize sockets");

	std::cout << "TCP server running on port " << port << "\n";

	for (;;)
	{
		std::cout << "Waiting for connection...\n";
		SOCKET acceptedSocket = accept(listenSocket, nullptr, nullptr);
		if (acceptedSocket == INVALID_SOCKET)
			throw std::runtime_error("accept failed");
		{
			std::scoped_lock lock(clientMutex);
			clientSocket = acceptedSocket;
		}

		std::cout << "Client connected.\n";

		for (;;)
		{
			std::string line;
			SOCKET currentClient;
			{
				std::scoped_lock lock(clientMutex);
				currentClient = clientSocket;
			}

			if (!RecvLine(currentClient, line))
			{
				std::cout << "Connection closed.\n";
				{
					std::scoped_lock lock(clientMutex);
					closesocket(clientSocket);
					clientSocket = INVALID_SOCKET;
				}
				break;
			}

			line = TrimLine(std::move(line));
			if (line.empty())
				continue;

			line.erase(std::remove_if(line.begin(), line.end(),
				[](unsigned char c) { return c < 32 && c != '\n' && c != '\r'; }),
				line.end());

			//std::cout << "RX: " << line << "\n";

			std::string reply = HandleMessage(line);

			if (!SendLine(currentClient, reply))
			{
				std::cout << "Connection closed.\n";
				{
					std::scoped_lock lock(clientMutex);
					closesocket(clientSocket);
					clientSocket = INVALID_SOCKET;
				}
				break;
			}

			//std::cout << "TX: " << reply << "\n";
		}
	}
}
