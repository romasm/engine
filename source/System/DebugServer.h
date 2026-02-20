#pragma once

#ifdef _DEV

#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

namespace EngineCore
{

class DebugServer
{
public:
	static constexpr uint16_t PORT = 7777;

	DebugServer();
	~DebugServer();

	// Call from main thread each frame: executes queued Lua commands and refreshes the log cache
	void Tick();

	inline static DebugServer* Get() { return instance; }

private:
	void ServerThreadFunction();
	void HandleConnection(uintptr_t clientSocket);

	static std::string EscapeJsonString(const std::string& input);

	// HTTP thread pushes code here; main thread drains and executes it
	std::queue<std::string> pendingLuaCode;
	std::mutex pendingLuaMutex;

	// Main thread builds this; HTTP thread reads it
	std::string cachedLogJson;
	std::mutex cachedLogMutex;
	int cachedLogSize;

	uintptr_t listenSocket;
	std::thread serverThread;
	std::atomic<bool> isRunning;

	static DebugServer* instance;
};

} // namespace EngineCore

#endif // _DEV
