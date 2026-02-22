#pragma once
#include "DX12Types.h"

// CPU/GPU synchronization via ID3D12Fence.
// Wraps a single fence and a Win32 event handle for blocking CPU waits.

namespace EngineCore::RHI::DX12
{

class DX12Fence
{
public:
	DX12Fence() = default;
	~DX12Fence() { Destroy(); }

	// Not copyable — owns OS handle and COM reference.
	DX12Fence(const DX12Fence&) = delete;
	DX12Fence& operator=(const DX12Fence&) = delete;

	bool Init(ID3D12Device* device, uint64_t initialValue = 0)
	{
		m_nextValue           = initialValue + 1;
		m_lastCompletedValue  = initialValue;

		HRESULT hr = device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&m_fence));
		if(FAILED(hr)) return false;

		m_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		return m_event != nullptr;
	}

	void Destroy()
	{
		if(m_fence) { m_fence->Release(); m_fence = nullptr; }
		if(m_event) { CloseHandle(m_event); m_event = nullptr; }
	}

	// Signal the fence from the GPU side via the given queue.
	// Returns the fence value that will be set when the queue finishes.
	uint64_t Signal(ID3D12CommandQueue* queue)
	{
		const uint64_t value = m_nextValue++;
		queue->Signal(m_fence, value);
		return value;
	}

	// Block the CPU until the GPU fence reaches or exceeds the given value.
	void WaitForValue(uint64_t value)
	{
		if(IsComplete(value))
			return;

		m_fence->SetEventOnCompletion(value, m_event);
		WaitForSingleObjectEx(m_event, INFINITE, FALSE);
		m_lastCompletedValue = m_fence->GetCompletedValue();
	}

	// Stall the CPU until all GPU work submitted to the given queue is done.
	void WaitForGPU(ID3D12CommandQueue* queue)
	{
		WaitForValue(Signal(queue));
	}

	bool IsComplete(uint64_t value)
	{
		if(value <= m_lastCompletedValue)
			return true;
		m_lastCompletedValue = m_fence->GetCompletedValue();
		return value <= m_lastCompletedValue;
	}

	uint64_t NextValue() const { return m_nextValue; }

private:
	ID3D12Fence* m_fence              = nullptr;
	HANDLE       m_event              = nullptr;
	uint64_t     m_nextValue          = 1;
	uint64_t     m_lastCompletedValue = 0;
};

} // namespace EngineCore::RHI::DX12
