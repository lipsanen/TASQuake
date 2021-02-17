#include "ipc.hpp"

#ifdef _WINDOWS
#include <chrono>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

#ifdef _WINDOWS
#pragma comment(lib, "Ws2_32.lib")
static u_long BLOCKING = 1;
#endif
using namespace ipc;

static PrintFunc PRINT_FUNC = nullptr;
static bool WINSOCK_INITIALIZED = false;
const int BUFLEN = 32767;

void CloseSocket(int& socket) {
#ifdef _WINDOWS
	closesocket(socket);
	socket = INVALID_SOCKET;
#endif
}

static bool DataAvailable(int&socket) {
#ifdef _WINDOWS
	fd_set read;
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	FD_ZERO(&read);
	FD_SET(socket, &read);
	int result = select(socket+1, &read, &read, &read, &timeout);

	return result > 0;
#else
	return false;
#endif
}

ipc::IPCServer::IPCServer()
{
#ifdef _WINDOWS
	listenSocket = INVALID_SOCKET;
	clientSocket = INVALID_SOCKET;
#endif
	RECV_BUFFER = new char[BUFLEN];
}

void IPCServer::InitWinsock()
{
#ifdef _WINDOWS
	WSADATA wsaData;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {

		WINSOCK_INITIALIZED = false;
		Print("WSAStartup failed: %d\n", iResult);
		return;
	}

	WINSOCK_INITIALIZED = true;
#endif
}

void IPCServer::AddPrintFunc(PrintFunc func)
{
	PRINT_FUNC = func;
}

void IPCServer::StartListening(const char* port)
{
#ifdef _WINDOWS
	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	int iResult = getaddrinfo("127.0.0.1", port, &hints, &result);
	if (iResult != 0) {
		Print("getaddrinfo failed: %d\n", iResult);
		return;
	}

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (listenSocket == INVALID_SOCKET) {
		Print("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		return;
	}

	// Setup the TCP listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		Print("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		CloseSocket(listenSocket);
		return;
	}

	freeaddrinfo(result);
	iResult = ioctlsocket(listenSocket, FIONBIO, &BLOCKING);
	if (iResult == -1) {
		Print("bind failed with error: %d\n", WSAGetLastError());
		CloseSocket(listenSocket);
		return;
	}
#endif
}

void ipc::IPCServer::CloseConnections()
{
#ifdef _WINDOWS
	if(listenSocket != SOCKET_ERROR)
		CloseSocket(listenSocket);
	if (clientSocket != SOCKET_ERROR)
		CloseSocket(clientSocket);
#endif
}

void ipc::IPCServer::Loop()
{
	CheckForConnections();
	ReadMessages();
	DispatchMessages();
}

bool ipc::IPCServer::BlockForMessages(const std::string& type, int timeoutMsec)
{
#ifdef _WINDOWS
	long long msecElapsed = 0;
	auto begin = std::chrono::steady_clock::now();

	if (msgQueue.find(type) != msgQueue.end()) {
		auto& vec = msgQueue.find(type)->second;
		auto end = std::chrono::steady_clock::now();
		ReadMessages();

		while (vec.empty() && msecElapsed < timeoutMsec) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			end = std::chrono::steady_clock::now();
			msecElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
			ReadMessages();
		}
		bool result = !vec.empty();
		DispatchMessages(type);
		return result;
	}
	else
	{
		Print("Message type has no callback!\n");
		return false;
	}
#else
	return false;
#endif
}

void ipc::IPCServer::AddCallback(std::string type, MsgCallback callback, bool blocking)
{
	callbacks[type] = callback;
	blockingMap[type] = blocking;
	msgQueue[type] = std::vector<nlohmann::json>();
}

void ipc::IPCServer::SendMsg(const nlohmann::json& msg)
{
#ifdef _WINDOWS
	if (clientSocket == SOCKET_ERROR) {
		Print("No client connected.\n");
		return;
	}

	std::string out = msg.dump();
	const char* string = out.c_str();

	for (std::size_t i = 0; i <= out.size();)
	{
		int result = send(clientSocket, string, out.size() - i + 1, 0);
		if (result == SOCKET_ERROR) {
			Print("Send failed: %d\n", WSAGetLastError());
			CloseSocket(clientSocket);
			return;
		}

		i += result;
		string += result;
	}
#else
#endif
}

bool ipc::IPCServer::ClientConnected()
{
#ifdef _WINDOWS
	return clientSocket != SOCKET_ERROR;
#else
	return false;
#endif
}

ipc::IPCServer::~IPCServer()
{
	delete[] RECV_BUFFER;
}

void ipc::IPCServer::ReadMessages()
{
#ifdef _WINDOWS
	if (clientSocket == SOCKET_ERROR || !DataAvailable(clientSocket)) {
		return;
	}

	int offset = 0;
	int result;

	do {

		result = recv(clientSocket, RECV_BUFFER + offset,  BUFLEN - offset, 0);
		
		if (result > 0) {
			offset += result;
		}
		else if (result == 0) {
			CloseSocket(clientSocket);
			Print("Client disconnected, closing socket.\n");
			return;
		}
		else {
			int	error = WSAGetLastError();

			if (error != WSAEWOULDBLOCK && error != WSAECONNREFUSED) {
				CloseSocket(clientSocket);
				Print("Client disconnected, closing socket.\n");
				return;
			}
		}
		
	} while (result > 0 && offset < BUFLEN);

	int bytesRead = offset;
	int startIndex = 0;

	for (int i = 0; i < bytesRead; ++i)
	{
		if (RECV_BUFFER[i] == '\0') {
			char* str = RECV_BUFFER + startIndex;
			try {
				nlohmann::json msg = nlohmann::json::parse(str, RECV_BUFFER + i);

				if (msg.find("type") != msg.end()) {
					std::string type = msg["type"];

					if (callbacks.find(type) == callbacks.end())
					{
						Print("No callback for message type %s\n", type.c_str());
					}
					else {
						if (msgQueue.find(type) == msgQueue.end()) {
							msgQueue[type] = std::vector<nlohmann::json>();
						}

						msgQueue[type].push_back(msg);
					}
				}
				else {
					Print("Bad message received.\n");
				}
			}
			catch (const std::exception& ex) {
				Print("Error parsing message: %s\n", ex.what());
			}		
			startIndex = i + 1;
		}
	}
#endif
}

void ipc::IPCServer::CheckForConnections()
{
#ifdef _WINDOWS
	if (listenSocket == SOCKET_ERROR || clientSocket != SOCKET_ERROR)
	{
		return;
	}

	int result = listen(listenSocket, SOMAXCONN);

	if (result == SOCKET_ERROR) {

		int	error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK || error == WSAECONNREFUSED)
		{
			return;
		}

		Print("Listen failed with error: %ld\n", WSAGetLastError());
		CloseSocket(listenSocket);
		return;
	}

	// Accept a client socket
	clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		int	error = WSAGetLastError();
		if (error == WSAEWOULDBLOCK || error == WSAECONNREFUSED)
		{
			return;
		}

		Print("Accept failed: %d\n", WSAGetLastError());
		CloseSocket(listenSocket);
		return;
	}

	Print("Got client\n");
	ioctlsocket(clientSocket, FIONBIO, &BLOCKING);
#endif
}

void ipc::IPCServer::DispatchMessages()
{
	for (auto& pair : msgQueue) {
		if (!blockingMap[pair.first] && !pair.second.empty()) {
			DispatchMessages(pair.first);
		}
	}
}

void ipc::IPCServer::DispatchMessages(const std::string& type)
{
	auto& vec = msgQueue.find(type)->second;
	auto callbackIt = callbacks.find(type);
	if (callbackIt != callbacks.end()) {
		MsgCallback callback = callbackIt->second;

		for (auto& item : vec) {
			callback(item);
		}
	}
	vec.clear();
}

void ipc::Print(const char* msg, ...)
{
#ifdef _WINDOWS
	if (PRINT_FUNC != nullptr) {
		const int buflen = 256;
		char buffer[buflen];
		va_list args;
		va_start(args, msg);
		vsnprintf(buffer, buflen - 1, msg, args);

		PRINT_FUNC(buffer);
	}
#endif
}

void ipc::Shutdown_IPC()
{
#ifdef _WINDOWS
	WSACleanup();
	WINSOCK_INITIALIZED = false;
#endif
}

bool ipc::Winsock_Initialized()
{
	return WINSOCK_INITIALIZED;
}
