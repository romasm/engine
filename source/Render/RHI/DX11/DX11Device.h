#pragma once
#include "GfxDevice.h"
#include "GfxSync.h"
#include "DX11Types.h"
#include "DX11CommandList.h"
#include "MaterialData.h"

// DX11Device — implements IGfxDevice on top of ID3D11Device.
// Delegates most resource creation to existing Buffer/Render/RenderState systems
// so that existing code continues to work unchanged during the migration.

namespace EngineCore::RHI::DX11
{

class DX11FrameScheduler : public IGfxFrameScheduler
{
public:
	explicit DX11FrameScheduler(ID3D11DeviceContext* context, ID3D11DeviceContext3* context3,
		IDXGISwapChain1* swapChain)
		: m_swapChain(swapChain)
		, m_commandList(context, context3)
	{}

	void BeginFrame() override {}  // DX11 has no frame-level sync
	void FlushGPU()   override {}  // immediate context flushes implicitly

	void ExecuteCommandLists(uint32_t /*count*/, IGfxCommandList* const* /*lists*/) override {}

	void Present(uint32_t syncInterval) override
	{
		DXGI_PRESENT_PARAMETERS params = {};
		m_swapChain->Present1(syncInterval, 0, &params);
	}

	uint32_t CurrentFrameIndex() override { return 0; }

	IGfxCommandList* AllocateCommandList() override  { return &m_commandList; }
	void             FreeCommandList(IGfxCommandList*) override {}  // singleton

private:
	IDXGISwapChain1* m_swapChain;
	DX11CommandList  m_commandList;
};

// -----------------------------------------------------------------------

class DX11Device : public IGfxDevice
{
public:
	DX11Device(ID3D11Device* device, ID3D11Device3* device3,
		ID3D11DeviceContext* context, ID3D11DeviceContext3* context3)
		: m_device(device), m_device3(device3)
		, m_context(context), m_context3(context3)
	{}

	~DX11Device() override {}

	bool Init(HWND /*windowHandle*/) override { return true; }
	void Shutdown() override {}

	// -----------------------------------------------------------------------
	// Buffers

	GfxBuffer* CreateBuffer(const BufferDesc& desc) override
	{
		D3D11_BUFFER_DESC d;
		d.ByteWidth           = desc.sizeBytes;
		d.BindFlags           = 0;
		d.MiscFlags           = 0;
		d.StructureByteStride = desc.structureStrideBytes;
		d.CPUAccessFlags      = (desc.usage == BufferUsage::Dynamic) ? D3D11_CPU_ACCESS_WRITE : 0;
		d.Usage               = (desc.usage == BufferUsage::Dynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;

		if(desc.isVertex)   d.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
		if(desc.isIndex)    d.BindFlags |= D3D11_BIND_INDEX_BUFFER;
		if(desc.isConstant) d.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
		if(desc.allowSRV)   { d.BindFlags |= D3D11_BIND_SHADER_RESOURCE; d.MiscFlags |= (desc.structureStrideBytes > 0 ? D3D11_RESOURCE_MISC_BUFFER_STRUCTURED : 0); }
		if(desc.allowUAV)   d.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = desc.initialData;

		ID3D11Buffer* buffer = nullptr;
		HRESULT hr = m_device->CreateBuffer(&d, desc.initialData ? &initData : nullptr, &buffer);
		if(FAILED(hr)) return nullptr;

		auto* result = new DX11Buffer();
		result->buffer = buffer;
		return result;
	}

	void DestroyBuffer(GfxBuffer* buffer) override
	{
		delete Cast(buffer);
	}

	// -----------------------------------------------------------------------
	// Textures

	GfxTexture* CreateTexture(const TextureDesc& desc) override
	{
		auto* result = new DX11Texture();
		result->width     = desc.width;
		result->height    = desc.height;
		result->depth     = desc.depth;
		result->mipLevels = desc.mipLevels;
		result->msaaSamples = max(1u, desc.msaaSamples);
		result->format    = desc.format;

		UINT bindFlags = 0;
		if(desc.allowRTV) bindFlags |= D3D11_BIND_RENDER_TARGET;
		if(desc.allowDSV) bindFlags |= D3D11_BIND_DEPTH_STENCIL;
		if(desc.allowSRV) bindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if(desc.allowUAV) bindFlags |= D3D11_BIND_UNORDERED_ACCESS;

		UINT miscFlags = desc.generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

		if(desc.dimension == TextureDimension::Tex2D || desc.dimension == TextureDimension::CubeMap)
		{
			D3D11_TEXTURE2D_DESC d = {};
			d.Width            = desc.width;
			d.Height           = desc.height;
			d.MipLevels        = desc.mipLevels;
			d.ArraySize        = (desc.dimension == TextureDimension::CubeMap) ? 6 : max(1u, desc.depth);
			d.Format           = desc.format;
			d.SampleDesc.Count = max(1u, desc.msaaSamples);
			d.SampleDesc.Quality = desc.msaaQuality;
			d.Usage            = D3D11_USAGE_DEFAULT;
			d.BindFlags        = bindFlags;
			d.MiscFlags        = miscFlags | (desc.dimension == TextureDimension::CubeMap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0);

			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = desc.initialData;
			m_device->CreateTexture2D(&d, desc.initialData ? &initData : nullptr, &result->texture2D);
		}
		else if(desc.dimension == TextureDimension::Tex3D)
		{
			D3D11_TEXTURE3D_DESC d = {};
			d.Width     = desc.width;
			d.Height    = desc.height;
			d.Depth     = desc.depth;
			d.MipLevels = desc.mipLevels;
			d.Format    = desc.format;
			d.Usage     = D3D11_USAGE_DEFAULT;
			d.BindFlags = bindFlags;
			d.MiscFlags = miscFlags;
			m_device->CreateTexture3D(&d, nullptr, &result->texture3D);
		}

		return result;
	}

	void DestroyTexture(GfxTexture* texture) override
	{
		delete Cast(texture);
	}

	// -----------------------------------------------------------------------
	// Views

	GfxRTV* CreateRTV(GfxTexture* texture, DXGI_FORMAT format,
		uint32_t mipSlice, uint32_t /*arraySlice*/) override
	{
		auto* dx11tex = Cast(texture);
		D3D11_RENDER_TARGET_VIEW_DESC desc = {};
		desc.Format = format;

		if(dx11tex->texture3D)
		{
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
			desc.Texture3D.MipSlice = mipSlice;
			desc.Texture3D.WSize = dx11tex->depth;
		}
		else if(dx11tex->msaaSamples > 1)
		{
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = mipSlice;
		}

		auto* result = new DX11RTV();
		m_device->CreateRenderTargetView(dx11tex->AsResource(), &desc, &result->view);
		return result;
	}

	GfxDSV* CreateDSV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mipSlice) override
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
		desc.Format = format;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = mipSlice;

		auto* result = new DX11DSV();
		m_device->CreateDepthStencilView(Cast(texture)->AsResource(), &desc, &result->view);
		return result;
	}

	GfxSRV* CreateSRV(GfxTexture* texture, DXGI_FORMAT format,
		uint32_t mostDetailedMip, uint32_t mipLevels) override
	{
		auto* dx11tex = Cast(texture);
		D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = format;

		if(dx11tex->texture3D)
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			desc.Texture3D.MostDetailedMip = mostDetailedMip;
			desc.Texture3D.MipLevels = (mipLevels == UINT32_MAX) ? UINT(-1) : mipLevels;
		}
		else
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MostDetailedMip = mostDetailedMip;
			desc.Texture2D.MipLevels = (mipLevels == UINT32_MAX) ? UINT(-1) : mipLevels;
		}

		auto* result = new DX11SRV();
		m_device->CreateShaderResourceView(dx11tex->AsResource(), &desc, &result->view);
		return result;
	}

	GfxSRV* CreateSRV(GfxBuffer* buffer, uint32_t firstElement, uint32_t numElements) override
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		desc.BufferEx.FirstElement = firstElement;
		desc.BufferEx.NumElements  = numElements;

		auto* result = new DX11SRV();
		m_device->CreateShaderResourceView(Cast(buffer)->buffer, &desc, &result->view);
		return result;
	}

	GfxUAV* CreateUAV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mipSlice) override
	{
		auto* dx11tex = Cast(texture);
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.Format = format;

		if(dx11tex->texture3D)
		{
			desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
			desc.Texture3D.MipSlice = mipSlice;
			desc.Texture3D.WSize = dx11tex->depth;
		}
		else
		{
			desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = mipSlice;
		}

		auto* result = new DX11UAV();
		m_device->CreateUnorderedAccessView(dx11tex->AsResource(), &desc, &result->view);
		return result;
	}

	GfxUAV* CreateUAV(GfxBuffer* buffer, uint32_t firstElement, uint32_t numElements) override
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		desc.Buffer.FirstElement = firstElement;
		desc.Buffer.NumElements  = numElements;

		auto* result = new DX11UAV();
		m_device->CreateUnorderedAccessView(Cast(buffer)->buffer, &desc, &result->view);
		return result;
	}

	void DestroyRTV(GfxRTV* rtv)     override { delete Cast(rtv); }
	void DestroyDSV(GfxDSV* dsv)     override { delete Cast(dsv); }
	void DestroyView(GfxSRV* srv)    override { delete Cast(srv); }
	void DestroyView(GfxUAV* uav)    override { delete Cast(uav); }

	// -----------------------------------------------------------------------
	// Samplers

	GfxSampler* CreateSampler(const SamplerDesc& desc) override
	{
		auto* result = new DX11Sampler();
		m_device->CreateSamplerState(&desc.d3d11Desc, &result->sampler);
		return result;
	}

	void DestroySampler(GfxSampler* sampler) override { delete Cast(sampler); }

	// -----------------------------------------------------------------------
	// Pipeline states
	// The DX11 backend stores state indices from RenderStateMgr; PSOs are created
	// lazily when Set() is called on the technique.

	GfxPipelineState* CreateGraphicsPSO(const GraphicsPSODesc& desc) override
	{
		auto* pso = new DX11PipelineState();
		pso->shadersID[SHADER_VS] = desc.vertexShaderID;
		pso->shadersID[SHADER_PS] = desc.pixelShaderID;
		pso->shadersID[SHADER_HS] = desc.hullShaderID;
		pso->shadersID[SHADER_DS] = desc.domainShaderID;
		pso->shadersID[SHADER_GS] = desc.geometryShaderID;
		pso->depthState   = desc.depthStateID;
		pso->blendState   = desc.blendStateID;
		pso->rastState    = desc.rastStateID;
		pso->inputLayout  = Cast(desc.inputLayout);
		return pso;
	}

	GfxPipelineState* CreateComputePSO(const ComputePSODesc& desc) override
	{
		auto* pso = new DX11PipelineState();
		pso->computeShaderID = desc.computeShaderID;
		pso->isCompute = true;
		return pso;
	}

	void DestroyPSO(GfxPipelineState* pso) override { delete Cast(pso); }

	// -----------------------------------------------------------------------
	// Input layout

	GfxInputLayout* CreateInputLayout(const void* bytecode, size_t bytecodeSize,
		const D3D11_INPUT_ELEMENT_DESC* elements, uint32_t elementCount) override
	{
		auto* result = new DX11InputLayout();
		m_device->CreateInputLayout(elements, elementCount, bytecode, bytecodeSize, &result->layout);
		return result;
	}

	void DestroyInputLayout(GfxInputLayout* layout) override { delete Cast(layout); }

	// -----------------------------------------------------------------------
	// Back buffer (managed by Window/SwapChain, not here)

	GfxTexture*  GetBackBuffer(uint32_t /*index*/) override { return nullptr; }
	uint32_t     GetBackBufferCount()              override { return 1; }
	uint32_t     GetCurrentBackBufferIndex()       override { return 0; }

	// -----------------------------------------------------------------------
	// Query heaps

	GfxQueryHeap* CreateQueryHeap(QueryType type, uint32_t count) override
	{
		D3D11_QUERY_DESC desc = {};
		desc.Query = (type == QueryType::TimestampDisjoint)
			? D3D11_QUERY_TIMESTAMP_DISJOINT
			: D3D11_QUERY_TIMESTAMP;

		auto* heap = new DX11QueryHeap();
		heap->queries.resize(count);
		for(uint32_t i = 0; i < count; ++i)
		{
			heap->queries[i] = nullptr;
			if(FAILED(m_device->CreateQuery(&desc, &heap->queries[i])))
			{
				delete heap;
				return nullptr;
			}
		}
		return heap;
	}

	void DestroyQueryHeap(GfxQueryHeap* heap) override { delete Cast(heap); }

	// -----------------------------------------------------------------------
	// Diagnostics

	const char* GetAdapterName() override { return "DirectX 11"; }
	bool SupportsRaytracing()    override { return false; }
	bool SupportsMeshShaders()   override { return false; }

private:
	ID3D11Device*         m_device   = nullptr;
	ID3D11Device3*        m_device3  = nullptr;
	ID3D11DeviceContext*  m_context  = nullptr;
	ID3D11DeviceContext3* m_context3 = nullptr;
};

} // namespace EngineCore::RHI::DX11
