#pragma once
#include "RHITypes.h"

// IGfxDevice — creates and destroys GPU resources.
// One instance exists for the lifetime of the application.
// Call-sites use this to allocate buffers/textures/PSOs etc.

namespace EngineCore::RHI
{

// -----------------------------------------------------------------------
// Descriptions passed to creation functions

struct BufferDesc
{
	uint32_t    sizeBytes;
	BufferUsage usage;
	bool        allowSRV;    // can be bound as shader resource
	bool        allowUAV;    // can be bound as unordered access
	bool        isIndex;     // index buffer layout
	bool        isVertex;    // vertex buffer layout
	bool        isConstant;  // constant/uniform buffer layout
	uint32_t    structureStrideBytes; // >0 → structured buffer
	const void* initialData; // nullptr = zero-init
};

struct TextureDesc
{
	TextureDimension dimension;
	uint32_t    width;
	uint32_t    height;
	uint32_t    depth;      // slices for Tex2DArray / depth for Tex3D
	uint32_t    mipLevels;  // 0 = full chain
	DXGI_FORMAT format;
	bool        allowRTV;
	bool        allowDSV;
	bool        allowSRV;
	bool        allowUAV;
	bool        generateMips; // runtime mip generation
	uint32_t    msaaSamples;  // 1 = no MSAA
	uint32_t    msaaQuality;
	const void* initialData;
};

struct SamplerDesc
{
	// Thin wrapper over D3D11_SAMPLER_DESC / D3D12_STATIC_SAMPLER_DESC
	// (raw DX descriptor stored to keep migration incremental)
	D3D11_SAMPLER_DESC d3d11Desc; // shared layout with DX12 static sampler subset
};

// PSO descriptor: wraps existing D3D11 state descs + shader IDs.
// DX12 backend builds a D3D12_GRAPHICS_PIPELINE_STATE_DESC from this.
struct GraphicsPSODesc
{
	// Shader bytecode IDs (ShaderCodeMgr pool indices)
	uint16_t vertexShaderID;
	uint16_t hullShaderID;
	uint16_t domainShaderID;
	uint16_t geometryShaderID;
	uint16_t pixelShaderID;

	// For DX12: input layout is derived from vertex shader reflection
	GfxInputLayout* inputLayout;

	// Render state (stored as DX11 descs; DX12 backend converts)
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_BLEND_DESC         blendDesc;
	D3D11_RASTERIZER_DESC    rasterizerDesc;

	// Cached render state IDs (RenderStateMgr pool indices, used by DX11 backend)
	uint16_t depthStateID = 0;
	uint16_t blendStateID = 0;
	uint16_t rastStateID  = 0;

	// Render target formats (required for DX12 PSO creation)
	static constexpr uint32_t MaxRenderTargets = 8;
	DXGI_FORMAT renderTargetFormats[MaxRenderTargets];
	uint32_t    renderTargetCount;
	DXGI_FORMAT depthStencilFormat;

	uint32_t msaaSamples;
	uint32_t msaaQuality;

	Topology topology;
};

struct ComputePSODesc
{
	uint16_t computeShaderID;
};

// PSO descriptor for the mesh shader pipeline (AS + MS + PS).
// Replaces VS/HS/DS/GS in the mesh shader tier; no input assembler stage.
// Only usable when IGfxDevice::SupportsMeshShaders() returns true.
struct MeshShaderPSODesc
{
	// SHADER_NULL = stage not present (amplification shader is optional).
	uint16_t amplificationShaderID;
	uint16_t meshShaderID;
	uint16_t pixelShaderID;

	// Render state (stored as DX11 descs; DX12 backend converts)
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_BLEND_DESC         blendDesc;
	D3D11_RASTERIZER_DESC    rasterizerDesc;

	static constexpr uint32_t MaxRenderTargets = 8;
	DXGI_FORMAT renderTargetFormats[MaxRenderTargets];
	uint32_t    renderTargetCount;
	DXGI_FORMAT depthStencilFormat;

	uint32_t msaaSamples;
	uint32_t msaaQuality;
};

// -----------------------------------------------------------------------
// IGfxDevice interface

class IGfxDevice
{
public:
	virtual ~IGfxDevice() {}

	// Lifecycle
	virtual bool Init(HWND windowHandle) = 0;
	virtual void Shutdown() = 0;

	// -----------------------------------------------------------------------
	// Buffer creation / destruction

	virtual GfxBuffer* CreateBuffer(const BufferDesc& desc) = 0;
	virtual void       DestroyBuffer(GfxBuffer* buffer) = 0;

	// -----------------------------------------------------------------------
	// Texture creation / destruction

	virtual GfxTexture* CreateTexture(const TextureDesc& desc) = 0;
	virtual void        DestroyTexture(GfxTexture* texture) = 0;

	// -----------------------------------------------------------------------
	// View creation
	// Views are owned by the resource and destroyed with it, or manually.

	virtual GfxRTV* CreateRTV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mipSlice = 0,
		uint32_t arraySlice = 0) = 0;
	virtual GfxDSV* CreateDSV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mipSlice = 0) = 0;
	virtual GfxSRV* CreateSRV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mostDetailedMip = 0,
		uint32_t mipLevels = UINT32_MAX) = 0;
	virtual GfxSRV* CreateSRV(GfxBuffer* buffer, uint32_t firstElement, uint32_t numElements) = 0;
	virtual GfxUAV* CreateUAV(GfxTexture* texture, DXGI_FORMAT format, uint32_t mipSlice = 0) = 0;
	virtual GfxUAV* CreateUAV(GfxBuffer* buffer, uint32_t firstElement, uint32_t numElements) = 0;

	virtual void DestroyRTV(GfxRTV* rtv) = 0;
	virtual void DestroyDSV(GfxDSV* dsv) = 0;
	virtual void DestroyView(GfxSRV* srv) = 0;
	virtual void DestroyView(GfxUAV* uav) = 0;

	// -----------------------------------------------------------------------
	// Sampler creation

	virtual GfxSampler* CreateSampler(const SamplerDesc& desc) = 0;
	virtual void        DestroySampler(GfxSampler* sampler) = 0;

	// -----------------------------------------------------------------------
	// Pipeline state objects

	virtual GfxPipelineState* CreateGraphicsPSO(const GraphicsPSODesc& desc) = 0;
	virtual GfxPipelineState* CreateComputePSO(const ComputePSODesc& desc) = 0;
	// Mesh shader PSO (AS + MS + PS). Returns nullptr on DX11 or if device lacks mesh shader tier.
	virtual GfxPipelineState* CreateMeshShaderPSO(const MeshShaderPSODesc& desc)
	{ (void)desc; return nullptr; }
	virtual void              DestroyPSO(GfxPipelineState* pso) = 0;

	// -----------------------------------------------------------------------
	// Input layout

	virtual GfxInputLayout* CreateInputLayout(const void* shaderBytecode, size_t bytecodeSize,
		const D3D11_INPUT_ELEMENT_DESC* elements, uint32_t elementCount) = 0;
	virtual void            DestroyInputLayout(GfxInputLayout* layout) = 0;

	// -----------------------------------------------------------------------
	// Back buffer access (for render target setup)

	virtual GfxTexture* GetBackBuffer(uint32_t frameIndex) = 0;
	virtual uint32_t    GetBackBufferCount() = 0;
	virtual uint32_t    GetCurrentBackBufferIndex() = 0;
	virtual GfxRTV*     GetBackBufferRTV(uint32_t /*index*/) { return nullptr; }

	// -----------------------------------------------------------------------
	// Raytracing acceleration structures and pipelines (DXR — DX12 only).
	// DX11 backend: creation methods return nullptr; destroy methods are safe no-ops.

	virtual GfxBLAS* CreateBLAS(const BLASDesc& desc)
	{ (void)desc; return nullptr; }
	virtual void DestroyBLAS(GfxBLAS* blas) { delete blas; }

	virtual GfxTLAS* CreateTLAS(const TLASDesc& desc)
	{ (void)desc; return nullptr; }
	virtual void DestroyTLAS(GfxTLAS* tlas) { delete tlas; }

	virtual GfxRaytracingPipeline* CreateRaytracingPipeline(const RaytracingPipelineDesc& desc)
	{ (void)desc; return nullptr; }
	virtual void DestroyRaytracingPipeline(GfxRaytracingPipeline* pipeline) { delete pipeline; }

	// -----------------------------------------------------------------------
	// Query heaps (GPU timestamp profiling)

	virtual GfxQueryHeap* CreateQueryHeap(QueryType /*type*/, uint32_t /*count*/)
	{ return nullptr; }
	virtual void DestroyQueryHeap(GfxQueryHeap* heap) { delete heap; }

	// -----------------------------------------------------------------------
	// DDS texture loading (creates texture + SRV from raw DDS bytes)
	// DX11: returns nullptr (uses DDSTextureLoader directly)
	// DX12: parses DDS, creates committed resource, uploads all subresources, creates SRV

	virtual GfxSRV* LoadDDSFromMemory(const uint8_t* /*data*/, uint32_t /*dataSize*/) { return nullptr; }

	// -----------------------------------------------------------------------
	// Swap chain (deferred — HWND is not always available at device creation time)
	// DX11 backend: no-op (Window owns the swap chain).
	// DX12 backend: creates IDXGISwapChain4 inside DX12FrameScheduler.

	virtual bool InitSwapChain(HWND /*hwnd*/, uint32_t /*width*/, uint32_t /*height*/) { return true; }

	// -----------------------------------------------------------------------
	// Backend identification

	virtual bool IsDX12() const { return false; }

	// -----------------------------------------------------------------------
	// Diagnostics

	virtual const char* GetAdapterName() = 0;
	virtual bool        SupportsRaytracing() = 0;
	virtual bool        SupportsMeshShaders() = 0;
};

} // namespace EngineCore::RHI
