#pragma once
#include "RHITypes.h"

// DX11 concrete resource types.
// Each wraps raw DX11 COM pointers behind the RHI base structs.
// High-level code uses GfxBuffer*/GfxTexture* etc.; only the DX11 backend
// casts to these concrete types to extract the underlying DX11 objects.

namespace EngineCore::RHI::DX11
{

// -----------------------------------------------------------------------
// Buffer

struct DX11Buffer : GfxBuffer
{
	ID3D11Buffer* buffer = nullptr;

	~DX11Buffer() override
	{
		if(buffer) { buffer->Release(); buffer = nullptr; }
	}
};

// -----------------------------------------------------------------------
// Texture

struct DX11Texture : GfxTexture
{
	ID3D11Texture2D* texture2D = nullptr;
	ID3D11Texture3D* texture3D = nullptr;
	uint32_t         msaaSamples = 1;

	~DX11Texture() override
	{
		if(texture2D) { texture2D->Release(); texture2D = nullptr; }
		if(texture3D) { texture3D->Release(); texture3D = nullptr; }
	}

	ID3D11Resource* AsResource() const
	{
		if(texture2D) return texture2D;
		return texture3D;
	}
};

// -----------------------------------------------------------------------
// Views

struct DX11RTV : GfxRTV
{
	ID3D11RenderTargetView* view = nullptr;
	~DX11RTV() override { if(view) { view->Release(); view = nullptr; } }
};

struct DX11DSV : GfxDSV
{
	ID3D11DepthStencilView* view = nullptr;
	~DX11DSV() override { if(view) { view->Release(); view = nullptr; } }
};

struct DX11SRV : GfxSRV
{
	ID3D11ShaderResourceView* view = nullptr;
	~DX11SRV() override { if(view) { view->Release(); view = nullptr; } }
};

struct DX11UAV : GfxUAV
{
	ID3D11UnorderedAccessView* view = nullptr;
	~DX11UAV() override { if(view) { view->Release(); view = nullptr; } }
};

// -----------------------------------------------------------------------
// Sampler

struct DX11Sampler : GfxSampler
{
	ID3D11SamplerState* sampler = nullptr;
	~DX11Sampler() override { if(sampler) { sampler->Release(); sampler = nullptr; } }
};

// -----------------------------------------------------------------------
// Input layout

struct DX11InputLayout : GfxInputLayout
{
	ID3D11InputLayout* layout = nullptr;
	~DX11InputLayout() override { if(layout) { layout->Release(); layout = nullptr; } }
};

// -----------------------------------------------------------------------
// Pipeline state (stores compiled DX11 state IDs — indices into RenderStateMgr arrays)

struct DX11PipelineState : GfxPipelineState
{
	uint16_t shadersID[5] = {};  // indexed by SHADER_PS(0), SHADER_VS(1), SHADER_DS(2), SHADER_HS(3), SHADER_GS(4)
	uint16_t computeShaderID = 0;
	uint16_t depthState   = 0;
	uint16_t blendState   = 0;
	uint16_t rastState    = 0;
	uint8_t  queue        = 0;
	bool     isCompute    = false;
	DX11InputLayout* inputLayout = nullptr;

	~DX11PipelineState() override {}
};

// -----------------------------------------------------------------------
// Query heap

struct DX11QueryHeap : GfxQueryHeap
{
	DArray<ID3D11Query*> queries;

	~DX11QueryHeap() override
	{
		for(uint32_t i = 0; i < queries.size(); ++i)
			if(queries[i]) { queries[i]->Release(); queries[i] = nullptr; }
		queries.destroy();
	}
};

// -----------------------------------------------------------------------
// Helper casts (safe — callers only use these inside DX11 backend code)

inline DX11Buffer*       Cast(GfxBuffer*       p) { return static_cast<DX11Buffer*>(p); }
inline DX11Texture*      Cast(GfxTexture*      p) { return static_cast<DX11Texture*>(p); }
inline DX11RTV*          Cast(GfxRTV*          p) { return static_cast<DX11RTV*>(p); }
inline DX11DSV*          Cast(GfxDSV*          p) { return static_cast<DX11DSV*>(p); }
inline DX11SRV*          Cast(GfxSRV*          p) { return static_cast<DX11SRV*>(p); }
inline DX11UAV*          Cast(GfxUAV*          p) { return static_cast<DX11UAV*>(p); }
inline DX11Sampler*      Cast(GfxSampler*      p) { return static_cast<DX11Sampler*>(p); }
inline DX11InputLayout*  Cast(GfxInputLayout*  p) { return static_cast<DX11InputLayout*>(p); }
inline DX11PipelineState* Cast(GfxPipelineState* p) { return static_cast<DX11PipelineState*>(p); }
inline DX11QueryHeap*    Cast(GfxQueryHeap*    p) { return static_cast<DX11QueryHeap*>(p); }

// -----------------------------------------------------------------------
// Binary-compatible casts from RHI descriptor structs to D3D11 equivalents.
// Guaranteed safe by static_assert in RHITypes.h.

inline const D3D11_DEPTH_STENCIL_DESC& ToD3D11(const DepthStencilDesc& d)
{ return reinterpret_cast<const D3D11_DEPTH_STENCIL_DESC&>(d); }

inline const D3D11_BLEND_DESC& ToD3D11(const BlendDesc& d)
{ return reinterpret_cast<const D3D11_BLEND_DESC&>(d); }

inline const D3D11_RASTERIZER_DESC& ToD3D11(const RasterizerDesc& d)
{ return reinterpret_cast<const D3D11_RASTERIZER_DESC&>(d); }

inline const D3D11_SAMPLER_DESC& ToD3D11(const SamplerDesc& d)
{ return reinterpret_cast<const D3D11_SAMPLER_DESC&>(d); }

inline const D3D11_INPUT_ELEMENT_DESC* ToD3D11(const InputElementDesc* d)
{ return reinterpret_cast<const D3D11_INPUT_ELEMENT_DESC*>(d); }

} // namespace EngineCore::RHI::DX11
