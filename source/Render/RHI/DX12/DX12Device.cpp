#include "stdafx.h"
#include "DX12Device.h"
#include "Log.h"
#include "ShaderCodeMgr.h"

namespace EngineCore::RHI::DX12
{

// -----------------------------------------------------------------------
// DX12Device::Init

bool DX12Device::Init(HWND hwnd)
{
#ifdef _DEBUG
	// Enable the DX12 debug layer before creating the device so it is active
	// for all subsequent API calls. Must happen before D3D12CreateDevice.
	ComPtr<ID3D12Debug> debugController;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		debugController->EnableDebugLayer();
#endif

	if(!CreateDevice())                  return false;
	if(!CreateQueues())                  return false;
	if(!CreateDescriptorHeaps())         return false;
	if(!CreateUploadHelpers())           return false;
	if(!CreateRootSignatures())  return false;

	QueryCapabilities();

	// Create null SRV and UAV descriptors for unbound slots in descriptor tables.
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		AllocateCbvSrvUavSlot(cpuHandle, gpuHandle);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels     = 1;
		m_device->CreateShaderResourceView(nullptr, &srvDesc, cpuHandle);
		m_nullSRV.cpuHandle = cpuHandle;
		m_nullSRV.gpuHandle = gpuHandle;

		AllocateCbvSrvUavSlot(cpuHandle, gpuHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(nullptr, nullptr, &uavDesc, cpuHandle);
		m_nullUAV.cpuHandle = cpuHandle;
		m_nullUAV.gpuHandle = gpuHandle;
	}

	m_frameScheduler = new DX12FrameScheduler();

	// The swapchain needs a window; if none is provided we create everything
	// else and allow InitSwapChain() to be called later (deferred path).
	if(hwnd)
	{
		RECT clientRect = {};
		GetClientRect(hwnd, &clientRect);
		const uint32_t width  = static_cast<uint32_t>(max(1L, clientRect.right  - clientRect.left));
		const uint32_t height = static_cast<uint32_t>(max(1L, clientRect.bottom - clientRect.top));

		if(!m_frameScheduler->Init(m_device, m_factory, m_directQueue,
		                           m_rtvHeap, m_rtvDescriptorSize,
		                           hwnd, width, height))
			return false;

		InitBackBufferTextures();
	}
	else
	{
		if(!m_frameScheduler->Init(m_device, m_factory, m_directQueue,
		                           m_rtvHeap, m_rtvDescriptorSize,
		                           nullptr, 0, 0))
			return false;
	}

	// Configure the SRV/UAV descriptor ring on all pool command lists.
	const uint32_t ringSize = CbvSrvUavHeapSize - DynamicDescriptorStart;
	m_frameScheduler->InitPoolBindingState(
		m_cbvSrvUavHeap, m_cbvSrvUavDescriptorSize,
		DynamicDescriptorStart, ringSize,
		m_nullSRV.cpuHandle, m_nullUAV.cpuHandle,
		m_graphicsRootSignature, m_computeRootSignature);

	return true;
}

// -----------------------------------------------------------------------
// DX12Device::InitSwapChain (deferred)

bool DX12Device::InitSwapChain(HWND hwnd, uint32_t width, uint32_t height)
{
	if(!m_frameScheduler || !hwnd) return false;

	bool result = m_frameScheduler->InitSwapChain(
		m_factory, hwnd, width, height,
		m_device, m_rtvHeap, m_rtvDescriptorSize);

	if(result)
		InitBackBufferTextures();

	return result;
}

// -----------------------------------------------------------------------
// DX12Device::ResizeSwapChain

bool DX12Device::ResizeSwapChain(uint32_t width, uint32_t height)
{
	if(!m_frameScheduler) return false;

	// Flush all GPU work so back buffer references are safe to release.
	m_frameScheduler->FlushGPU();

	// Release our AddRef'd back-buffer texture wrappers.
	for(auto& texture : m_backBufferTextures)
	{
		if(texture.resource) { texture.resource->Release(); texture.resource = nullptr; }
	}

	bool result = m_frameScheduler->GetSwapChain().Resize(
		width, height, m_device, m_rtvHeap);

	if(result)
		InitBackBufferTextures();

	return result;
}

// -----------------------------------------------------------------------
// DX12Device::Shutdown

void DX12Device::Shutdown()
{
	if(m_frameScheduler)
	{
		m_frameScheduler->FlushGPU(); // drain all in-flight work before releasing
		delete m_frameScheduler;
		m_frameScheduler = nullptr;
	}

	// Release back-buffer wrappers (our AddRef'd references) before the
	// resources themselves are freed by the swapchain teardown above.
	for(auto& texture : m_backBufferTextures)
	{
		if(texture.resource) { texture.resource->Release(); texture.resource = nullptr; }
	}

	if(m_uploadCommandList) { m_uploadCommandList->Release(); m_uploadCommandList = nullptr; }
	if(m_uploadAllocator)   { m_uploadAllocator->Release();   m_uploadAllocator   = nullptr; }

	if(m_copyQueue)    { m_copyQueue->Destroy();    delete m_copyQueue;    m_copyQueue    = nullptr; }
	if(m_computeQueue) { m_computeQueue->Destroy();  delete m_computeQueue; m_computeQueue = nullptr; }
	if(m_directQueue)  { m_directQueue->Destroy();   delete m_directQueue;  m_directQueue  = nullptr; }

	if(m_graphicsRootSignature) { m_graphicsRootSignature->Release(); m_graphicsRootSignature = nullptr; }
	if(m_computeRootSignature)  { m_computeRootSignature->Release();  m_computeRootSignature  = nullptr; }

	if(m_samplerHeap)   { m_samplerHeap->Release();   m_samplerHeap   = nullptr; }
	if(m_cbvSrvUavHeap) { m_cbvSrvUavHeap->Release(); m_cbvSrvUavHeap = nullptr; }
	if(m_dsvHeap)       { m_dsvHeap->Release();       m_dsvHeap       = nullptr; }
	if(m_rtvHeap)       { m_rtvHeap->Release();       m_rtvHeap       = nullptr; }

	if(m_device)  { m_device->Release();  m_device  = nullptr; }
	if(m_adapter) { m_adapter->Release(); m_adapter = nullptr; }
	if(m_factory) { m_factory->Release(); m_factory = nullptr; }
}

// -----------------------------------------------------------------------
// Private: CreateDevice

bool DX12Device::CreateDevice()
{
	// Create DXGI factory with debug flags in debug builds.
	UINT factoryFlags = 0;
#ifdef _DEBUG
	factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory));
	if(FAILED(hr)) return false;

	// Pick the highest-performance GPU (dedicated over integrated).
	ComPtr<IDXGIAdapter1> adapter;
	for(UINT i = 0; m_factory->EnumAdapterByGpuPreference(i,
	        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
	        IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// Skip the Microsoft Basic Render Driver (software rasterizer).
		if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		// Try to create a DX12 device at feature level 12.0 minimum.
		if(SUCCEEDED(D3D12CreateDevice(adapter.Get(),
		    D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
		{
			adapter->AddRef();
			m_adapter = adapter.Get();

			// Store the adapter description for GetAdapterName().
			WideCharToMultiByte(CP_ACP, 0, desc.Description, -1,
				m_adapterName, sizeof(m_adapterName), nullptr, nullptr);

			m_dedicatedVideoMemoryMB = desc.DedicatedVideoMemory / (1024 * 1024);
			break;
		}
	}

	if(!m_device) return false;

	// Upgrade to ID3D12Device8 (required for mesh shaders and DXR).
	ComPtr<ID3D12Device8> device8;
	hr = m_device->QueryInterface(IID_PPV_ARGS(&device8));
	if(SUCCEEDED(hr))
	{
		m_device->Release();
		m_device = device8.Detach();
	}
	// If QI fails (old Windows SDK / driver), keep ID3D12Device — no mesh shaders.

	return true;
}

// -----------------------------------------------------------------------
// Private: CreateQueues

bool DX12Device::CreateQueues()
{
	m_directQueue  = new DX12CommandQueue();
	m_computeQueue = new DX12CommandQueue();
	m_copyQueue    = new DX12CommandQueue();

	if(!m_directQueue->Init(m_device,  D3D12_COMMAND_LIST_TYPE_DIRECT))  return false;
	if(!m_computeQueue->Init(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE)) return false;
	if(!m_copyQueue->Init(m_device,    D3D12_COMMAND_LIST_TYPE_COPY))    return false;

	return true;
}

// -----------------------------------------------------------------------
// Private: CreateDescriptorHeaps

bool DX12Device::CreateDescriptorHeaps()
{
	m_rtvDescriptorSize       = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvDescriptorSize       = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_samplerDescriptorSize   = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// RTV heap — CPU-only (render targets are set by CPU, not referenced in shaders).
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = RtvHeapSize;
		desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap));
		if(FAILED(hr)) return false;
	}

	// DSV heap — CPU-only.
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.NumDescriptors = DsvHeapSize;
		desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvHeap));
		if(FAILED(hr)) return false;
	}

	// CBV/SRV/UAV heap — shader-visible bindless heap.
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = CbvSrvUavHeapSize;
		desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HRESULT hr = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_cbvSrvUavHeap));
		if(FAILED(hr)) return false;
	}

	// Sampler heap — shader-visible, separate from CBV/SRV/UAV per DX12 rules.
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		desc.NumDescriptors = SamplerHeapSize;
		desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HRESULT hr = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_samplerHeap));
		if(FAILED(hr)) return false;
	}

	return true;
}

// -----------------------------------------------------------------------
// Private: CreateUploadHelpers
// One reusable allocator + command list on the direct queue for synchronous
// CPU→GPU uploads during resource creation.

bool DX12Device::CreateUploadHelpers()
{
	HRESULT hr = m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_uploadAllocator));
	if(FAILED(hr)) return false;

	hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_uploadAllocator, nullptr, IID_PPV_ARGS(&m_uploadCommandList));
	if(FAILED(hr)) return false;

	m_uploadCommandList->Close(); // start in closed state; UploadXxxSync will open it
	return true;
}

// -----------------------------------------------------------------------
// Private: QueryCapabilities

void DX12Device::QueryCapabilities()
{
	// Raytracing (DXR tier 1.0+ required).
	D3D12_RAYTRACING_TIER raytracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 opts = {};
		if(SUCCEEDED(m_device->CheckFeatureSupport(
		    D3D12_FEATURE_D3D12_OPTIONS5, &opts, sizeof(opts))))
		{
			raytracingTier = opts.RaytracingTier;
			m_supportsRaytracing = (raytracingTier >= D3D12_RAYTRACING_TIER_1_0);
		}
	}

	// Mesh shaders (tier 1 required for amplification + mesh pipeline).
	D3D12_MESH_SHADER_TIER meshShaderTier = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS7 opts = {};
		if(SUCCEEDED(m_device->CheckFeatureSupport(
		    D3D12_FEATURE_D3D12_OPTIONS7, &opts, sizeof(opts))))
		{
			meshShaderTier = opts.MeshShaderTier;
			m_supportsMeshShaders = (meshShaderTier >= D3D12_MESH_SHADER_TIER_1);
		}
	}

	// Feature level — query the highest supported level.
	const char* featureLevelString = "12.0";
	{
		D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
		D3D_FEATURE_LEVEL levelsToCheck[] = {
			D3D_FEATURE_LEVEL_12_2,
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
		};
		featureLevels.NumFeatureLevels = ARRAYSIZE(levelsToCheck);
		featureLevels.pFeatureLevelsRequested = levelsToCheck;

		if(SUCCEEDED(m_device->CheckFeatureSupport(
		    D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels))))
		{
			switch(featureLevels.MaxSupportedFeatureLevel)
			{
			case D3D_FEATURE_LEVEL_12_2: featureLevelString = "12.2"; break;
			case D3D_FEATURE_LEVEL_12_1: featureLevelString = "12.1"; break;
			default:                     featureLevelString = "12.0"; break;
			}
		}
	}

	// Shader model
	const char* shaderModelString = "6.0";
	{
		D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_7;
		if(SUCCEEDED(m_device->CheckFeatureSupport(
		    D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
		{
			switch(shaderModel.HighestShaderModel)
			{
			case D3D_SHADER_MODEL_6_7: shaderModelString = "6.7"; break;
			case D3D_SHADER_MODEL_6_6: shaderModelString = "6.6"; break;
			case D3D_SHADER_MODEL_6_5: shaderModelString = "6.5"; break;
			case D3D_SHADER_MODEL_6_4: shaderModelString = "6.4"; break;
			case D3D_SHADER_MODEL_6_3: shaderModelString = "6.3"; break;
			case D3D_SHADER_MODEL_6_2: shaderModelString = "6.2"; break;
			case D3D_SHADER_MODEL_6_1: shaderModelString = "6.1"; break;
			default:                   shaderModelString = "6.0"; break;
			}
		}
	}

	// Log device info
	LOG("------ GPU Device Info ------");
	LOG("  API:             DirectX 12");
	LOG("  GPU:             %s", m_adapterName);
	LOG("  VRAM:            %zu MB", m_dedicatedVideoMemoryMB);
	LOG("  Feature level:   %s", featureLevelString);
	LOG("  Shader model:    %s", shaderModelString);
	LOG("  Raytracing:      %s (tier %s)",
		m_supportsRaytracing ? "supported" : "not supported",
		raytracingTier == D3D12_RAYTRACING_TIER_1_1 ? "1.1" :
		raytracingTier == D3D12_RAYTRACING_TIER_1_0 ? "1.0" : "none");
	LOG("  Mesh shaders:    %s (tier %s)",
		m_supportsMeshShaders ? "supported" : "not supported",
		meshShaderTier == D3D12_MESH_SHADER_TIER_1 ? "1" : "none");
	LOG("-----------------------------");
}

// -----------------------------------------------------------------------
// Private: InitBackBufferTextures
// Creates thin DX12Texture wrappers over each swapchain back buffer so
// that callers can use IGfxDevice::GetBackBuffer().

void DX12Device::InitBackBufferTextures()
{
	if(!m_frameScheduler) return;
	auto& swapChain = m_frameScheduler->GetSwapChain();
	if(!swapChain.IsInitialized()) return;

	for(uint32_t index = 0; index < DX12SwapChain::BackBufferCount; ++index)
	{
		ID3D12Resource* backBuffer = swapChain.GetBackBuffer(index);
		if(!backBuffer) continue;

		// AddRef so our DX12Texture holds its own reference independent of the
		// swapchain. Released explicitly in Shutdown().
		backBuffer->AddRef();

		DX12Texture& texture  = m_backBufferTextures[index];
		texture.resource      = backBuffer;
		texture.currentState  = D3D12_RESOURCE_STATE_PRESENT;
		texture.dimension     = TextureDimension::Tex2D;

		D3D12_RESOURCE_DESC resourceDesc = backBuffer->GetDesc();
		texture.width     = static_cast<uint32_t>(resourceDesc.Width);
		texture.height    = resourceDesc.Height;
		texture.depth     = 1;
		texture.mipLevels = 1;
		texture.format    = resourceDesc.Format;

		// Wrap the swap chain's CPU RTV handle for IGfxDevice::GetBackBufferRTV().
		m_backBufferRTVs[index].handle = swapChain.GetBackBufferRTV(index);
	}
}

// -----------------------------------------------------------------------
// Private: Descriptor allocators (linear; indices into each heap)

D3D12_CPU_DESCRIPTOR_HANDLE DX12Device::AllocateRtvHandle()
{
	assert(m_nextRtvIndex < RtvHeapSize && "RTV heap exhausted");
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(m_nextRtvIndex) * m_rtvDescriptorSize;
	++m_nextRtvIndex;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Device::AllocateDsvHandle()
{
	assert(m_nextDsvIndex < DsvHeapSize && "DSV heap exhausted");
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(m_nextDsvIndex) * m_dsvDescriptorSize;
	++m_nextDsvIndex;
	return handle;
}

uint32_t DX12Device::AllocateCbvSrvUavSlot(D3D12_CPU_DESCRIPTOR_HANDLE& outCpu,
                                             D3D12_GPU_DESCRIPTOR_HANDLE& outGpu)
{
	assert(m_nextCbvSrvUavIndex < CbvSrvUavHeapSize && "CBV/SRV/UAV heap exhausted");
	const uint32_t index = m_nextCbvSrvUavIndex++;

	outCpu      = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	outCpu.ptr += static_cast<SIZE_T>(index) * m_cbvSrvUavDescriptorSize;

	outGpu      = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	outGpu.ptr += static_cast<SIZE_T>(index) * m_cbvSrvUavDescriptorSize;

	return index;
}

uint32_t DX12Device::AllocateSamplerSlot(D3D12_CPU_DESCRIPTOR_HANDLE& outCpu,
                                          D3D12_GPU_DESCRIPTOR_HANDLE& outGpu)
{
	assert(m_nextSamplerIndex < SamplerHeapSize && "Sampler heap exhausted");
	const uint32_t index = m_nextSamplerIndex++;

	outCpu      = m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
	outCpu.ptr += static_cast<SIZE_T>(index) * m_samplerDescriptorSize;

	outGpu      = m_samplerHeap->GetGPUDescriptorHandleForHeapStart();
	outGpu.ptr += static_cast<SIZE_T>(index) * m_samplerDescriptorSize;

	return index;
}

// -----------------------------------------------------------------------
// Private: UploadBufferSync
// Copies CPU data into a default-heap buffer via a temporary upload buffer.
// Blocks until the GPU copy is complete.

void DX12Device::UploadBufferSync(ID3D12Resource* dest, const void* data,
                                   uint64_t sizeBytes, D3D12_RESOURCE_STATES finalState)
{
	// Create a temporary upload buffer for the staging data.
	ComPtr<ID3D12Resource> uploadBuffer;
	CD3DX12_HEAP_PROPERTIES uploadProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC   uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeBytes);

	HRESULT hr = m_device->CreateCommittedResource(
		&uploadProps, D3D12_HEAP_FLAG_NONE,
		&uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&uploadBuffer));
	if(FAILED(hr)) return;

	// Map, copy, unmap.
	void* mapped = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	uploadBuffer->Map(0, &readRange, &mapped);
	memcpy(mapped, data, static_cast<size_t>(sizeBytes));
	uploadBuffer->Unmap(0, nullptr);

	// Record the copy and an optional transition to the desired final state.
	m_uploadAllocator->Reset();
	m_uploadCommandList->Reset(m_uploadAllocator, nullptr);

	m_uploadCommandList->CopyBufferRegion(dest, 0, uploadBuffer.Get(), 0, sizeBytes);

	if(finalState != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			dest, D3D12_RESOURCE_STATE_COPY_DEST, finalState);
		m_uploadCommandList->ResourceBarrier(1, &barrier);
	}

	m_uploadCommandList->Close();

	// Execute synchronously; uploadBuffer stays alive in this scope until
	// Flush() returns, guaranteeing the GPU has finished with the staging data.
	ID3D12CommandList* lists[] = { m_uploadCommandList };
	m_directQueue->ExecuteAndSignal(1, lists);
	m_directQueue->Flush();
}

// -----------------------------------------------------------------------
// Private: UploadTextureSync
// Copies mip 0 of a texture from CPU memory into a default-heap resource.
// Handles DX12's required row-pitch alignment in the upload buffer.

void DX12Device::UploadTextureSync(ID3D12Resource* dest,
                                    const D3D12_RESOURCE_DESC& destDesc, const void* data)
{
	// Query DX12-aligned layout for subresource 0.
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT   numRows;
	UINT64 rowSizeBytes;
	UINT64 totalBytes;
	m_device->GetCopyableFootprints(&destDesc, 0, 1, 0,
		&footprint, &numRows, &rowSizeBytes, &totalBytes);

	// Create upload buffer sized to hold the aligned subresource data.
	ComPtr<ID3D12Resource> uploadBuffer;
	CD3DX12_HEAP_PROPERTIES uploadProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC   uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);

	HRESULT hr = m_device->CreateCommittedResource(
		&uploadProps, D3D12_HEAP_FLAG_NONE,
		&uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&uploadBuffer));
	if(FAILED(hr)) return;

	// Copy row by row, inserting the padding required by DX12's row-pitch alignment.
	BYTE* mapped = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mapped));

	const BYTE* src = static_cast<const BYTE*>(data);
	BYTE*       dst = mapped + footprint.Offset;
	for(UINT row = 0; row < numRows; ++row)
	{
		memcpy(dst, src, static_cast<size_t>(rowSizeBytes));
		src += rowSizeBytes;
		dst += footprint.Footprint.RowPitch;
	}

	uploadBuffer->Unmap(0, nullptr);

	// Record copy and transition COPY_DEST → COMMON.
	m_uploadAllocator->Reset();
	m_uploadCommandList->Reset(m_uploadAllocator, nullptr);

	D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
	dstLoc.pResource        = dest;
	dstLoc.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLoc.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
	srcLoc.pResource       = uploadBuffer.Get();
	srcLoc.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLoc.PlacedFootprint = footprint;

	m_uploadCommandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		dest, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	m_uploadCommandList->ResourceBarrier(1, &barrier);

	m_uploadCommandList->Close();

	ID3D12CommandList* lists[] = { m_uploadCommandList };
	m_directQueue->ExecuteAndSignal(1, lists);
	m_directQueue->Flush();
}

// -----------------------------------------------------------------------
// CreateBuffer

GfxBuffer* DX12Device::CreateBuffer(const BufferDesc& desc)
{
	const bool isDynamic = (desc.usage == BufferUsage::Dynamic);

	// Constant buffers must have their size aligned to 256 bytes on DX12.
	const uint32_t alignedSize = desc.isConstant
		? (desc.sizeBytes + 255u) & ~255u
		: desc.sizeBytes;

	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	if(desc.allowUAV) flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CD3DX12_HEAP_PROPERTIES heapProps(isDynamic ? D3D12_HEAP_TYPE_UPLOAD
	                                             : D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize, flags);

	// Dynamic (upload-heap) buffers require GENERIC_READ as initial state.
	// Static buffers start in COPY_DEST when data is provided, COMMON otherwise.
	D3D12_RESOURCE_STATES initialState = isDynamic
		? D3D12_RESOURCE_STATE_GENERIC_READ
		: (desc.initialData ? D3D12_RESOURCE_STATE_COPY_DEST
		                    : D3D12_RESOURCE_STATE_COMMON);

	ID3D12Resource* resource = nullptr;
	HRESULT hr = m_device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, initialState,
		nullptr, IID_PPV_ARGS(&resource));
	if(FAILED(hr)) return nullptr;

	// Upload initial data into static buffers; leave in COMMON afterward so
	// the DX12 runtime can promote to VB/IB/SRV state on first GPU access.
	if(!isDynamic && desc.initialData)
	{
		UploadBufferSync(resource, desc.initialData, alignedSize,
		                 D3D12_RESOURCE_STATE_COMMON);
		initialState = D3D12_RESOURCE_STATE_COMMON;
	}

	DX12Buffer* buffer      = new DX12Buffer();
	buffer->resource        = resource;
	buffer->currentState    = initialState;
	buffer->sizeBytes       = alignedSize;
	buffer->structureStride = desc.structureStrideBytes;
	return buffer;
}

// -----------------------------------------------------------------------
// CreateTexture

GfxTexture* DX12Device::CreateTexture(const TextureDesc& desc)
{
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	if(desc.allowRTV) flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if(desc.allowDSV) flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if(desc.allowUAV) flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	// Pure depth buffers that are never sampled save bandwidth with this flag.
	if(desc.allowDSV && !desc.allowSRV)
		flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	// Compute the full mip chain if the caller passes 0.
	uint32_t mipLevels = desc.mipLevels;
	if(mipLevels == 0)
	{
		uint32_t maxDim = max(desc.width, desc.height);
		mipLevels = 1;
		while(maxDim > 1) { maxDim >>= 1; ++mipLevels; }
	}

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension        =
		(desc.dimension == TextureDimension::Tex3D)
		? D3D12_RESOURCE_DIMENSION_TEXTURE3D
		: D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment        = 0;
	resourceDesc.Width            = desc.width;
	resourceDesc.Height           = desc.height;
	resourceDesc.DepthOrArraySize = static_cast<UINT16>(
		desc.dimension == TextureDimension::CubeMap ? 6u : max(1u, desc.depth));
	resourceDesc.MipLevels        = static_cast<UINT16>(mipLevels);
	resourceDesc.Format           = desc.format;
	resourceDesc.SampleDesc.Count   = max(1u, desc.msaaSamples);
	resourceDesc.SampleDesc.Quality = desc.msaaQuality;
	resourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags            = flags;

	// Choose initial state: COPY_DEST if we are about to upload data;
	// otherwise the natural state for RTV/DSV targets.
	D3D12_RESOURCE_STATES initialState;
	if(desc.initialData && !desc.allowDSV && !desc.allowRTV)
		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
	else if(desc.allowDSV)
		initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	else if(desc.allowRTV)
		initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	else
		initialState = D3D12_RESOURCE_STATE_COMMON;

	// Provide optimized clear values for RTV and DSV resources to help the driver.
	D3D12_CLEAR_VALUE  clearValue    = {};
	D3D12_CLEAR_VALUE* clearValuePtr = nullptr;
	if(desc.allowRTV)
	{
		clearValue.Format   = desc.format;
		clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = clearValue.Color[3] = 0.0f;
		clearValuePtr = &clearValue;
	}
	else if(desc.allowDSV)
	{
		// DX12 requires a concrete (non-typeless) format for the optimized clear value.
		DXGI_FORMAT dsvFormat = desc.format;
		switch(desc.format)
		{
		case DXGI_FORMAT_R32_TYPELESS:       dsvFormat = DXGI_FORMAT_D32_FLOAT;              break;
		case DXGI_FORMAT_R24G8_TYPELESS:     dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;      break;
		case DXGI_FORMAT_R16_TYPELESS:       dsvFormat = DXGI_FORMAT_D16_UNORM;              break;
		case DXGI_FORMAT_R32G8X24_TYPELESS:  dsvFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;   break;
		default: break;
		}
		clearValue.Format               = dsvFormat;
		clearValue.DepthStencil.Depth   = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
		clearValuePtr = &clearValue;
	}

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	ID3D12Resource* resource = nullptr;
	HRESULT hr = m_device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, initialState,
		clearValuePtr, IID_PPV_ARGS(&resource));
	if(FAILED(hr)) return nullptr;

	// Upload mip 0 subresource data for plain shader-read textures.
	if(desc.initialData && !desc.allowDSV && !desc.allowRTV)
	{
		UploadTextureSync(resource, resourceDesc, desc.initialData);
		initialState = D3D12_RESOURCE_STATE_COMMON;
	}

	DX12Texture* texture  = new DX12Texture();
	texture->resource     = resource;
	texture->currentState = initialState;
	texture->width        = desc.width;
	texture->height       = desc.height;
	texture->depth        = max(1u, desc.depth);
	texture->mipLevels    = mipLevels;
	texture->format       = desc.format;
	texture->dimension    = desc.dimension;
	return texture;
}

// -----------------------------------------------------------------------
// LoadDDSFromMemory — parse DDS, create DX12 texture + SRV with all mips

GfxSRV* DX12Device::LoadDDSFromMemory(const uint8_t* data, uint32_t dataSize)
{
	using namespace DirectX;

	TexMetadata metadata;
	ScratchImage scratchImage;
	HRESULT hr = LoadFromDDSMemory(data, dataSize, DDS_FLAGS_NONE, &metadata, scratchImage);
	if(FAILED(hr))
		return nullptr;

	// Build resource desc from DDS metadata
	D3D12_RESOURCE_DESC resourceDesc = {};
	switch(metadata.dimension)
	{
	case TEX_DIMENSION_TEXTURE1D: resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D; break;
	case TEX_DIMENSION_TEXTURE2D: resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; break;
	case TEX_DIMENSION_TEXTURE3D: resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D; break;
	default: return nullptr;
	}
	resourceDesc.Width            = static_cast<UINT64>(metadata.width);
	resourceDesc.Height           = static_cast<UINT>(metadata.height);
	resourceDesc.DepthOrArraySize = static_cast<UINT16>(
		metadata.dimension == TEX_DIMENSION_TEXTURE3D ? metadata.depth : metadata.arraySize);
	resourceDesc.MipLevels        = static_cast<UINT16>(metadata.mipLevels);
	resourceDesc.Format           = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	ID3D12Resource* resource = nullptr;
	hr = m_device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr, IID_PPV_ARGS(&resource));
	if(FAILED(hr))
		return nullptr;

	// Compute layouts for all subresources
	const uint32_t subresourceCount = static_cast<uint32_t>(metadata.mipLevels * metadata.arraySize);
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(subresourceCount);
	std::vector<UINT>   numRows(subresourceCount);
	std::vector<UINT64> rowSizes(subresourceCount);
	UINT64 totalUploadSize = 0;
	m_device->GetCopyableFootprints(&resourceDesc, 0, subresourceCount, 0,
		layouts.data(), numRows.data(), rowSizes.data(), &totalUploadSize);

	// Create upload buffer
	ComPtr<ID3D12Resource> uploadBuffer;
	CD3DX12_HEAP_PROPERTIES uploadProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC   uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(totalUploadSize);
	hr = m_device->CreateCommittedResource(
		&uploadProps, D3D12_HEAP_FLAG_NONE,
		&uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&uploadBuffer));
	if(FAILED(hr))
	{
		resource->Release();
		return nullptr;
	}

	// Map upload buffer and copy all subresource data
	BYTE* mappedData = nullptr;
	D3D12_RANGE readRange = {0, 0};
	uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));

	for(uint32_t i = 0; i < subresourceCount; ++i)
	{
		const uint32_t mipLevel   = i % static_cast<uint32_t>(metadata.mipLevels);
		const uint32_t arraySlice = i / static_cast<uint32_t>(metadata.mipLevels);

		const Image* img = scratchImage.GetImage(mipLevel, arraySlice, 0);
		if(!img) continue;

		BYTE*       dst = mappedData + layouts[i].Offset;
		const BYTE* src = img->pixels;
		for(UINT row = 0; row < numRows[i]; ++row)
		{
			memcpy(dst, src, static_cast<size_t>(rowSizes[i]));
			src += img->rowPitch;
			dst += layouts[i].Footprint.RowPitch;
		}
	}

	uploadBuffer->Unmap(0, nullptr);

	// Record copy commands for all subresources
	m_uploadAllocator->Reset();
	m_uploadCommandList->Reset(m_uploadAllocator, nullptr);

	for(uint32_t i = 0; i < subresourceCount; ++i)
	{
		D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
		dstLoc.pResource        = resource;
		dstLoc.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLoc.SubresourceIndex = i;

		D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
		srcLoc.pResource       = uploadBuffer.Get();
		srcLoc.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLoc.PlacedFootprint = layouts[i];

		m_uploadCommandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
	}

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	m_uploadCommandList->ResourceBarrier(1, &barrier);

	m_uploadCommandList->Close();
	ID3D12CommandList* lists[] = { m_uploadCommandList };
	m_directQueue->ExecuteAndSignal(1, lists);
	m_directQueue->Flush();

	// Create DX12Texture wrapper
	DX12Texture* texture  = new DX12Texture();
	texture->resource     = resource;
	texture->currentState = D3D12_RESOURCE_STATE_COMMON;
	texture->width        = static_cast<uint32_t>(metadata.width);
	texture->height       = static_cast<uint32_t>(metadata.height);
	texture->depth        = max(1u, static_cast<uint32_t>(metadata.depth));
	texture->mipLevels    = static_cast<uint32_t>(metadata.mipLevels);
	texture->format       = metadata.format;
	if(metadata.IsCubemap())
		texture->dimension = TextureDimension::CubeMap;
	else if(metadata.dimension == TEX_DIMENSION_TEXTURE3D)
		texture->dimension = TextureDimension::Tex3D;
	else
		texture->dimension = TextureDimension::Tex2D;

	// Create SRV descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format                  = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if(metadata.IsCubemap())
	{
		srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip     = 0;
		srvDesc.TextureCube.MipLevels           = static_cast<UINT>(metadata.mipLevels);
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else if(metadata.dimension == TEX_DIMENSION_TEXTURE3D)
	{
		srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip     = 0;
		srvDesc.Texture3D.MipLevels           = static_cast<UINT>(metadata.mipLevels);
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	}
	else if(metadata.arraySize > 1)
	{
		srvDesc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip       = 0;
		srvDesc.Texture2DArray.MipLevels             = static_cast<UINT>(metadata.mipLevels);
		srvDesc.Texture2DArray.FirstArraySlice       = 0;
		srvDesc.Texture2DArray.ArraySize             = static_cast<UINT>(metadata.arraySize);
		srvDesc.Texture2DArray.PlaneSlice            = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp   = 0.0f;
	}
	else
	{
		srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip     = 0;
		srvDesc.Texture2D.MipLevels           = static_cast<UINT>(metadata.mipLevels);
		srvDesc.Texture2D.PlaneSlice          = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}

	DX12SRV* srv = new DX12SRV();
	srv->heapIndex     = AllocateCbvSrvUavSlot(srv->cpuHandle, srv->gpuHandle);
	srv->ownedTexture  = texture;
	m_device->CreateShaderResourceView(resource, &srvDesc, srv->cpuHandle);
	return srv;
}

// -----------------------------------------------------------------------
// CreateRTV

GfxRTV* DX12Device::CreateRTV(GfxTexture* texture, DXGI_FORMAT format,
                               uint32_t mipSlice, uint32_t arraySlice)
{
	auto* tex = Cast(texture);
	const uint32_t sampleCount = tex->resource->GetDesc().SampleDesc.Count;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = format;

	switch(tex->dimension)
	{
	case TextureDimension::Tex2D:
		if(sampleCount > 1)
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			rtvDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice   = mipSlice;
			rtvDesc.Texture2D.PlaneSlice = 0;
		}
		break;
	case TextureDimension::Tex3D:
		rtvDesc.ViewDimension            = D3D12_RTV_DIMENSION_TEXTURE3D;
		rtvDesc.Texture3D.MipSlice       = mipSlice;
		rtvDesc.Texture3D.FirstWSlice    = arraySlice;
		rtvDesc.Texture3D.WSize          = 1;
		break;
	case TextureDimension::CubeMap:
		rtvDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice          = mipSlice;
		rtvDesc.Texture2DArray.FirstArraySlice   = arraySlice;
		rtvDesc.Texture2DArray.ArraySize         = 1;
		rtvDesc.Texture2DArray.PlaneSlice        = 0;
		break;
	}

	DX12RTV* rtv = new DX12RTV();
	rtv->handle = AllocateRtvHandle();
	m_device->CreateRenderTargetView(tex->resource, &rtvDesc, rtv->handle);
	return rtv;
}

// -----------------------------------------------------------------------
// CreateDSV

GfxDSV* DX12Device::CreateDSV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mipSlice)
{
	auto* tex = Cast(texture);
	const uint32_t sampleCount = tex->resource->GetDesc().SampleDesc.Count;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = format;
	dsvDesc.Flags  = D3D12_DSV_FLAG_NONE;

	if(sampleCount > 1)
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		dsvDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = mipSlice;
	}

	DX12DSV* dsv = new DX12DSV();
	dsv->handle = AllocateDsvHandle();
	m_device->CreateDepthStencilView(tex->resource, &dsvDesc, dsv->handle);
	return dsv;
}

// -----------------------------------------------------------------------
// CreateSRV (texture)

GfxSRV* DX12Device::CreateSRV(GfxTexture* texture, DXGI_FORMAT format,
                                uint32_t mostDetailedMip, uint32_t mipLevels)
{
	auto* tex = Cast(texture);

	const uint32_t effectiveMips = (mipLevels == UINT32_MAX)
		? (tex->mipLevels - mostDetailedMip)
		: mipLevels;

	const uint32_t sampleCount = tex->resource->GetDesc().SampleDesc.Count;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format                  = format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch(tex->dimension)
	{
	case TextureDimension::Tex2D:
		if(sampleCount > 1)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip     = mostDetailedMip;
			srvDesc.Texture2D.MipLevels           = effectiveMips;
			srvDesc.Texture2D.PlaneSlice          = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		}
		break;
	case TextureDimension::Tex3D:
		srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip     = mostDetailedMip;
		srvDesc.Texture3D.MipLevels           = effectiveMips;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		break;
	case TextureDimension::CubeMap:
		srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip     = mostDetailedMip;
		srvDesc.TextureCube.MipLevels           = effectiveMips;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	}

	DX12SRV* srv = new DX12SRV();
	srv->heapIndex = AllocateCbvSrvUavSlot(srv->cpuHandle, srv->gpuHandle);
	m_device->CreateShaderResourceView(tex->resource, &srvDesc, srv->cpuHandle);
	return srv;
}

// -----------------------------------------------------------------------
// CreateSRV (structured buffer)

GfxSRV* DX12Device::CreateSRV(GfxBuffer* buffer, uint32_t firstElement,
                                uint32_t numElements)
{
	auto* buf = Cast(buffer);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format                     = DXGI_FORMAT_UNKNOWN; // structured buffers use UNKNOWN
	srvDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement        = firstElement;
	srvDesc.Buffer.NumElements         = numElements;
	srvDesc.Buffer.StructureByteStride = buf->structureStride;
	srvDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;

	DX12SRV* srv = new DX12SRV();
	srv->heapIndex = AllocateCbvSrvUavSlot(srv->cpuHandle, srv->gpuHandle);
	m_device->CreateShaderResourceView(buf->resource, &srvDesc, srv->cpuHandle);
	return srv;
}

// -----------------------------------------------------------------------
// CreateUAV (texture)

GfxUAV* DX12Device::CreateUAV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mipSlice)
{
	auto* tex = Cast(texture);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;

	switch(tex->dimension)
	{
	case TextureDimension::Tex2D:
		uavDesc.ViewDimension            = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice       = mipSlice;
		uavDesc.Texture2D.PlaneSlice     = 0;
		break;
	case TextureDimension::Tex3D:
		uavDesc.ViewDimension             = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.MipSlice        = mipSlice;
		uavDesc.Texture3D.FirstWSlice     = 0;
		uavDesc.Texture3D.WSize           = tex->depth;
		break;
	case TextureDimension::CubeMap:
		uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.MipSlice          = mipSlice;
		uavDesc.Texture2DArray.FirstArraySlice   = 0;
		uavDesc.Texture2DArray.ArraySize         = 6;
		uavDesc.Texture2DArray.PlaneSlice        = 0;
		break;
	}

	DX12UAV* uav = new DX12UAV();
	uav->heapIndex = AllocateCbvSrvUavSlot(uav->cpuHandle, uav->gpuHandle);
	// pCounterResource = nullptr: no append/consume counter.
	m_device->CreateUnorderedAccessView(tex->resource, nullptr, &uavDesc, uav->cpuHandle);
	return uav;
}

// -----------------------------------------------------------------------
// CreateUAV (structured buffer)

GfxUAV* DX12Device::CreateUAV(GfxBuffer* buffer, uint32_t firstElement,
                                uint32_t numElements)
{
	auto* buf = Cast(buffer);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format                      = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement         = firstElement;
	uavDesc.Buffer.NumElements          = numElements;
	uavDesc.Buffer.StructureByteStride  = buf->structureStride;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_NONE;

	DX12UAV* uav = new DX12UAV();
	uav->heapIndex = AllocateCbvSrvUavSlot(uav->cpuHandle, uav->gpuHandle);
	m_device->CreateUnorderedAccessView(buf->resource, nullptr, &uavDesc, uav->cpuHandle);
	return uav;
}

// -----------------------------------------------------------------------
// CreateSampler
// D3D11 and D3D12 sampler descriptors share the same Filter / AddressMode /
// ComparisonFunc enum values, so a direct static_cast is valid.

GfxSampler* DX12Device::CreateSampler(const SamplerDesc& desc)
{
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter         = static_cast<D3D12_FILTER>(desc.d3d11Desc.Filter);
	samplerDesc.AddressU       = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(desc.d3d11Desc.AddressU);
	samplerDesc.AddressV       = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(desc.d3d11Desc.AddressV);
	samplerDesc.AddressW       = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(desc.d3d11Desc.AddressW);
	samplerDesc.MipLODBias     = desc.d3d11Desc.MipLODBias;
	samplerDesc.MaxAnisotropy  = desc.d3d11Desc.MaxAnisotropy;
	samplerDesc.ComparisonFunc = static_cast<D3D12_COMPARISON_FUNC>(desc.d3d11Desc.ComparisonFunc);
	memcpy(samplerDesc.BorderColor, desc.d3d11Desc.BorderColor, sizeof(samplerDesc.BorderColor));
	samplerDesc.MinLOD = desc.d3d11Desc.MinLOD;
	samplerDesc.MaxLOD = desc.d3d11Desc.MaxLOD;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	AllocateSamplerSlot(cpuHandle, gpuHandle);

	m_device->CreateSampler(&samplerDesc, cpuHandle);

	DX12Sampler* sampler = new DX12Sampler();
	sampler->handle = cpuHandle;
	return sampler;
}

// -----------------------------------------------------------------------
// GetBackBuffer

GfxTexture* DX12Device::GetBackBuffer(uint32_t frameIndex)
{
	if(frameIndex >= DX12SwapChain::BackBufferCount)     return nullptr;
	if(!m_backBufferTextures[frameIndex].resource)       return nullptr;
	return &m_backBufferTextures[frameIndex];
}

// -----------------------------------------------------------------------
// Phase 4 helpers — D3D11 state desc → D3D12 conversions.
// The two APIs share the same enum values for Fill/Cull/Blend/Comparison so
// direct static_cast is safe throughout.

static void ConvertBlendDesc(const D3D11_BLEND_DESC& src, D3D12_BLEND_DESC& dst)
{
	dst.AlphaToCoverageEnable  = src.AlphaToCoverageEnable;
	dst.IndependentBlendEnable = src.IndependentBlendEnable;
	for(uint32_t i = 0; i < 8; ++i)
	{
		const auto& s = src.RenderTarget[i];
		auto&       d = dst.RenderTarget[i];
		d.BlendEnable           = s.BlendEnable;
		d.LogicOpEnable         = FALSE;
		// DX12 validates blend enum values even when BlendEnable=FALSE.
		// DX11 allows 0 (uninitialized) when blend is disabled; DX12 does not.
		d.SrcBlend              = s.SrcBlend      ? static_cast<D3D12_BLEND>(s.SrcBlend)      : D3D12_BLEND_ONE;
		d.DestBlend             = s.DestBlend     ? static_cast<D3D12_BLEND>(s.DestBlend)     : D3D12_BLEND_ZERO;
		d.BlendOp               = s.BlendOp       ? static_cast<D3D12_BLEND_OP>(s.BlendOp)    : D3D12_BLEND_OP_ADD;
		d.SrcBlendAlpha         = s.SrcBlendAlpha ? static_cast<D3D12_BLEND>(s.SrcBlendAlpha) : D3D12_BLEND_ONE;
		d.DestBlendAlpha        = s.DestBlendAlpha? static_cast<D3D12_BLEND>(s.DestBlendAlpha): D3D12_BLEND_ZERO;
		d.BlendOpAlpha          = s.BlendOpAlpha  ? static_cast<D3D12_BLEND_OP>(s.BlendOpAlpha): D3D12_BLEND_OP_ADD;
		d.LogicOp               = D3D12_LOGIC_OP_NOOP;
		d.RenderTargetWriteMask = s.RenderTargetWriteMask;
	}
}

static void ConvertDepthStencilDesc(const D3D11_DEPTH_STENCIL_DESC& src,
                                    D3D12_DEPTH_STENCIL_DESC& dst)
{
	dst.DepthEnable    = src.DepthEnable;
	dst.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(src.DepthWriteMask);
	// DX12 requires valid comparison func even when DepthEnable=FALSE.
	dst.DepthFunc      = src.DepthFunc ? static_cast<D3D12_COMPARISON_FUNC>(src.DepthFunc) : D3D12_COMPARISON_FUNC_LESS;
	dst.StencilEnable  = src.StencilEnable;
	dst.StencilReadMask  = src.StencilReadMask;
	dst.StencilWriteMask = src.StencilWriteMask;

	auto convertOp = [](const D3D11_DEPTH_STENCILOP_DESC& s,
	                    D3D12_DEPTH_STENCILOP_DESC& d)
	{
		d.StencilFailOp      = s.StencilFailOp      ? static_cast<D3D12_STENCIL_OP>(s.StencilFailOp)      : D3D12_STENCIL_OP_KEEP;
		d.StencilDepthFailOp = s.StencilDepthFailOp ? static_cast<D3D12_STENCIL_OP>(s.StencilDepthFailOp) : D3D12_STENCIL_OP_KEEP;
		d.StencilPassOp      = s.StencilPassOp      ? static_cast<D3D12_STENCIL_OP>(s.StencilPassOp)      : D3D12_STENCIL_OP_KEEP;
		d.StencilFunc        = s.StencilFunc         ? static_cast<D3D12_COMPARISON_FUNC>(s.StencilFunc)   : D3D12_COMPARISON_FUNC_ALWAYS;
	};
	convertOp(src.FrontFace, dst.FrontFace);
	convertOp(src.BackFace,  dst.BackFace);
}

static void ConvertRasterizerDesc(const D3D11_RASTERIZER_DESC& src,
                                  D3D12_RASTERIZER_DESC& dst)
{
	dst.FillMode              = static_cast<D3D12_FILL_MODE>(src.FillMode);
	dst.CullMode              = static_cast<D3D12_CULL_MODE>(src.CullMode);
	dst.FrontCounterClockwise = src.FrontCounterClockwise;
	dst.DepthBias             = src.DepthBias;
	dst.DepthBiasClamp        = src.DepthBiasClamp;
	dst.SlopeScaledDepthBias  = src.SlopeScaledDepthBias;
	dst.DepthClipEnable       = src.DepthClipEnable;
	dst.MultisampleEnable     = src.MultisampleEnable;
	dst.AntialiasedLineEnable = src.AntialiasedLineEnable;
	dst.ForcedSampleCount     = 0;
	dst.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertTopologyType(Topology topo)
{
	switch(topo)
	{
	case Topology::PointList:    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case Topology::LineList:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case Topology::Patch3:       return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	default:                     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
}

static DXGI_FORMAT GetInputFormat(D3D_REGISTER_COMPONENT_TYPE component, BYTE mask)
{
	if(component == D3D_REGISTER_COMPONENT_FLOAT32)
	{
		switch(mask)
		{
		case 0x01: return DXGI_FORMAT_R32_FLOAT;
		case 0x03: return DXGI_FORMAT_R32G32_FLOAT;
		case 0x07: return DXGI_FORMAT_R32G32B32_FLOAT;
		case 0x0f: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
	}
	else if(component == D3D_REGISTER_COMPONENT_UINT32)
	{
		switch(mask)
		{
		case 0x01: return DXGI_FORMAT_R32_UINT;
		case 0x03: return DXGI_FORMAT_R32G32_UINT;
		case 0x07: return DXGI_FORMAT_R32G32B32_UINT;
		case 0x0f: return DXGI_FORMAT_R32G32B32A32_UINT;
		}
	}
	else if(component == D3D_REGISTER_COMPONENT_SINT32)
	{
		switch(mask)
		{
		case 0x01: return DXGI_FORMAT_R32_SINT;
		case 0x03: return DXGI_FORMAT_R32G32_SINT;
		case 0x07: return DXGI_FORMAT_R32G32B32_SINT;
		case 0x0f: return DXGI_FORMAT_R32G32B32A32_SINT;
		}
	}
	return DXGI_FORMAT_UNKNOWN;
}

// -----------------------------------------------------------------------
// Private: CreateRootSignatures
// Creates separate graphics and compute root signatures.
//
// Graphics root signature (19 params, 36 DWORDs of 64 limit):
//   [0-3]   Root CBV  VS b0-b3   (VERTEX visibility)
//   [4-13]  Root CBV  PS b0-b9   (PIXEL visibility)
//   [14]    Root CBV  GS b0      (GEOMETRY visibility)
//   [15]    Root CBV  HS b0      (HULL visibility)
//   [16]    Root CBV  DS b0      (DOMAIN visibility)
//   [17]    SRV table t0-t127    (ALL visibility)
//   [18]    UAV table u0-u31     (ALL visibility)
//   Static samplers s0-s7.
//
// Compute root signature (6 params, 10 DWORDs):
//   [0-3]   Root CBV  CS b0-b3   (ALL visibility — compute has no stage distinction)
//   [4]     SRV table t0-t127    (ALL visibility)
//   [5]     UAV table u0-u31     (ALL visibility)
//   Static samplers s0-s7.

bool DX12Device::CreateRootSignatures()
{
	// Standard static samplers — register slots s0-s7, space0.
	D3D12_STATIC_SAMPLER_DESC samplers[8] = {};

	auto fillSampler = [](D3D12_STATIC_SAMPLER_DESC& s,
	                      uint32_t reg,
	                      D3D12_FILTER filter,
	                      D3D12_TEXTURE_ADDRESS_MODE address,
	                      D3D12_COMPARISON_FUNC comparison = D3D12_COMPARISON_FUNC_ALWAYS)
	{
		s.Filter           = filter;
		s.AddressU         = s.AddressV = s.AddressW = address;
		s.MipLODBias       = 0.0f;
		s.MaxAnisotropy    = (filter == D3D12_FILTER_ANISOTROPIC) ? 16u : 1u;
		s.ComparisonFunc   = comparison;
		s.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		s.MinLOD           = 0.0f;
		s.MaxLOD           = D3D12_FLOAT32_MAX;
		s.ShaderRegister   = reg;
		s.RegisterSpace    = 0;
		s.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	};

	fillSampler(samplers[0], 0, D3D12_FILTER_MIN_MAG_MIP_POINT,    D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	fillSampler(samplers[1], 1, D3D12_FILTER_MIN_MAG_MIP_POINT,    D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	fillSampler(samplers[2], 2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,   D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	fillSampler(samplers[3], 3, D3D12_FILTER_MIN_MAG_MIP_LINEAR,   D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	fillSampler(samplers[4], 4, D3D12_FILTER_ANISOTROPIC,          D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	fillSampler(samplers[5], 5, D3D12_FILTER_ANISOTROPIC,          D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	fillSampler(samplers[6], 6, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
	            D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_COMPARISON_FUNC_LESS_EQUAL);
	fillSampler(samplers[7], 7, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
	            D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_COMPARISON_FUNC_GREATER_EQUAL);

	// Helper to serialize + create a root signature.
	auto createRS = [&](uint32_t paramCount, CD3DX12_ROOT_PARAMETER1* params,
	                    D3D12_ROOT_SIGNATURE_FLAGS flags,
	                    ID3D12RootSignature** outRS) -> bool
	{
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
		rsDesc.Init_1_1(paramCount, params, 8, samplers, flags);

		ComPtr<ID3DBlob> serialized, error;
		HRESULT hr = D3DX12SerializeVersionedRootSignature(
			&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &serialized, &error);
		if(FAILED(hr))
		{
			if(error) LOG("Root signature serialization error: %s",
			              (const char*)error->GetBufferPointer());
			return false;
		}
		hr = m_device->CreateRootSignature(
			0, serialized->GetBufferPointer(), serialized->GetBufferSize(),
			IID_PPV_ARGS(outRS));
		return SUCCEEDED(hr);
	};

	// --- Graphics root signature ---
	{
		CD3DX12_ROOT_PARAMETER1 params[GfxRP_Count];

		// VS root CBVs (b0-b3, VERTEX visibility)
		for(uint32_t i = 0; i < GfxRP_VS_CBV_Count; ++i)
			params[GfxRP_VS_CBV_Base + i].InitAsConstantBufferView(
				i, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
				D3D12_SHADER_VISIBILITY_VERTEX);

		// PS root CBVs (b0-b9, PIXEL visibility)
		for(uint32_t i = 0; i < GfxRP_PS_CBV_Count; ++i)
			params[GfxRP_PS_CBV_Base + i].InitAsConstantBufferView(
				i, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
				D3D12_SHADER_VISIBILITY_PIXEL);

		// GS root CBV (b0, GEOMETRY)
		params[GfxRP_GS_CBV].InitAsConstantBufferView(
			0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_GEOMETRY);

		// HS root CBV (b0, HULL)
		params[GfxRP_HS_CBV].InitAsConstantBufferView(
			0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_HULL);

		// DS root CBV (b0, DOMAIN)
		params[GfxRP_DS_CBV].InitAsConstantBufferView(
			0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_DOMAIN);

		// SRV descriptor table (t0-t127, ALL)
		CD3DX12_DESCRIPTOR_RANGE1 srvRange;
		srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 128, 0, 0,
		              D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
		              | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
		params[GfxRP_SRV_Table].InitAsDescriptorTable(1, &srvRange,
			D3D12_SHADER_VISIBILITY_ALL);

		// UAV descriptor table (u0-u31, ALL)
		CD3DX12_DESCRIPTOR_RANGE1 uavRange;
		uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 32, 0, 0,
		              D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
		              | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
		params[GfxRP_UAV_Table].InitAsDescriptorTable(1, &uavRange,
			D3D12_SHADER_VISIBILITY_ALL);

		if(!createRS(GfxRP_Count, params,
		             D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
		             &m_graphicsRootSignature))
			return false;
	}

	// --- Compute root signature ---
	{
		CD3DX12_ROOT_PARAMETER1 params[CsRP_Count];

		// CS root CBVs (b0-b3, ALL — compute only has one stage)
		for(uint32_t i = 0; i < CsRP_CBV_Count; ++i)
			params[CsRP_CBV_Base + i].InitAsConstantBufferView(
				i, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
				D3D12_SHADER_VISIBILITY_ALL);

		// SRV descriptor table (t0-t127, ALL)
		CD3DX12_DESCRIPTOR_RANGE1 srvRange;
		srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 128, 0, 0,
		              D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
		              | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
		params[CsRP_SRV_Table].InitAsDescriptorTable(1, &srvRange,
			D3D12_SHADER_VISIBILITY_ALL);

		// UAV descriptor table (u0-u31, ALL)
		CD3DX12_DESCRIPTOR_RANGE1 uavRange;
		uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 32, 0, 0,
		              D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
		              | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
		params[CsRP_UAV_Table].InitAsDescriptorTable(1, &uavRange,
			D3D12_SHADER_VISIBILITY_ALL);

		if(!createRS(CsRP_Count, params,
		             D3D12_ROOT_SIGNATURE_FLAG_NONE,
		             &m_computeRootSignature))
			return false;
	}

	return true;
}

// -----------------------------------------------------------------------
// CreateGraphicsPSO
// Converts a GraphicsPSODesc (D3D11-centric) into a DX12 graphics PSO.
// DXBC shader bytecodes are read from ShaderCodeMgr::GetShaderBytecode().

GfxPipelineState* DX12Device::CreateGraphicsPSO(const GraphicsPSODesc& desc)
{
	if(!m_graphicsRootSignature || !m_device) return nullptr;

	const auto& vsBytecode = ShaderCodeMgr::GetShaderBytecode(desc.vertexShaderID);
	if(vsBytecode.empty()) return nullptr;

	const auto& psBytecode = ShaderCodeMgr::GetShaderBytecode(desc.pixelShaderID);
	const auto& hsBytecode = ShaderCodeMgr::GetShaderBytecode(desc.hullShaderID);
	const auto& dsBytecode = ShaderCodeMgr::GetShaderBytecode(desc.domainShaderID);
	const auto& gsBytecode = ShaderCodeMgr::GetShaderBytecode(desc.geometryShaderID);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_graphicsRootSignature;

	psoDesc.VS = { vsBytecode.data(), vsBytecode.size() };
	if(!psBytecode.empty()) psoDesc.PS = { psBytecode.data(), psBytecode.size() };
	if(!hsBytecode.empty()) psoDesc.HS = { hsBytecode.data(), hsBytecode.size() };
	if(!dsBytecode.empty()) psoDesc.DS = { dsBytecode.data(), dsBytecode.size() };
	if(!gsBytecode.empty()) psoDesc.GS = { gsBytecode.data(), gsBytecode.size() };

	// Blend state
	ConvertBlendDesc(desc.blendDesc, psoDesc.BlendState);

	// Depth-stencil state
	ConvertDepthStencilDesc(desc.depthStencilDesc, psoDesc.DepthStencilState);

	// Rasterizer state
	ConvertRasterizerDesc(desc.rasterizerDesc, psoDesc.RasterizerState);

	// Input layout — reflect from VS bytecode so we don't need to store
	// element descs separately from the DX11 ID3D11InputLayout.
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
	ID3D11ShaderReflection* vsReflector = nullptr;
	D3DReflect(vsBytecode.data(), vsBytecode.size(),
	           IID_ID3D11ShaderReflection, (void**)&vsReflector);
	if(vsReflector)
	{
		D3D11_SHADER_DESC shaderDesc = {};
		if(SUCCEEDED(vsReflector->GetDesc(&shaderDesc)))
		{
			inputElements.reserve(shaderDesc.InputParameters);
			for(uint32_t i = 0; i < shaderDesc.InputParameters; ++i)
			{
				D3D11_SIGNATURE_PARAMETER_DESC paramDesc = {};
				if(FAILED(vsReflector->GetInputParameterDesc(i, &paramDesc)))
					continue;
				D3D12_INPUT_ELEMENT_DESC elem = {};
				elem.SemanticName         = paramDesc.SemanticName; // valid while reflector lives
				elem.SemanticIndex        = paramDesc.SemanticIndex;
				elem.Format               = GetInputFormat(paramDesc.ComponentType, paramDesc.Mask);
				elem.InputSlot            = 0;
				elem.AlignedByteOffset    = D3D12_APPEND_ALIGNED_ELEMENT;
				elem.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
				elem.InstanceDataStepRate = 0;
				inputElements.push_back(elem);
			}
		}
		// Release happens AFTER CreateGraphicsPipelineState so semantic name
		// pointers remain valid during PSO creation.
	}
	psoDesc.InputLayout = { inputElements.data(),
	                        static_cast<uint32_t>(inputElements.size()) };

	// Render targets and depth-stencil formats.
	// If the caller didn't specify formats (common for DX11-era code), use
	// sensible defaults so the PSO actually produces visible output.
	if(desc.renderTargetCount > 0)
	{
		psoDesc.NumRenderTargets = desc.renderTargetCount;
		for(uint32_t i = 0; i < desc.renderTargetCount; ++i)
			psoDesc.RTVFormats[i] = desc.renderTargetFormats[i];
	}
	else if(!psBytecode.empty())
	{
		// Has a pixel shader but no RT format specified — default to back buffer format.
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0]    = DX12SwapChain::BackBufferFormat;
	}
	psoDesc.DSVFormat = desc.depthStencilFormat;

	// Topology and sample state
	psoDesc.PrimitiveTopologyType = ConvertTopologyType(desc.topology);
	psoDesc.SampleMask            = UINT_MAX;
	psoDesc.SampleDesc.Count      = desc.msaaSamples ? desc.msaaSamples : 1;
	psoDesc.SampleDesc.Quality    = desc.msaaQuality;

	ID3D12PipelineState* pso = nullptr;
	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

	if(vsReflector) vsReflector->Release();

	if(FAILED(hr))
	{
		ERR("DX12 CreateGraphicsPipelineState failed (HRESULT 0x%08X) VS=%u PS=%u RTs=%u RTFmt=%u DSFmt=%u Topo=%u "
		    "DepthEnable=%u Fill=%u Cull=%u BlendEnable=%u WriteMask=0x%X InputElems=%u",
		    static_cast<unsigned>(hr), desc.vertexShaderID, desc.pixelShaderID,
		    psoDesc.NumRenderTargets, psoDesc.RTVFormats[0], psoDesc.DSVFormat,
		    static_cast<unsigned>(psoDesc.PrimitiveTopologyType),
		    psoDesc.DepthStencilState.DepthEnable,
		    psoDesc.RasterizerState.FillMode, psoDesc.RasterizerState.CullMode,
		    psoDesc.BlendState.RenderTarget[0].BlendEnable,
		    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask,
		    psoDesc.InputLayout.NumElements);

		// Capture D3D12 debug layer message for detailed diagnostics
		ID3D12InfoQueue* infoQueue = nullptr;
		if(SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			UINT64 messageCount = infoQueue->GetNumStoredMessages();
			for(UINT64 mi = (messageCount > 3 ? messageCount - 3 : 0); mi < messageCount; ++mi)
			{
				SIZE_T messageLength = 0;
				infoQueue->GetMessage(mi, nullptr, &messageLength);
				if(messageLength > 0)
				{
					auto* message = (D3D12_MESSAGE*)malloc(messageLength);
					if(message && SUCCEEDED(infoQueue->GetMessage(mi, message, &messageLength)))
						ERR("  D3D12: %s", message->pDescription);
					free(message);
				}
			}
			infoQueue->Release();
		}
		return nullptr;
	}

	DX12PipelineState* dx12pso = new DX12PipelineState();
	dx12pso->pso           = pso;
	dx12pso->rootSignature = m_graphicsRootSignature;
	m_graphicsRootSignature->AddRef(); // each PSO holds its own reference
	dx12pso->isCompute     = false;
	return dx12pso;
}

// -----------------------------------------------------------------------
// CreateComputePSO

GfxPipelineState* DX12Device::CreateComputePSO(const ComputePSODesc& desc)
{
	if(!m_computeRootSignature || !m_device) return nullptr;

	const auto& csBytecode = ShaderCodeMgr::GetShaderBytecode(desc.computeShaderID);
	if(csBytecode.empty()) return nullptr;

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_computeRootSignature;
	psoDesc.CS             = { csBytecode.data(), csBytecode.size() };

	ID3D12PipelineState* pso = nullptr;
	HRESULT hr = m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	if(FAILED(hr)) return nullptr;

	DX12PipelineState* dx12pso = new DX12PipelineState();
	dx12pso->pso           = pso;
	dx12pso->rootSignature = m_computeRootSignature;
	m_computeRootSignature->AddRef();
	dx12pso->isCompute     = true;
	return dx12pso;
}

// -----------------------------------------------------------------------
// CreateMeshShaderPSO (Phase 7)
// Creates a pipeline state object for the amplification+mesh+pixel shader
// pipeline using D3D12_PIPELINE_STATE_STREAM_DESC.  The subobject types
// AS (24) and MS (25) are defined in d3d12.h but not yet templated in the
// project's d3dx12.h, so we define local aliases using the existing
// CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<> template.

GfxPipelineState* DX12Device::CreateMeshShaderPSO(const MeshShaderPSODesc& desc)
{
	if(!m_supportsMeshShaders || !m_graphicsRootSignature || !m_device) return nullptr;

	const auto& msBytecode = ShaderCodeMgr::GetShaderBytecode(desc.meshShaderID);
	if(msBytecode.empty()) return nullptr;

	// Local typedef — d3dx12.h already has the template; just provide
	// the subobject type values for AS and MS that the file lacks typedefs for.
	using StreamAS = CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<
	    D3D12_SHADER_BYTECODE,
	    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>;
	using StreamMS = CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<
	    D3D12_SHADER_BYTECODE,
	    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>;

	struct MeshShaderStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        rootSignature;
		StreamAS                                            amplificationShader;
		StreamMS                                            meshShader;
		CD3DX12_PIPELINE_STATE_STREAM_PS                    pixelShader;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL         depthStencil;
		CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC            blendDesc;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER            rasterizerDesc;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS renderTargetFormats;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  depthStencilFormat;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           sampleDesc;
	};

	MeshShaderStream stream = {};
	stream.rootSignature = m_graphicsRootSignature;

	// Amplification shader is optional.
	if(desc.amplificationShaderID != SHADER_NULL)
	{
		const auto& asBytecode = ShaderCodeMgr::GetShaderBytecode(desc.amplificationShaderID);
		if(!asBytecode.empty())
			stream.amplificationShader = D3D12_SHADER_BYTECODE{
			    asBytecode.data(), asBytecode.size() };
	}

	stream.meshShader = D3D12_SHADER_BYTECODE{ msBytecode.data(), msBytecode.size() };

	const auto& psBytecode = ShaderCodeMgr::GetShaderBytecode(desc.pixelShaderID);
	if(!psBytecode.empty())
		stream.pixelShader = D3D12_SHADER_BYTECODE{ psBytecode.data(), psBytecode.size() };

	// Convert render state — same helpers used by CreateGraphicsPSO.
	D3D12_BLEND_DESC         blendD3D12        = {};
	D3D12_DEPTH_STENCIL_DESC depthStencilD3D12 = {};
	D3D12_RASTERIZER_DESC    rasterizerD3D12   = {};
	ConvertBlendDesc(desc.blendDesc, blendD3D12);
	ConvertDepthStencilDesc(desc.depthStencilDesc, depthStencilD3D12);
	ConvertRasterizerDesc(desc.rasterizerDesc, rasterizerD3D12);

	// CD3DX12_BLEND_DESC / _DEPTH_STENCIL_DESC / _RASTERIZER_DESC all derive from
	// their D3D12 base with no extra data members, only additional constructors.
	// reinterpret_cast is safe here and avoids a user-defined conversion requirement.
	stream.blendDesc      = reinterpret_cast<const CD3DX12_BLEND_DESC&>(blendD3D12);
	stream.depthStencil   = reinterpret_cast<const CD3DX12_DEPTH_STENCIL_DESC&>(depthStencilD3D12);
	stream.rasterizerDesc = reinterpret_cast<const CD3DX12_RASTERIZER_DESC&>(rasterizerD3D12);

	D3D12_RT_FORMAT_ARRAY rtFormats = {};
	rtFormats.NumRenderTargets = desc.renderTargetCount;
	for(uint32_t i = 0; i < desc.renderTargetCount; ++i)
		rtFormats.RTFormats[i] = desc.renderTargetFormats[i];
	stream.renderTargetFormats = rtFormats;

	stream.depthStencilFormat = desc.depthStencilFormat;

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count   = desc.msaaSamples ? desc.msaaSamples : 1;
	sampleDesc.Quality = desc.msaaQuality;
	stream.sampleDesc = sampleDesc;

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.SizeInBytes                   = sizeof(stream);
	streamDesc.pPipelineStateSubobjectStream = &stream;

	ID3D12PipelineState* pso = nullptr;
	HRESULT hr = m_device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));
	if(FAILED(hr)) return nullptr;

	DX12PipelineState* dx12pso = new DX12PipelineState();
	dx12pso->pso           = pso;
	dx12pso->rootSignature = m_graphicsRootSignature;
	m_graphicsRootSignature->AddRef();
	dx12pso->isCompute     = false;
	return dx12pso;
}

// -----------------------------------------------------------------------
// CreateBLAS (Phase 6)
// Allocates the result and scratch GPU buffers for one triangle mesh BLAS.
// The actual build command is recorded by DX12CommandList::BuildBLAS().

GfxBLAS* DX12Device::CreateBLAS(const BLASDesc& desc)
{
	if(!m_supportsRaytracing || !desc.vertexBuffer) return nullptr;

	auto* vertexBuf = Cast(desc.vertexBuffer);

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type  = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	geomDesc.Triangles.VertexBuffer.StartAddress  =
		vertexBuf->resource->GetGPUVirtualAddress() + desc.positionOffset;
	geomDesc.Triangles.VertexBuffer.StrideInBytes = desc.vertexStride;
	geomDesc.Triangles.VertexCount                = desc.vertexCount;
	geomDesc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;

	if(desc.indexBuffer)
	{
		auto* indexBuf = Cast(desc.indexBuffer);
		geomDesc.Triangles.IndexBuffer = indexBuf->resource->GetGPUVirtualAddress();
		geomDesc.Triangles.IndexCount  = desc.indexCount;
		geomDesc.Triangles.IndexFormat = desc.index16bit
		    ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	}

	// Query prebuild info using a temporary inputs struct so pGeometryDescs is valid.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs       = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Flags          = desc.allowUpdate
	    ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
	    : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	auto* blas        = new DX12BLAS();
	blas->geomDesc    = geomDesc;
	blas->buildInputs = inputs;
	blas->buildInputs.pGeometryDescs = &blas->geomDesc; // stable pointer into the struct

	const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

	// Result buffer in RAYTRACING_ACCELERATION_STRUCTURE state.
	{
		CD3DX12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(
		    prebuildInfo.ResultDataMaxSizeInBytes,
		    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		HRESULT hr = m_device->CreateCommittedResource(
		    &defaultHeap, D3D12_HEAP_FLAG_NONE, &resultDesc,
		    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		    nullptr, IID_PPV_ARGS(&blas->resultBuffer));
		if(FAILED(hr)) { delete blas; return nullptr; }
	}

	// Scratch buffer sized for both initial build and updates.
	{
		const uint64_t scratchSize = max(prebuildInfo.ScratchDataSizeInBytes,
		                                 prebuildInfo.UpdateScratchDataSizeInBytes);
		CD3DX12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(
		    scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		HRESULT hr = m_device->CreateCommittedResource(
		    &defaultHeap, D3D12_HEAP_FLAG_NONE, &scratchDesc,
		    D3D12_RESOURCE_STATE_COMMON,
		    nullptr, IID_PPV_ARGS(&blas->scratchBuffer));
		if(FAILED(hr)) { delete blas; return nullptr; }
	}

	return blas;
}

// -----------------------------------------------------------------------
// CreateTLAS (Phase 6)
// Allocates result, scratch, and an upload-heap instance-desc buffer.
// Per-frame instance data is written by DX12CommandList::BuildTLAS().

GfxTLAS* DX12Device::CreateTLAS(const TLASDesc& desc)
{
	if(!m_supportsRaytracing || desc.maxInstances == 0) return nullptr;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs    = desc.maxInstances;
	inputs.Flags       = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE
	                   | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	auto* tlas                = new DX12TLAS();
	tlas->maxInstances        = desc.maxInstances;
	tlas->buildInputsTemplate = inputs;

	const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

	// Result buffer.
	{
		CD3DX12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(
		    prebuildInfo.ResultDataMaxSizeInBytes,
		    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		HRESULT hr = m_device->CreateCommittedResource(
		    &defaultHeap, D3D12_HEAP_FLAG_NONE, &resultDesc,
		    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		    nullptr, IID_PPV_ARGS(&tlas->resultBuffer));
		if(FAILED(hr)) { delete tlas; return nullptr; }
	}

	// Scratch buffer.
	{
		const uint64_t scratchSize = max(prebuildInfo.ScratchDataSizeInBytes,
		                                 prebuildInfo.UpdateScratchDataSizeInBytes);
		CD3DX12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(
		    scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		HRESULT hr = m_device->CreateCommittedResource(
		    &defaultHeap, D3D12_HEAP_FLAG_NONE, &scratchDesc,
		    D3D12_RESOURCE_STATE_COMMON,
		    nullptr, IID_PPV_ARGS(&tlas->scratchBuffer));
		if(FAILED(hr)) { delete tlas; return nullptr; }
	}

	// Upload-heap instance buffer: CPU writes D3D12_RAYTRACING_INSTANCE_DESC each frame.
	{
		const uint64_t instanceBufSize =
		    static_cast<uint64_t>(desc.maxInstances) * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
		CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC   instanceDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceBufSize);
		HRESULT hr = m_device->CreateCommittedResource(
		    &uploadHeap, D3D12_HEAP_FLAG_NONE, &instanceDesc,
		    D3D12_RESOURCE_STATE_GENERIC_READ,
		    nullptr, IID_PPV_ARGS(&tlas->instanceBuffer));
		if(FAILED(hr)) { delete tlas; return nullptr; }
	}

	return tlas;
}

// -----------------------------------------------------------------------
// CreateRaytracingPipeline (Phase 6)
// Compiles an RT state object from a DXIL library and builds a minimal
// shader table: [0] raygen  [1] miss  [2] hit group.

GfxRaytracingPipeline* DX12Device::CreateRaytracingPipeline(
    const RaytracingPipelineDesc& desc)
{
	if(!m_supportsRaytracing)  return nullptr;
	if(!desc.shaderBytecode)   return nullptr;
	if(!desc.raygenEntryPoint) return nullptr;
	if(!desc.missEntryPoint)   return nullptr;
	if(!desc.closestHitEntry)  return nullptr;
	if(!desc.hitGroupName)     return nullptr;

	D3D12_EXPORT_DESC shaderExports[3] = {};
	shaderExports[0] = { desc.raygenEntryPoint, nullptr, D3D12_EXPORT_FLAG_NONE };
	shaderExports[1] = { desc.missEntryPoint,   nullptr, D3D12_EXPORT_FLAG_NONE };
	shaderExports[2] = { desc.closestHitEntry,  nullptr, D3D12_EXPORT_FLAG_NONE };

	D3D12_DXIL_LIBRARY_DESC libraryDesc = {};
	libraryDesc.DXILLibrary = { desc.shaderBytecode, desc.shaderBytecodeSize };
	libraryDesc.NumExports  = 3;
	libraryDesc.pExports    = shaderExports;

	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	hitGroupDesc.HitGroupExport           = desc.hitGroupName;
	hitGroupDesc.Type                     = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupDesc.ClosestHitShaderImport   = desc.closestHitEntry;
	hitGroupDesc.AnyHitShaderImport       = nullptr;
	hitGroupDesc.IntersectionShaderImport = nullptr;

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxPayloadSizeInBytes   = desc.maxPayloadSize;
	shaderConfig.MaxAttributeSizeInBytes = desc.maxAttributeSize;

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = desc.maxRecursionDepth;

	D3D12_GLOBAL_ROOT_SIGNATURE globalRS = {};
	globalRS.pGlobalRootSignature = m_graphicsRootSignature;

	D3D12_STATE_SUBOBJECT subobjects[5] = {};
	subobjects[0] = { D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,               &libraryDesc   };
	subobjects[1] = { D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,                  &hitGroupDesc  };
	subobjects[2] = { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,   &shaderConfig  };
	subobjects[3] = { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig};
	subobjects[4] = { D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,      &globalRS      };

	D3D12_STATE_OBJECT_DESC soDesc = {};
	soDesc.Type          = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	soDesc.NumSubobjects = 5;
	soDesc.pSubobjects   = subobjects;

	auto* pipeline = new DX12RaytracingPipeline();

	HRESULT hr = m_device->CreateStateObject(&soDesc, IID_PPV_ARGS(&pipeline->stateObject));
	if(FAILED(hr)) { delete pipeline; return nullptr; }

	hr = pipeline->stateObject->QueryInterface(IID_PPV_ARGS(&pipeline->properties));
	if(FAILED(hr)) { delete pipeline; return nullptr; }

	// Build the shader table: three records each padded to TableAlignment.
	constexpr uint32_t IdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;   // 32 bytes
	constexpr uint32_t TableAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT; // 64 bytes
	const uint32_t recordSize = (IdentifierSize + TableAlignment - 1) & ~(TableAlignment - 1);
	const uint32_t tableSize  = recordSize * 3;

	CD3DX12_HEAP_PROPERTIES uploadProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC   tableDesc = CD3DX12_RESOURCE_DESC::Buffer(tableSize);
	hr = m_device->CreateCommittedResource(
	    &uploadProps, D3D12_HEAP_FLAG_NONE, &tableDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr, IID_PPV_ARGS(&pipeline->shaderTable));
	if(FAILED(hr)) { delete pipeline; return nullptr; }

	uint8_t* mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	hr = pipeline->shaderTable->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));
	if(FAILED(hr)) { delete pipeline; return nullptr; }

	void* raygenID = pipeline->properties->GetShaderIdentifier(desc.raygenEntryPoint);
	void* missID   = pipeline->properties->GetShaderIdentifier(desc.missEntryPoint);
	void* hitID    = pipeline->properties->GetShaderIdentifier(desc.hitGroupName);

	memcpy(mappedData,                  raygenID, IdentifierSize);
	memcpy(mappedData + recordSize,     missID,   IdentifierSize);
	memcpy(mappedData + recordSize * 2, hitID,    IdentifierSize);

	pipeline->shaderTable->Unmap(0, nullptr);

	pipeline->shaderRecordSize = recordSize;
	pipeline->raygenOffset     = 0;
	pipeline->missOffset       = recordSize;
	pipeline->hitGroupOffset   = static_cast<uint64_t>(recordSize) * 2;

	return pipeline;
}

} // namespace EngineCore::RHI::DX12
