#pragma once
#include "DX12Fence.h"

// DX12CommandQueue — wraps a single ID3D12CommandQueue with an integrated fence.
// The engine maintains three queues: direct (graphics + compute), compute-only,
// and copy. All three are created in DX12Device; this class manages one of them.

namespace EngineCore::RHI::DX12
{

class DX12CommandQueue
{
public:
	DX12CommandQueue() = default;
	~DX12CommandQueue() { Destroy(); }

	DX12CommandQueue(const DX12CommandQueue&) = delete;
	DX12CommandQueue& operator=(const DX12CommandQueue&) = delete;

	bool Init(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type     = type;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue));
		if(FAILED(hr)) return false;

		return m_fence.Init(device);
	}

	void Destroy()
	{
		m_fence.Destroy();
		if(m_queue) { m_queue->Release(); m_queue = nullptr; }
	}

	// Execute command lists and signal the fence.
	// Returns the fence value that will be signaled when execution completes.
	uint64_t ExecuteAndSignal(uint32_t count, ID3D12CommandList* const* lists)
	{
		m_queue->ExecuteCommandLists(count, lists);
		return m_fence.Signal(m_queue);
	}

	void WaitForFenceValue(uint64_t value) { m_fence.WaitForValue(value); }

	// Flush all pending GPU work on this queue (CPU stall).
	void Flush() { m_fence.WaitForGPU(m_queue); }

	ID3D12CommandQueue* GetQueue() const { return m_queue; }

private:
	ID3D12CommandQueue* m_queue = nullptr;
	DX12Fence           m_fence;
};

} // namespace EngineCore::RHI::DX12
