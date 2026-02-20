#include "stdafx.h"

#ifdef _DEV

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include "DebugServer.h"
#include "Log.h"
#include "LuaVM.h"

namespace EngineCore
{

DebugServer* DebugServer::instance = nullptr;

DebugServer::DebugServer() : isRunning(false), listenSocket((uintptr_t)INVALID_SOCKET), cachedLogSize(0)
{
	instance = this;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ERR("DebugServer: WSAStartup failed");
		return;
	}

	SOCKET nativeSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (nativeSocket == INVALID_SOCKET)
	{
		ERR("DebugServer: socket() failed: %d", WSAGetLastError());
		WSACleanup();
		return;
	}
	listenSocket = (uintptr_t)nativeSocket;

	int reuseAddr = 1;
	::setsockopt(nativeSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseAddr, sizeof(reuseAddr));

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(PORT);

	if (::bind(nativeSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		ERR("DebugServer: bind() failed on port %d: %d", PORT, WSAGetLastError());
		::closesocket(nativeSocket);
		listenSocket = (uintptr_t)INVALID_SOCKET;
		WSACleanup();
		return;
	}

	if (::listen(nativeSocket, 4) == SOCKET_ERROR)
	{
		ERR("DebugServer: listen() failed: %d", WSAGetLastError());
		::closesocket(nativeSocket);
		listenSocket = (uintptr_t)INVALID_SOCKET;
		WSACleanup();
		return;
	}

	isRunning = true;
	serverThread = std::thread(&DebugServer::ServerThreadFunction, this);

	LOG("DebugServer listening on http://127.0.0.1:%d", PORT);
}

DebugServer::~DebugServer()
{
	isRunning = false;
	SOCKET nativeSocket = (SOCKET)listenSocket;
	if (nativeSocket != INVALID_SOCKET)
	{
		::closesocket(nativeSocket);
		listenSocket = (uintptr_t)INVALID_SOCKET;
	}
	if (serverThread.joinable())
		serverThread.join();
	WSACleanup();
	instance = nullptr;
}

void DebugServer::Tick()
{
	// Rebuild cached log JSON whenever new entries arrive
	int currentLogSize = Log::GetBufferSize();
	if (currentLogSize != cachedLogSize)
	{
		std::string json = "[";
		for (int i = 0; i < currentLogSize; i++)
		{
			if (i > 0) json += ",";
			json += "{\"prefix\":\"" + EscapeJsonString(Log::GetBufferPrefix(i))
				  + "\",\"text\":\""   + EscapeJsonString(Log::GetBufferText(i)) + "\"}";
		}
		json += "]";

		std::lock_guard<std::mutex> lock(cachedLogMutex);
		cachedLogJson = std::move(json);
		cachedLogSize = currentLogSize;
	}

	// Execute Lua code queued by the HTTP thread
	std::queue<std::string> localQueue;
	{
		std::lock_guard<std::mutex> lock(pendingLuaMutex);
		std::swap(localQueue, pendingLuaCode);
	}

	while (!localQueue.empty())
	{
		std::string code = localQueue.front();
		localQueue.pop();

		if (luaL_dostring(LSTATE, code.c_str()) != 0)
		{
			const char* errorMessage = lua_tostring(LSTATE, -1);
			if (errorMessage)
				LUA_ERROR("DebugServer exec: %s", errorMessage);
			lua_pop(LSTATE, 1);
		}
	}
}

void DebugServer::ServerThreadFunction()
{
	SOCKET nativeSocket = (SOCKET)listenSocket;
	while (isRunning)
	{
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(nativeSocket, &readSet);

		timeval timeout = {0, 100000}; // 100ms poll interval
		int selectResult = ::select(0, &readSet, nullptr, nullptr, &timeout);

		if (selectResult > 0 && FD_ISSET(nativeSocket, &readSet))
		{
			SOCKET clientSocket = ::accept(nativeSocket, nullptr, nullptr);
			if (clientSocket != INVALID_SOCKET)
			{
				HandleConnection((uintptr_t)clientSocket);
				::closesocket(clientSocket);
			}
		}
	}
}

void DebugServer::HandleConnection(uintptr_t clientSocket)
{
	SOCKET nativeSocket = (SOCKET)clientSocket;

	char requestBuffer[8192] = {};
	int bytesReceived = ::recv(nativeSocket, requestBuffer, sizeof(requestBuffer) - 1, 0);
	if (bytesReceived <= 0)
		return;

	std::string request(requestBuffer, bytesReceived);

	// Parse first line: "METHOD /path HTTP/1.x"
	size_t firstLineEnd = request.find("\r\n");
	if (firstLineEnd == std::string::npos)
		return;

	std::string firstLine = request.substr(0, firstLineEnd);

	size_t methodEnd = firstLine.find(' ');
	if (methodEnd == std::string::npos)
		return;

	std::string method = firstLine.substr(0, methodEnd);

	size_t pathStart = methodEnd + 1;
	size_t pathEnd = firstLine.find(' ', pathStart);
	if (pathEnd == std::string::npos)
		return;

	std::string path = firstLine.substr(pathStart, pathEnd - pathStart);

	// Strip query string from path
	size_t queryStart = path.find('?');
	if (queryStart != std::string::npos)
		path = path.substr(0, queryStart);

	// Extract body (everything after the blank line separating headers from body)
	std::string body;
	size_t headerEnd = request.find("\r\n\r\n");
	if (headerEnd != std::string::npos)
		body = request.substr(headerEnd + 4);

	// Handle CORS preflight
	if (method == "OPTIONS")
	{
		std::string response =
			"HTTP/1.1 204 No Content\r\n"
			"Access-Control-Allow-Origin: *\r\n"
			"Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
			"Access-Control-Allow-Headers: Content-Type\r\n"
			"Connection: close\r\n"
			"\r\n";
		::send(nativeSocket, response.c_str(), (int)response.size(), 0);
		return;
	}

	std::string responseBody;
	std::string contentType = "application/json";
	int statusCode = 200;
	std::string statusText = "OK";

	if (method == "GET" && path == "/log")
	{
		std::lock_guard<std::mutex> lock(cachedLogMutex);
		responseBody = cachedLogJson.empty() ? "[]" : cachedLogJson;
	}
	else if (method == "GET" && path == "/logfile")
	{
		responseBody = "{\"path\":\"" + EscapeJsonString(Log::GetFilePath()) + "\"}";
	}
	else if (method == "POST" && path == "/exec")
	{
		if (!body.empty())
		{
			std::lock_guard<std::mutex> lock(pendingLuaMutex);
			pendingLuaCode.push(body);
			responseBody = "{\"status\":\"queued\"}";
		}
		else
		{
			statusCode = 400;
			statusText = "Bad Request";
			responseBody = "{\"error\":\"empty body\"}";
		}
	}
	else if (method == "GET" && path == "/")
	{
		contentType = "text/html";
		responseBody =
			"<!DOCTYPE html><html><head><title>Engine Debug</title></head><body>"
			"<h2>Engine Debug Server</h2>"
			"<ul>"
			"<li>GET /log &mdash; log entries as JSON array of {prefix, text}</li>"
			"<li>GET /logfile &mdash; path to the current log file on disk</li>"
			"<li>POST /exec &mdash; execute Lua code (raw body)</li>"
			"</ul>"
			"</body></html>";
	}
	else
	{
		statusCode = 404;
		statusText = "Not Found";
		responseBody = "{\"error\":\"not found\"}";
	}

	std::string response =
		"HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n"
		"Content-Type: " + contentType + "; charset=utf-8\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Content-Length: " + std::to_string(responseBody.size()) + "\r\n"
		"Connection: close\r\n"
		"\r\n" + responseBody;

	send(nativeSocket, response.c_str(), (int)response.size(), 0);
}

std::string DebugServer::EscapeJsonString(const std::string& input)
{
	std::string output;
	output.reserve(input.size());
	for (char character : input)
	{
		if      (character == '"')  output += "\\\"";
		else if (character == '\\') output += "\\\\";
		else if (character == '\n') output += "\\n";
		else if (character == '\r') output += "\\r";
		else if (character == '\t') output += "\\t";
		else output += character;
	}
	return output;
}

} // namespace EngineCore

#endif // _DEV
