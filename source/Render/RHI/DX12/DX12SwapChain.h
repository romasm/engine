#pragma once
#include "DX12Types.h"

// IDXGISwapChain4 wrapper with per-back-buffer RTV management.
// Created and owned by DX12FrameScheduler.
// Uses flip-discard presentation and triple buffering (3 back buffers).
//
// This swapchain is independent of Window::m_pSwapChain (which is the
// DX11 IDXGISwapChain1 used during the migration period). Window's
// swapchain is retired in Phase 8 once all rendering is on DX12.

namespace EngineCore::RHI::DX12
{

class DX12SwapChain
{
public:
	static constexpr uint32_t    BackBufferCount  = 3;
	static constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	DX12SwapChain() = default;
	~DX12SwapChain() { Destroy(); }

	DX12SwapChain(const DX12SwapChain&) = delete;
	DX12SwapChain& operator=(const DX12SwapChain&) = delete;

	bool Init(IDXGIFactory6*      factory,
	          ID3D12CommandQueue* directQueue,
	          HWND                hwnd,
	          uint32_t            width,
	          uint32_t            height,
	          ID3D12Device*       device,
	          ID3D12DescriptorHeap* rtvHeap,
	          uint32_t            rtvDescriptorSize)
	{
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width              = max(1u, width);
		desc.Height             = max(1u, height);
		desc.Format             = BackBufferFormat;
		desc.Stereo             = FALSE;
		desc.SampleDesc.Count   = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount        = BackBufferCount;
		desc.Scaling            = DXGI_SCALING_STRETCH;
		desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
		desc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		IDXGISwapChain1* swapChain1 = nullptr;
		HRESULT hr = factory->CreateSwapChainForHwnd(directQueue, hwnd, &desc,
			nullptr, nullptr, &swapChain1);
		if(FAILED(hr)) return false;

		// Disable Alt+Enter automatic fullscreen; engine handles it manually.
		factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

		hr = swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain));
		swapChain1->Release();
		if(FAILED(hr)) return false;

		m_rtvDescriptorSize = rtvDescriptorSize;
		return AcquireBackBuffers(device, rtvHeap);
	}

	void Destroy()
	{
		ReleaseBackBuffers();
		if(m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
	}

	bool IsInitialized() const { return m_swapChain != nullptr; }

	void Present(uint32_t syncInterval)
	{
		m_swapChain->Present(syncInterval, 0);
		m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	uint32_t                    GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }
	ID3D12Resource*             GetCurrentBackBuffer()      const { return m_backBuffers[m_currentBackBufferIndex]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV()   const { return m_rtvHandles[m_currentBackBufferIndex]; }
	IDXGISwapChain4*            GetSwapChain()              const { return m_swapChain; }

	// Resize the swap chain back buffers (call after GPU flush).
	bool Resize(uint32_t width, uint32_t height,
	            ID3D12Device* device, ID3D12DescriptorHeap* rtvHeap)
	{
		if(!m_swapChain) return false;

		ReleaseBackBuffers();

		HRESULT hr = m_swapChain->ResizeBuffers(
			BackBufferCount, max(1u, width), max(1u, height),
			BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		if(FAILED(hr)) return false;

		return AcquireBackBuffers(device, rtvHeap);
	}

	// Per-frame back buffer access (used by DX12Device::GetBackBuffer)
	ID3D12Resource*             GetBackBuffer(uint32_t index) const
	{ return (index < BackBufferCount) ? m_backBuffers[index] : nullptr; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(uint32_t index) const
	{ return (index < BackBufferCount) ? m_rtvHandles[index] : D3D12_CPU_DESCRIPTOR_HANDLE{}; }

private:
	bool AcquireBackBuffers(ID3D12Device* device, ID3D12DescriptorHeap* rtvHeap)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
			rtvHeap->GetCPUDescriptorHandleForHeapStart();

		for(uint32_t i = 0; i < BackBufferCount; ++i)
		{
			HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
			if(FAILED(hr)) return false;

			device->CreateRenderTargetView(m_backBuffers[i], nullptr, rtvHandle);
			m_rtvHandles[i] = rtvHandle;
			rtvHandle.ptr += m_rtvDescriptorSize;
		}

		m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
		return true;
	}

	void ReleaseBackBuffers()
	{
		for(uint32_t i = 0; i < BackBufferCount; ++i)
			if(m_backBuffers[i]) { m_backBuffers[i]->Release(); m_backBuffers[i] = nullptr; }
	}

	IDXGISwapChain4*            m_swapChain              = nullptr;
	ID3D12Resource*             m_backBuffers[BackBufferCount] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandles[BackBufferCount]  = {};
	uint32_t                    m_currentBackBufferIndex = 0;
	uint32_t                    m_rtvDescriptorSize      = 0;
};

} // namespace EngineCore::RHI::DX12
