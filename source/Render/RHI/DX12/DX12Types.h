#pragma once
#include "RHITypes.h"

// DX12 concrete resource types.
// Each wraps raw DX12 COM pointers behind the RHI base structs.
// High-level code uses GfxBuffer*/GfxTexture* etc.; only the DX12 backend
// casts to these concrete types to extract the underlying DX12 objects.

namespace EngineCore::RHI::DX12
{

// -----------------------------------------------------------------------
// Buffer

struct DX12Buffer : GfxBuffer
{
	ID3D12Resource*       resource        = nullptr;
	D3D12_RESOURCE_STATES currentState    = D3D12_RESOURCE_STATE_COMMON;
	uint32_t              sizeBytes       = 0;
	uint32_t              structureStride = 0; // >0 = structured buffer

	~DX12Buffer() override
	{
		if(resource) { resource->Release(); resource = nullptr; }
	}
};

// -----------------------------------------------------------------------
// Texture

struct DX12Texture : GfxTexture
{
	ID3D12Resource*       resource     = nullptr;
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;

	~DX12Texture() override
	{
		if(resource) { resource->Release(); resource = nullptr; }
	}
};

// -----------------------------------------------------------------------
// Views
// RTVs and DSVs live in CPU-only heaps; SRVs and UAVs live in the
// shader-visible bindless heap allocated in Phase 3.

struct DX12RTV : GfxRTV
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
	~DX12RTV() override {}
};

struct DX12DSV : GfxDSV
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
	~DX12DSV() override {}
};

struct DX12SRV : GfxSRV
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle  = {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle  = {};
	uint32_t                    heapIndex  = UINT32_MAX; // index in the bindless heap

	// Texture ownership is handled by base GfxSRV::sourceTexture + ownsSourceTexture.
	~DX12SRV() override {}
};

struct DX12UAV : GfxUAV
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle  = {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle  = {};
	uint32_t                    heapIndex  = UINT32_MAX;
	~DX12UAV() override {}
};

struct DX12Sampler : GfxSampler
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
	~DX12Sampler() override {}
};

// -----------------------------------------------------------------------
// Input layout
// DX12 PSOs embed the element descs directly; no separate runtime object.

struct DX12InputLayout : GfxInputLayout
{
	~DX12InputLayout() override {}
};

// -----------------------------------------------------------------------
// Pipeline state

struct DX12PipelineState : GfxPipelineState
{
	ID3D12PipelineState*  pso           = nullptr;
	ID3D12RootSignature*  rootSignature = nullptr;
	bool                  isCompute     = false;

	~DX12PipelineState() override
	{
		if(pso)           { pso->Release();           pso           = nullptr; }
		if(rootSignature) { rootSignature->Release();  rootSignature = nullptr; }
	}
};

// -----------------------------------------------------------------------
// Raytracing types

// DX12 bottom-level acceleration structure.
// CreateBLAS() allocates the result and scratch buffers.
// DX12CommandList::BuildBLAS() records the actual GPU build command.
struct DX12BLAS : GfxBLAS
{
	ID3D12Resource* resultBuffer  = nullptr;  // AS resource (RAYTRACING_ACCELERATION_STRUCTURE state)
	ID3D12Resource* scratchBuffer = nullptr;  // temporary UAV storage; kept alive for ALLOW_UPDATE

	// Stored geometry descriptor and build inputs so BuildBLAS() can issue the
	// build command without needing vertex/index buffer pointers again.
	// pGeometryDescs inside buildInputs points to the inline geomDesc field.
	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc   = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};

	~DX12BLAS() override
	{
		if(resultBuffer)  { resultBuffer->Release();  resultBuffer  = nullptr; }
		if(scratchBuffer) { scratchBuffer->Release(); scratchBuffer = nullptr; }
	}
};

// DX12 top-level acceleration structure.
// CreateTLAS() allocates result/scratch and a persistent upload-heap instance buffer.
// DX12CommandList::BuildTLAS() fills the instance buffer each frame and records the build.
struct DX12TLAS : GfxTLAS
{
	ID3D12Resource* resultBuffer   = nullptr;
	ID3D12Resource* scratchBuffer  = nullptr;
	ID3D12Resource* instanceBuffer = nullptr;  // upload heap; D3D12_RAYTRACING_INSTANCE_DESC[]
	uint32_t        maxInstances   = 0;

	// Template inputs (NumDescs and InstanceDescs are filled per-frame in BuildTLAS).
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputsTemplate = {};

	~DX12TLAS() override
	{
		if(resultBuffer)   { resultBuffer->Release();   resultBuffer   = nullptr; }
		if(scratchBuffer)  { scratchBuffer->Release();  scratchBuffer  = nullptr; }
		if(instanceBuffer) { instanceBuffer->Release(); instanceBuffer = nullptr; }
	}
};

// DX12 raytracing pipeline state object + shader table.
// Three fixed-size records: [0] raygen  [1] miss  [2] hit group.
struct DX12RaytracingPipeline : GfxRaytracingPipeline
{
	ID3D12StateObject*           stateObject = nullptr;
	ID3D12StateObjectProperties* properties  = nullptr;  // for GetShaderIdentifier()
	ID3D12Resource*              shaderTable = nullptr;  // upload-heap buffer

	uint32_t shaderRecordSize = 0;  // aligned stride per record (≥ 32 bytes identifier)
	uint64_t raygenOffset     = 0;  // byte offset of raygen record in shaderTable
	uint64_t missOffset       = 0;  // byte offset of miss record
	uint64_t hitGroupOffset   = 0;  // byte offset of hit-group record

	~DX12RaytracingPipeline() override
	{
		if(stateObject) { stateObject->Release(); stateObject = nullptr; }
		if(properties)  { properties->Release();  properties  = nullptr; }
		if(shaderTable) { shaderTable->Release(); shaderTable = nullptr; }
	}
};

// -----------------------------------------------------------------------
// Helper casts (safe — callers only use these inside DX12 backend code)

inline DX12Buffer*               Cast(GfxBuffer*               p) { return static_cast<DX12Buffer*>(p); }
inline DX12Texture*              Cast(GfxTexture*              p) { return static_cast<DX12Texture*>(p); }
inline DX12RTV*                  Cast(GfxRTV*                  p) { return static_cast<DX12RTV*>(p); }
inline DX12DSV*                  Cast(GfxDSV*                  p) { return static_cast<DX12DSV*>(p); }
inline DX12SRV*                  Cast(GfxSRV*                  p) { return static_cast<DX12SRV*>(p); }
inline DX12UAV*                  Cast(GfxUAV*                  p) { return static_cast<DX12UAV*>(p); }
inline DX12Sampler*              Cast(GfxSampler*              p) { return static_cast<DX12Sampler*>(p); }
inline DX12InputLayout*          Cast(GfxInputLayout*          p) { return static_cast<DX12InputLayout*>(p); }
inline DX12PipelineState*        Cast(GfxPipelineState*        p) { return static_cast<DX12PipelineState*>(p); }
inline DX12BLAS*                 Cast(GfxBLAS*                 p) { return static_cast<DX12BLAS*>(p); }
inline DX12TLAS*                 Cast(GfxTLAS*                 p) { return static_cast<DX12TLAS*>(p); }
inline DX12RaytracingPipeline*   Cast(GfxRaytracingPipeline*   p) { return static_cast<DX12RaytracingPipeline*>(p); }

} // namespace EngineCore::RHI::DX12
