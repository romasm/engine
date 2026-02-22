#pragma once
#include "RHITypes.h"

// IGfxFrameScheduler — manages per-frame synchronization and command list submission.
// DX11 backend: trivial (immediate context, no explicit sync needed).
// DX12 backend: manages command allocator pools, fences, and back-buffer rotation.

namespace EngineCore::RHI
{

class IGfxCommandList;

class IGfxFrameScheduler
{
public:
	static constexpr uint32_t FramesInFlight = 2; // double-buffered

	virtual ~IGfxFrameScheduler() {}

	// Called at the beginning of each frame before any render work.
	// On DX12: waits for the oldest in-flight frame to complete, resets allocator.
	virtual void BeginFrame() = 0;

	// Called after all command lists have been recorded.
	// On DX12: executes the command lists on the direct queue.
	virtual void ExecuteCommandLists(uint32_t count, IGfxCommandList* const* lists) = 0;

	// Present the back buffer. syncInterval 0 = no vsync, 1 = vsync.
	virtual void Present(uint32_t syncInterval) = 0;

	// Stall the CPU until the GPU has completed all in-flight work.
	// Used for teardown and resource deletion.
	virtual void FlushGPU() = 0;

	// Index of the back buffer being rendered to this frame (0 or 1 for double-buffer).
	virtual uint32_t CurrentFrameIndex() = 0;

	// Allocate a new command list for recording.
	// On DX11 returns the singleton wrapper; on DX12 returns a free list from pool.
	virtual IGfxCommandList* AllocateCommandList() = 0;

	// Return a command list to the pool after submission.
	virtual void FreeCommandList(IGfxCommandList* list) = 0;
};

} // namespace EngineCore::RHI
