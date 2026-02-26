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

class DX11Device; // forward declaration

class DX11FrameScheduler : public IGfxFrameScheduler
{
public:
	explicit DX11FrameScheduler(DX11Device* owner,
		ID3D11DeviceContext* context, ID3D11DeviceContext3* context3)
		: m_owner(owner)
		, m_commandList(context, context3)
	{}

	void BeginFrame() override {}  // DX11 has no frame-level sync
	void FlushGPU()   override {}  // immediate context flushes implicitly

	void ExecuteCommandLists(uint32_t /*count*/, IGfxCommandList* const* /*lists*/) override {}

	void Present(uint32_t syncInterval) override;  // implemented after DX11Device is defined

	uint32_t CurrentFrameIndex() override { return 0; }

	IGfxCommandList* AllocateCommandList() override  { return &m_commandList; }
	void             FreeCommandList(IGfxCommandList*) override {}  // singleton

private:
	DX11Device*     m_owner;
	DX11CommandList m_commandList;
};

// -----------------------------------------------------------------------

class DX11Device : public IGfxDevice
{
public:
	DX11Device() = default;

	~DX11Device() override
	{
		if(m_context) m_context->ClearState();
		_RELEASE(m_context);
		_RELEASE(m_context3);
		_RELEASE(m_device);
		_RELEASE(m_device3);
		_RELEASE(m_dxgiDevice);
		_RELEASE(m_dxgiAdapter);
		_RELEASE(m_dxgiFactory);
	}

	bool Init(HWND /*windowHandle*/) override
	{
		uint32_t createDeviceFlags = 0;
	#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif

		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};
		uint32_t numFeatureLevels = ARRAYSIZE(featureLevels);

		HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags,
			featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &m_device, nullptr, &m_context);
		if(FAILED(hr)) return false;

		hr = m_device->QueryInterface(__uuidof(ID3D11Device3), reinterpret_cast<void**>(&m_device3));
		if(FAILED(hr)) return false;
		hr = m_context->QueryInterface(__uuidof(ID3D11DeviceContext3), reinterpret_cast<void**>(&m_context3));
		if(FAILED(hr)) return false;

		hr = m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&m_dxgiDevice));
		if(FAILED(hr)) return false;
		hr = m_dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&m_dxgiAdapter));
		if(FAILED(hr)) return false;
		hr = m_dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&m_dxgiFactory));
		if(FAILED(hr)) return false;

		return true;
	}

	void Shutdown() override {}

	bool InitSwapChain(HWND hwnd, uint32_t width, uint32_t height) override
	{
		DXGI_SWAP_CHAIN_DESC1 schd = {};
		schd.BufferCount = 1;
		schd.Width = width;
		schd.Height = height;
		schd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		schd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		schd.SampleDesc.Count = 1;
		schd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		schd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC schdf = {};
		schdf.RefreshRate.Denominator = 1;
		schdf.RefreshRate.Numerator = 60;
		schdf.Scaling = DXGI_MODE_SCALING_CENTERED;
		schdf.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
		schdf.Windowed = TRUE;

		HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(m_device, hwnd, &schd, &schdf, nullptr, &m_swapChain);
		if(FAILED(hr)) return false;

		m_dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
		return true;
	}

	IDXGISwapChain1*      GetSwapChain()  const { return m_swapChain; }

	// Raw DX11 accessors (for legacy code that still uses direct DX11 calls)
	ID3D11Device*         GetDevice()     const { return m_device; }
	ID3D11Device3*        GetDevice3()    const { return m_device3; }
	ID3D11DeviceContext*  GetContext()     const { return m_context; }
	ID3D11DeviceContext3* GetContext3()    const { return m_context3; }
	IDXGIAdapter1*        GetAdapter()    const { return m_dxgiAdapter; }

	DX11FrameScheduler* CreateFrameScheduler()
	{
		return new DX11FrameScheduler(this, m_context, m_context3);
	}

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
		result->dimension = desc.dimension;

		UINT bindFlags = 0;
		if(desc.allowRTV) bindFlags |= D3D11_BIND_RENDER_TARGET;
		if(desc.allowDSV) bindFlags |= D3D11_BIND_DEPTH_STENCIL;
		if(desc.allowSRV) bindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if(desc.allowUAV) bindFlags |= D3D11_BIND_UNORDERED_ACCESS;

		UINT miscFlags = desc.generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

		const bool isCube = desc.dimension == TextureDimension::CubeMap
			|| desc.dimension == TextureDimension::CubeMapArray;

		if(desc.dimension == TextureDimension::Tex2D || desc.dimension == TextureDimension::Tex2DArray
			|| isCube)
		{
			uint32_t arraySize = max(1u, desc.depth);
			if(desc.dimension == TextureDimension::CubeMap) arraySize = 6;
			else if(desc.dimension == TextureDimension::CubeMapArray) arraySize = max(1u, desc.depth) * 6;

			D3D11_TEXTURE2D_DESC d = {};
			d.Width            = desc.width;
			d.Height           = desc.height;
			d.MipLevels        = desc.mipLevels;
			d.ArraySize        = arraySize;
			d.Format           = desc.format;
			d.SampleDesc.Count = max(1u, desc.msaaSamples);
			d.SampleDesc.Quality = desc.msaaQuality;
			d.Usage            = D3D11_USAGE_DEFAULT;
			d.BindFlags        = bindFlags;
			d.MiscFlags        = miscFlags | (isCube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0);

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
			d.Usage     = desc.initialData ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
			d.BindFlags = bindFlags;
			d.MiscFlags = miscFlags;

			D3D11_SUBRESOURCE_DATA initData = {};
			if(desc.initialData)
			{
				initData.pSysMem          = desc.initialData;
				initData.SysMemPitch      = desc.initialDataRowPitch;
				initData.SysMemSlicePitch = desc.initialDataSlicePitch;
			}
			m_device->CreateTexture3D(&d, desc.initialData ? &initData : nullptr, &result->texture3D);
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

	GfxDSV* CreateDSV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mipSlice,
		uint32_t arraySlice) override
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
		desc.Format = format;
		if(arraySlice != UINT32_MAX)
		{
			desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = mipSlice;
			desc.Texture2DArray.FirstArraySlice = arraySlice;
			desc.Texture2DArray.ArraySize = 1;
		}
		else
		{
			desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = mipSlice;
		}

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
		else if(dx11tex->dimension == TextureDimension::Tex2DArray)
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MostDetailedMip = mostDetailedMip;
			desc.Texture2DArray.MipLevels = (mipLevels == UINT32_MAX) ? UINT(-1) : mipLevels;
			desc.Texture2DArray.FirstArraySlice = 0;
			desc.Texture2DArray.ArraySize = dx11tex->depth;
		}
		else if(dx11tex->dimension == TextureDimension::CubeMap)
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			desc.TextureCube.MostDetailedMip = mostDetailedMip;
			desc.TextureCube.MipLevels = (mipLevels == UINT32_MAX) ? UINT(-1) : mipLevels;
		}
		else if(dx11tex->dimension == TextureDimension::CubeMapArray)
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
			desc.TextureCubeArray.MostDetailedMip = mostDetailedMip;
			desc.TextureCubeArray.MipLevels = (mipLevels == UINT32_MAX) ? UINT(-1) : mipLevels;
			desc.TextureCubeArray.First2DArrayFace = 0;
			desc.TextureCubeArray.NumCubes = dx11tex->depth;
		}
		else
		{
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MostDetailedMip = mostDetailedMip;
			desc.Texture2D.MipLevels = (mipLevels == UINT32_MAX) ? UINT(-1) : mipLevels;
		}

		auto* result = new DX11SRV();
		result->sourceTexture = texture; // non-owning back-pointer for CopyTextureRegion etc.
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

	GfxUAV* CreateUAV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mipSlice,
		uint32_t arraySlice) override
	{
		auto* dx11tex = Cast(texture);
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.Format = format;

		if(arraySlice != UINT32_MAX)
		{
			// Per-slice UAV (e.g. individual cube face)
			desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = mipSlice;
			desc.Texture2DArray.FirstArraySlice = arraySlice;
			desc.Texture2DArray.ArraySize = 1;
		}
		else if(dx11tex->texture3D)
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
		m_device->CreateSamplerState(reinterpret_cast<const D3D11_SAMPLER_DESC*>(&desc), &result->sampler);
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
		const InputElementDesc* elements, uint32_t elementCount) override
	{
		auto* result = new DX11InputLayout();
		m_device->CreateInputLayout(DX11::ToD3D11(elements), elementCount, bytecode, bytecodeSize, &result->layout);
		return result;
	}

	void DestroyInputLayout(GfxInputLayout* layout) override { delete Cast(layout); }

	// -----------------------------------------------------------------------
	// Render state objects

	void* CreateDepthStencilStateObject(const DepthStencilDesc& desc) override
	{
		ID3D11DepthStencilState* state = nullptr;
		m_device->CreateDepthStencilState(&DX11::ToD3D11(desc), &state);
		return state;
	}

	void* CreateBlendStateObject(const BlendDesc& desc) override
	{
		ID3D11BlendState* state = nullptr;
		m_device->CreateBlendState(&DX11::ToD3D11(desc), &state);
		return state;
	}

	void* CreateRasterizerStateObject(const RasterizerDesc& desc) override
	{
		ID3D11RasterizerState* state = nullptr;
		m_device->CreateRasterizerState(&DX11::ToD3D11(desc), &state);
		return state;
	}

	// -----------------------------------------------------------------------
	// Shader objects

	void* CreateShaderObject(uint8_t type, const void* bytecode, size_t size) override
	{
		IUnknown* shader = nullptr;
		switch(type)
		{
		case 0: m_device->CreatePixelShader(bytecode, size, nullptr, (ID3D11PixelShader**)&shader); break;
		case 1: m_device->CreateVertexShader(bytecode, size, nullptr, (ID3D11VertexShader**)&shader); break;
		case 2: m_device->CreateDomainShader(bytecode, size, nullptr, (ID3D11DomainShader**)&shader); break;
		case 3: m_device->CreateHullShader(bytecode, size, nullptr, (ID3D11HullShader**)&shader); break;
		case 4: m_device->CreateGeometryShader(bytecode, size, nullptr, (ID3D11GeometryShader**)&shader); break;
		case 5: m_device->CreateComputeShader(bytecode, size, nullptr, (ID3D11ComputeShader**)&shader); break;
		}
		return shader;
	}

	void DestroyShaderObject(void* shaderObject) override
	{
		if(shaderObject)
			static_cast<IUnknown*>(shaderObject)->Release();
	}

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
	// CPU ↔ GPU texture data transfer

	HRESULT ReadTextureRegion(GfxTexture* texture, uint32_t subresource,
		const GfxBox& sourceBox, void* destData, uint32_t destRowPitch, uint32_t destDepthPitch) override
	{
		auto* dx11tex = Cast(texture);
		ID3D11Resource* resource = dx11tex->AsResource();
		if(!resource) return E_FAIL;

		HRESULT hr = m_context3->Map(resource, subresource, D3D11_MAP_READ, 0, nullptr);
		if(FAILED(hr)) return hr;

		D3D11_BOX d3dBox;
		d3dBox.left   = sourceBox.left;
		d3dBox.top    = sourceBox.top;
		d3dBox.front  = sourceBox.front;
		d3dBox.right  = sourceBox.right;
		d3dBox.bottom = sourceBox.bottom;
		d3dBox.back   = sourceBox.back;

		m_device3->ReadFromSubresource(destData, destRowPitch, destDepthPitch,
			resource, subresource, &d3dBox);

		m_context3->Unmap(resource, subresource);
		return S_OK;
	}

	HRESULT WriteTextureRegion(GfxTexture* texture, uint32_t subresource,
		const GfxBox& destBox, const void* sourceData, uint32_t sourceRowPitch, uint32_t sourceDepthPitch) override
	{
		auto* dx11tex = Cast(texture);
		ID3D11Resource* resource = dx11tex->AsResource();
		if(!resource) return E_FAIL;

		HRESULT hr = m_context3->Map(resource, subresource, D3D11_MAP_WRITE, 0, nullptr);
		if(FAILED(hr)) return hr;

		D3D11_BOX d3dBox;
		d3dBox.left   = destBox.left;
		d3dBox.top    = destBox.top;
		d3dBox.front  = destBox.front;
		d3dBox.right  = destBox.right;
		d3dBox.bottom = destBox.bottom;
		d3dBox.back   = destBox.back;

		m_device3->WriteToSubresource(resource, subresource, &d3dBox,
			sourceData, sourceRowPitch, sourceDepthPitch);

		m_context3->Unmap(resource, subresource);
		return S_OK;
	}

	// -----------------------------------------------------------------------
	// Texture readback

	HRESULT CaptureTexture(GfxTexture* texture, DirectX::ScratchImage& result) override
	{
		auto* dx11tex = Cast(texture);
		ID3D11Resource* resource = dx11tex->AsResource();
		if(!resource) return E_FAIL;
		return DirectX::CaptureTexture(m_device, m_context, resource, result);
	}

	HRESULT CaptureTexture(GfxSRV* srv, DirectX::ScratchImage& result) override
	{
		auto* dx11srv = Cast(srv);
		ID3D11Resource* resource = nullptr;
		dx11srv->view->GetResource(&resource);
		if(!resource) return E_FAIL;
		HRESULT hr = DirectX::CaptureTexture(m_device, m_context, resource, result);
		resource->Release();
		return hr;
	}

	// -----------------------------------------------------------------------
	// Diagnostics

	const char* GetAdapterName() override
	{
		if(m_adapterName[0] == 0 && m_dxgiAdapter)
		{
			DXGI_ADAPTER_DESC1 desc = {};
			if(SUCCEEDED(m_dxgiAdapter->GetDesc1(&desc)))
				WideCharToMultiByte(CP_ACP, 0, desc.Description, -1, m_adapterName, sizeof(m_adapterName), nullptr, nullptr);
		}
		return m_adapterName;
	}
	bool SupportsRaytracing()    override { return false; }
	bool SupportsMeshShaders()   override { return false; }

	ID3D11Device*         GetDX11Device()   override { return m_device; }
	ID3D11Device3*        GetDX11Device3()  override { return m_device3; }
	ID3D11DeviceContext*  GetDX11Context()  override { return m_context; }
	ID3D11DeviceContext3* GetDX11Context3() override { return m_context3; }

private:
	ID3D11Device*         m_device       = nullptr;
	ID3D11Device3*        m_device3      = nullptr;
	ID3D11DeviceContext*  m_context      = nullptr;
	ID3D11DeviceContext3* m_context3     = nullptr;
	IDXGIDevice*          m_dxgiDevice   = nullptr;
	IDXGIAdapter1*        m_dxgiAdapter  = nullptr;
	IDXGIFactory2*        m_dxgiFactory  = nullptr;
	IDXGISwapChain1*      m_swapChain    = nullptr;
	char                  m_adapterName[128] = {};
};

// DX11FrameScheduler::Present — defined here because it needs DX11Device to be complete
inline void DX11FrameScheduler::Present(uint32_t syncInterval)
{
	auto* swapChain = m_owner->GetSwapChain();
	if(swapChain)
	{
		DXGI_PRESENT_PARAMETERS params = {};
		swapChain->Present1(syncInterval, 0, &params);
	}
}

} // namespace EngineCore::RHI::DX11
