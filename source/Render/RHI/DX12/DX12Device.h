#pragma once
#include "GfxDevice.h"
#include "GfxSync.h"
#include "DX12Types.h"
#include "DX12CommandQueue.h"
#include "DX12CommandList.h"
#include "DX12SwapChain.h"

// DX12Device  — implements IGfxDevice on top of ID3D12Device8.
// DX12FrameScheduler — implements IGfxFrameScheduler; owns the swapchain,
//   command allocator pools, and per-frame fence values.

namespace EngineCore::RHI::DX12
{

// -----------------------------------------------------------------------
// Descriptor heap sizes

static constexpr uint32_t RtvHeapSize       = 512;  // back buffers + render targets + per-mip RTVs
static constexpr uint32_t DsvHeapSize       = 32;   // depth targets
static constexpr uint32_t CbvSrvUavHeapSize = 16384; // bindless shader resources (0..4095 static, 4096..16383 dynamic ring)
static constexpr uint32_t SamplerHeapSize   = 64;   // dynamic samplers

// -----------------------------------------------------------------------
// DX12FrameScheduler

class DX12Device; // forward decl

class DX12FrameScheduler : public IGfxFrameScheduler
{
	// Maximum number of command lists that can be in-flight simultaneously.
	// One per parallel render pass (shadow, depth-prepass, opaque, transparent, compute, …).
	static constexpr uint32_t MaxConcurrentCommandLists = 8;

	// Pool entry: one command list (with FramesInFlight allocators) + a lock-free
	// availability flag.  std::atomic is not movable/copyable so the pool is a
	// plain array — no std::vector.
	struct PoolEntry
	{
		DX12CommandList   commandList;
		std::atomic<bool> inUse{false};
	};

public:
	DX12FrameScheduler() = default;
	~DX12FrameScheduler() override { Destroy(); }

	bool Init(ID3D12Device8*        device,
	          IDXGIFactory6*        factory,
	          DX12CommandQueue*     directQueue,
	          ID3D12DescriptorHeap* rtvHeap,
	          uint32_t              rtvDescriptorSize,
	          HWND                  hwnd,
	          uint32_t              width,
	          uint32_t              height)
	{
		m_directQueue = directQueue;

		// Initialise every command list in the pool.
		for(auto& entry : m_pool)
		{
			if(!entry.commandList.Init(device, D3D12_COMMAND_LIST_TYPE_DIRECT, FramesInFlight))
				return false;
		}

		if(hwnd)
		{
			if(!m_swapChain.Init(factory, directQueue->GetQueue(), hwnd,
			                     width, height, device, rtvHeap, rtvDescriptorSize))
				return false;
		}

		return true;
	}

	// Create (or re-create) the swap chain after the window HWND is available.
	bool InitSwapChain(IDXGIFactory6*        factory,
	                   HWND                  hwnd,
	                   uint32_t              width,
	                   uint32_t              height,
	                   ID3D12Device8*        device,
	                   ID3D12DescriptorHeap* rtvHeap,
	                   uint32_t              rtvDescriptorSize)
	{
		return m_swapChain.Init(factory, m_directQueue->GetQueue(),
		                        hwnd, width, height,
		                        device, rtvHeap, rtvDescriptorSize);
	}

	void Destroy()
	{
		m_swapChain.Destroy();
		for(auto& entry : m_pool)
			entry.commandList.Destroy();
	}

	// -----------------------------------------------------------------------
	// IGfxFrameScheduler

	void BeginFrame() override
	{
		m_currentFrameIndex = (m_currentFrameIndex + 1) % FramesInFlight;

		// Wait for the GPU to finish with the allocators we're about to reuse.
		m_directQueue->WaitForFenceValue(m_frameFenceValues[m_currentFrameIndex]);

		// Prepare every pool entry for the new frame index so any thread can
		// immediately Open() whichever list it allocates.
		for(auto& entry : m_pool)
			entry.commandList.PrepareForFrame(m_currentFrameIndex);
	}

	void ExecuteCommandLists(uint32_t count, IGfxCommandList* const* lists) override
	{
		std::vector<ID3D12CommandList*> nativeLists;
		nativeLists.reserve(count);
		for(uint32_t i = 0; i < count; ++i)
		{
			auto* dx12List = static_cast<DX12CommandList*>(lists[i]);
			nativeLists.push_back(dx12List->GetCommandList());
		}

		m_frameFenceValues[m_currentFrameIndex] =
			m_directQueue->ExecuteAndSignal(
				static_cast<uint32_t>(nativeLists.size()),
				nativeLists.data());
	}

	void Present(uint32_t syncInterval) override
	{
		if(m_swapChain.IsInitialized())
			m_swapChain.Present(syncInterval);
	}

	void FlushGPU() override { m_directQueue->Flush(); }

	uint32_t CurrentFrameIndex() override { return m_currentFrameIndex; }

	// Grab a free command list from the pool (lock-free, thread-safe).
	// Returns nullptr if all MaxConcurrentCommandLists slots are in use.
	IGfxCommandList* AllocateCommandList() override
	{
		for(auto& entry : m_pool)
		{
			bool expected = false;
			if(entry.inUse.compare_exchange_strong(
				   expected, true,
				   std::memory_order_acquire,
				   std::memory_order_relaxed))
			{
				return &entry.commandList;
			}
		}
		return nullptr; // pool exhausted — caller must not exceed MaxConcurrentCommandLists passes
	}

	// Return a command list to the pool after submission.
	void FreeCommandList(IGfxCommandList* list) override
	{
		for(auto& entry : m_pool)
		{
			if(&entry.commandList == list)
			{
				entry.inUse.store(false, std::memory_order_release);
				return;
			}
		}
	}

	// Configure the SRV/UAV descriptor ring and root signatures on all pool command lists.
	void InitPoolBindingState(ID3D12DescriptorHeap* cbvSrvUavHeap,
	                          uint32_t descSize,
	                          uint32_t ringStart, uint32_t ringSize,
	                          D3D12_CPU_DESCRIPTOR_HANDLE nullSrvCpu,
	                          D3D12_CPU_DESCRIPTOR_HANDLE nullUavCpu,
	                          ID3D12RootSignature* graphicsRootSig,
	                          ID3D12RootSignature* computeRootSig)
	{
		for(auto& entry : m_pool)
			entry.commandList.InitBindingState(
				cbvSrvUavHeap, descSize, ringStart, ringSize,
				nullSrvCpu, nullUavCpu,
				graphicsRootSig, computeRootSig);
	}

	// Accessors used by DX12Device for back-buffer RTV setup.
	DX12SwapChain& GetSwapChain() { return m_swapChain; }

private:
	DX12CommandQueue* m_directQueue      = nullptr; // owned by DX12Device
	PoolEntry         m_pool[MaxConcurrentCommandLists];
	DX12SwapChain     m_swapChain;

	uint32_t m_currentFrameIndex                = 0;
	uint64_t m_frameFenceValues[FramesInFlight] = {};
};

// -----------------------------------------------------------------------
// DX12Device

class DX12Device : public IGfxDevice
{
public:
	DX12Device() = default;
	~DX12Device() override { Shutdown(); }

	// Init creates the DX12 device, three command queues, descriptor heaps,
	// and upload helpers. If hwnd is non-null a swap chain is created
	// immediately; otherwise call InitSwapChain() later (deferred path).
	bool Init(HWND hwnd) override;
	void Shutdown() override;

	// IGfxDevice overrides
	bool IsDX12() const override { return true; }

	// Create the swap chain after the fact (deferred window creation).
	bool InitSwapChain(HWND hwnd, uint32_t width, uint32_t height) override;
	bool ResizeSwapChain(uint32_t width, uint32_t height) override;

	// -----------------------------------------------------------------------
	// Buffer creation / destruction

	GfxBuffer* CreateBuffer(const BufferDesc& desc) override;
	void       DestroyBuffer(GfxBuffer* buffer) override { delete Cast(buffer); }

	// -----------------------------------------------------------------------
	// Texture creation / destruction

	GfxTexture* CreateTexture(const TextureDesc& desc) override;
	void        DestroyTexture(GfxTexture* texture) override { delete Cast(texture); }

	// DDS loading: parses DDS, uploads all subresources, returns SRV that owns the texture
	GfxSRV* LoadDDSFromMemory(const uint8_t* data, uint32_t dataSize) override;

	// -----------------------------------------------------------------------
	// View creation

	GfxRTV* CreateRTV(GfxTexture* texture, DXGI_FORMAT format,
	                  uint32_t mipSlice = 0, uint32_t arraySlice = 0) override;

	GfxDSV* CreateDSV(GfxTexture* texture, DXGI_FORMAT format,
	                  uint32_t mipSlice = 0) override;

	GfxSRV* CreateSRV(GfxTexture* texture, DXGI_FORMAT format,
	                  uint32_t mostDetailedMip = 0,
	                  uint32_t mipLevels = UINT32_MAX) override;

	GfxSRV* CreateSRV(GfxBuffer* buffer, uint32_t firstElement,
	                  uint32_t numElements) override;

	GfxUAV* CreateUAV(GfxTexture* texture, DXGI_FORMAT format,
	                  uint32_t mipSlice = 0) override;

	GfxUAV* CreateUAV(GfxBuffer* buffer, uint32_t firstElement,
	                  uint32_t numElements) override;

	void DestroyRTV(GfxRTV* rtv)  override { delete Cast(rtv); }
	void DestroyDSV(GfxDSV* dsv)  override { delete Cast(dsv); }
	void DestroyView(GfxSRV* srv) override { delete Cast(srv); }
	void DestroyView(GfxUAV* uav) override { delete Cast(uav); }

	// -----------------------------------------------------------------------
	// Sampler

	GfxSampler* CreateSampler(const SamplerDesc& desc) override;
	void        DestroySampler(GfxSampler* sampler) override { delete Cast(sampler); }

	// -----------------------------------------------------------------------
	// Pipeline state objects — Phase 4

	GfxPipelineState* CreateGraphicsPSO(const GraphicsPSODesc& desc) override;
	GfxPipelineState* CreateComputePSO(const ComputePSODesc& desc) override;
	GfxPipelineState* CreateMeshShaderPSO(const MeshShaderPSODesc& desc) override;
	void DestroyPSO(GfxPipelineState* pso) override { delete Cast(pso); }

	GfxInputLayout* CreateInputLayout(const void* /*bytecode*/, size_t /*size*/,
	    const D3D11_INPUT_ELEMENT_DESC* /*elements*/, uint32_t /*count*/) override
	{ return nullptr; /* Phase 4: DX12 embeds layout in PSO */ }
	void DestroyInputLayout(GfxInputLayout* layout) override { delete Cast(layout); }

	// -----------------------------------------------------------------------
	// Raytracing acceleration structures and pipelines (Phase 6)

	GfxBLAS* CreateBLAS(const BLASDesc& desc) override;
	void     DestroyBLAS(GfxBLAS* blas) override { delete Cast(blas); }

	GfxTLAS* CreateTLAS(const TLASDesc& desc) override;
	void     DestroyTLAS(GfxTLAS* tlas) override { delete Cast(tlas); }

	GfxRaytracingPipeline* CreateRaytracingPipeline(const RaytracingPipelineDesc& desc) override;
	void DestroyRaytracingPipeline(GfxRaytracingPipeline* pipeline) override
	{ delete Cast(pipeline); }

	// -----------------------------------------------------------------------
	// Back buffer access

	GfxTexture* GetBackBuffer(uint32_t frameIndex) override;
	uint32_t    GetBackBufferCount()            override { return DX12SwapChain::BackBufferCount; }
	uint32_t    GetCurrentBackBufferIndex()     override
	{ return m_frameScheduler ? m_frameScheduler->GetSwapChain().GetCurrentBackBufferIndex() : 0; }
	GfxRTV*     GetBackBufferRTV(uint32_t index) override
	{ return (index < DX12SwapChain::BackBufferCount) ? &m_backBufferRTVs[index] : nullptr; }

	// -----------------------------------------------------------------------
	// Diagnostics

	const char* GetAdapterName()   override { return m_adapterName; }
	bool        SupportsRaytracing()  override { return m_supportsRaytracing; }
	bool        SupportsMeshShaders() override { return m_supportsMeshShaders; }

	// -----------------------------------------------------------------------
	// Internal accessors

	ID3D12Device8*       GetDevice()         const { return m_device; }
	IDXGIFactory6*       GetFactory()        const { return m_factory; }
	DX12CommandQueue*    GetDirectQueue()    const { return m_directQueue; }
	DX12CommandQueue*    GetComputeQueue()   const { return m_computeQueue; }
	DX12CommandQueue*    GetCopyQueue()      const { return m_copyQueue; }
	DX12FrameScheduler*  GetFrameScheduler() const { return m_frameScheduler; }

	// Root signatures — separate for graphics (per-stage CBVs) and compute.
	// Graphics: [0-3] VS b0-b3, [4-13] PS b0-b9, [14] GS b0, [15] HS b0,
	//           [16] DS b0, [17] SRV table, [18] UAV table + static samplers s0-s7
	// Compute:  [0-3] CS b0-b3, [4] SRV table, [5] UAV table + static samplers s0-s7
	ID3D12RootSignature* GetGraphicsRootSignature() const { return m_graphicsRootSignature; }
	ID3D12RootSignature* GetComputeRootSignature()  const { return m_computeRootSignature; }

	// Graphics root parameter indices
	static constexpr uint32_t GfxRP_VS_CBV_Base  = 0;   // 4 slots: params 0-3
	static constexpr uint32_t GfxRP_VS_CBV_Count = 4;
	static constexpr uint32_t GfxRP_PS_CBV_Base  = 4;   // 10 slots: params 4-13
	static constexpr uint32_t GfxRP_PS_CBV_Count = 10;
	static constexpr uint32_t GfxRP_GS_CBV       = 14;
	static constexpr uint32_t GfxRP_HS_CBV       = 15;
	static constexpr uint32_t GfxRP_DS_CBV       = 16;
	static constexpr uint32_t GfxRP_SRV_Table    = 17;
	static constexpr uint32_t GfxRP_UAV_Table    = 18;
	static constexpr uint32_t GfxRP_Count        = 19;

	// Compute root parameter indices
	static constexpr uint32_t CsRP_CBV_Base  = 0;   // 4 slots: params 0-3
	static constexpr uint32_t CsRP_CBV_Count = 4;
	static constexpr uint32_t CsRP_SRV_Table = 4;
	static constexpr uint32_t CsRP_UAV_Table = 5;
	static constexpr uint32_t CsRP_Count     = 6;

	// Dynamic descriptor ring region (for per-draw SRV/UAV table copies)
	static constexpr uint32_t DynamicDescriptorStart = 4096;

	// Null descriptor accessors (for unbound SRV/UAV slots)
	const DX12SRV* GetNullSRV() const { return &m_nullSRV; }
	const DX12UAV* GetNullUAV() const { return &m_nullUAV; }

	ID3D12DescriptorHeap* GetRtvHeap()       const { return m_rtvHeap; }
	ID3D12DescriptorHeap* GetDsvHeap()       const { return m_dsvHeap; }
	ID3D12DescriptorHeap* GetCbvSrvUavHeap() const { return m_cbvSrvUavHeap; }
	ID3D12DescriptorHeap* GetSamplerHeap()   const { return m_samplerHeap; }

	uint32_t GetRtvDescriptorSize()       const { return m_rtvDescriptorSize; }
	uint32_t GetDsvDescriptorSize()       const { return m_dsvDescriptorSize; }
	uint32_t GetCbvSrvUavDescriptorSize() const { return m_cbvSrvUavDescriptorSize; }
	uint32_t GetSamplerDescriptorSize()   const { return m_samplerDescriptorSize; }

private:
	// Device creation helpers
	bool CreateDevice();
	bool CreateQueues();
	bool CreateDescriptorHeaps();
	bool CreateUploadHelpers();
	bool CreateRootSignatures();
	void QueryCapabilities();

	// Descriptor allocation (linear; no free list needed until Phase 5+)
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateRtvHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDsvHandle();
	uint32_t AllocateCbvSrvUavSlot(D3D12_CPU_DESCRIPTOR_HANDLE& outCpu,
	                                D3D12_GPU_DESCRIPTOR_HANDLE& outGpu);
	uint32_t AllocateSamplerSlot(D3D12_CPU_DESCRIPTOR_HANDLE& outCpu,
	                              D3D12_GPU_DESCRIPTOR_HANDLE& outGpu);

	// Synchronous GPU upload (blocks until GPU completes the copy)
	void UploadBufferSync(ID3D12Resource* dest, const void* data,
	                      uint64_t sizeBytes, D3D12_RESOURCE_STATES finalState);
	void UploadTextureSync(ID3D12Resource* dest,
	                       const D3D12_RESOURCE_DESC& destDesc, const void* data);

	// Wrap swapchain back buffers in DX12Texture for IGfxDevice::GetBackBuffer
	void InitBackBufferTextures();

	// -----------------------------------------------------------------------
	// Data

	// Device
	IDXGIFactory6*  m_factory = nullptr;
	IDXGIAdapter1*  m_adapter = nullptr;
	ID3D12Device8*  m_device  = nullptr;

	// Command queues (one per type)
	DX12CommandQueue* m_directQueue  = nullptr; // graphics + compute + copy
	DX12CommandQueue* m_computeQueue = nullptr; // async compute
	DX12CommandQueue* m_copyQueue    = nullptr; // background copy / streaming

	// Descriptor heaps
	ID3D12DescriptorHeap* m_rtvHeap       = nullptr; // CPU-only, non-shader-visible
	ID3D12DescriptorHeap* m_dsvHeap       = nullptr; // CPU-only, non-shader-visible
	ID3D12DescriptorHeap* m_cbvSrvUavHeap = nullptr; // shader-visible bindless
	ID3D12DescriptorHeap* m_samplerHeap   = nullptr; // shader-visible samplers

	uint32_t m_rtvDescriptorSize       = 0;
	uint32_t m_dsvDescriptorSize       = 0;
	uint32_t m_cbvSrvUavDescriptorSize = 0;
	uint32_t m_samplerDescriptorSize   = 0;

	// Descriptor allocators (linear counter per heap)
	// Slots 0..BackBufferCount-1 in the RTV heap are reserved for swap chain back buffers.
	uint32_t m_nextRtvIndex       = DX12SwapChain::BackBufferCount;
	uint32_t m_nextDsvIndex       = 0;
	uint32_t m_nextCbvSrvUavIndex = 0;
	uint32_t m_nextSamplerIndex   = 0;

	// Upload helpers — a dedicated allocator + command list used for one-shot
	// synchronous resource uploads during CreateBuffer/CreateTexture.
	ID3D12CommandAllocator*    m_uploadAllocator   = nullptr;
	ID3D12GraphicsCommandList* m_uploadCommandList = nullptr;

	// Back-buffer texture wrappers (AddRef'd; released in Shutdown)
	DX12Texture m_backBufferTextures[DX12SwapChain::BackBufferCount];

	// Back-buffer RTV wrappers (CPU descriptor handles from swap chain)
	DX12RTV m_backBufferRTVs[DX12SwapChain::BackBufferCount];

	// Frame scheduler (owns swapchain + per-frame allocators)
	DX12FrameScheduler* m_frameScheduler = nullptr;

	// Root signatures (Phase 10: separate graphics and compute)
	ID3D12RootSignature* m_graphicsRootSignature = nullptr;
	ID3D12RootSignature* m_computeRootSignature  = nullptr;

	// Null descriptors for unbound SRV/UAV slots in descriptor tables
	DX12SRV m_nullSRV;
	DX12UAV m_nullUAV;

	// Capability cache
	bool m_supportsRaytracing  = false;
	bool m_supportsMeshShaders = false;
	char m_adapterName[128]    = "DirectX 12";
	size_t m_dedicatedVideoMemoryMB = 0;
};

} // namespace EngineCore::RHI::DX12
