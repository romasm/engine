#pragma once
#include "RHITypes.h"

// IGfxCommandList — records GPU commands for one render pass.
// DX11 backend wraps the immediate context (single list per frame).
// DX12 backend wraps ID3D12GraphicsCommandList7 (multiple lists, parallel recording).

namespace EngineCore::RHI
{

class IGfxCommandList
{
public:
	virtual ~IGfxCommandList() {}

	// -----------------------------------------------------------------------
	// Frame lifecycle

	virtual void Open()  = 0;  // begin recording (reset allocator on DX12)
	virtual void Close() = 0;  // stop recording; ready for submission

	// -----------------------------------------------------------------------
	// Output merger

	virtual void SetRenderTargets(uint32_t count, GfxRTV* const* rtvs, GfxDSV* dsv) = 0;
	virtual void ClearRenderTarget(GfxRTV* rtv, float r, float g, float b, float a) = 0;
	virtual void ClearDepthStencil(GfxDSV* dsv, float depth, uint8_t stencil) = 0;
	virtual void ClearUAVFloat(GfxUAV* uav, float r, float g, float b, float a) = 0;
	virtual void ClearUAVUint(GfxUAV* uav, uint32_t r, uint32_t g, uint32_t b, uint32_t a) = 0;

	// -----------------------------------------------------------------------
	// Rasterizer

	virtual void SetViewport(const GfxViewport& viewport) = 0;
	virtual void SetTopology(Topology topology) = 0;

	// -----------------------------------------------------------------------
	// Pipeline state

	virtual void SetPipelineState(GfxPipelineState* pso) = 0;

	// Render state binding (DX11: dynamic state objects; DX12: baked into PSO — no-op)
	virtual void SetDepthStencilState(void* /*state*/, uint32_t /*stencilRef*/) {}
	virtual void SetBlendState(void* /*state*/, const float* /*blendFactor*/, uint32_t /*sampleMask*/) {}
	virtual void SetRasterizerState(void* /*state*/) {}

	// -----------------------------------------------------------------------
	// Input assembly

	virtual void SetVertexBuffer(uint32_t slot, GfxBuffer* buffer, uint32_t stride, uint32_t offset = 0) = 0;
	virtual void SetIndexBuffer(GfxBuffer* buffer, bool index32bit, uint32_t offset = 0) = 0;
	virtual void SetInputLayout(GfxInputLayout* layout) = 0;

	// -----------------------------------------------------------------------
	// Constant buffer binding (per stage, single slot)

	virtual void SetVSConstantBuffer(uint32_t slot, GfxBuffer* buffer) = 0;
	virtual void SetHSConstantBuffer(uint32_t slot, GfxBuffer* buffer) = 0;
	virtual void SetDSConstantBuffer(uint32_t slot, GfxBuffer* buffer) = 0;
	virtual void SetGSConstantBuffer(uint32_t slot, GfxBuffer* buffer) = 0;
	virtual void SetPSConstantBuffer(uint32_t slot, GfxBuffer* buffer) = 0;
	virtual void SetCSConstantBuffer(uint32_t slot, GfxBuffer* buffer) = 0;

	// Batch constant buffer binding (fills multiple slots starting at startSlot)
	virtual void SetVSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) = 0;
	virtual void SetHSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) = 0;
	virtual void SetDSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) = 0;
	virtual void SetGSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) = 0;
	virtual void SetPSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) = 0;
	virtual void SetCSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) = 0;

	// -----------------------------------------------------------------------
	// Shader resource binding (SRV, per stage, single slot)

	virtual void SetVSResource(uint32_t slot, GfxSRV* srv) = 0;
	virtual void SetHSResource(uint32_t slot, GfxSRV* srv) = 0;
	virtual void SetDSResource(uint32_t slot, GfxSRV* srv) = 0;
	virtual void SetGSResource(uint32_t slot, GfxSRV* srv) = 0;
	virtual void SetPSResource(uint32_t slot, GfxSRV* srv) = 0;
	virtual void SetCSResource(uint32_t slot, GfxSRV* srv) = 0;

	// Batch SRV bind (fills multiple slots starting at startSlot)
	virtual void SetVSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) = 0;
	virtual void SetHSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) = 0;
	virtual void SetDSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) = 0;
	virtual void SetGSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) = 0;
	virtual void SetPSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) = 0;
	virtual void SetCSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) = 0;

	// -----------------------------------------------------------------------
	// Unordered access binding (UAV)

	virtual void SetCSUnorderedAccessViews(uint32_t startSlot, uint32_t count, GfxUAV* const* uavs) = 0;
	// OM-bound UAVs (pixel shader writable)
	virtual void SetOMUnorderedAccessViews(uint32_t startSlot, uint32_t count, GfxUAV* const* uavs, const uint32_t* initialCounts) = 0;

	// -----------------------------------------------------------------------
	// Sampler binding (per stage, single slot)

	virtual void SetVSSampler(uint32_t slot, GfxSampler* sampler) = 0;
	virtual void SetHSSampler(uint32_t slot, GfxSampler* sampler) = 0;
	virtual void SetDSSampler(uint32_t slot, GfxSampler* sampler) = 0;
	virtual void SetGSSampler(uint32_t slot, GfxSampler* sampler) = 0;
	virtual void SetPSSampler(uint32_t slot, GfxSampler* sampler) = 0;
	virtual void SetCSSampler(uint32_t slot, GfxSampler* sampler) = 0;

	// Batch sampler binding (fills multiple slots starting at startSlot)
	virtual void SetVSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) = 0;
	virtual void SetHSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) = 0;
	virtual void SetDSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) = 0;
	virtual void SetGSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) = 0;
	virtual void SetPSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) = 0;
	virtual void SetCSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) = 0;

	// -----------------------------------------------------------------------
	// Draw and dispatch

	virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) = 0;
	virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
		uint32_t startVertex = 0, uint32_t startInstance = 0) = 0;
	virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
		uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0) = 0;

	virtual void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) = 0;

	// -----------------------------------------------------------------------
	// Dynamic resource updates

	// Write new data into a dynamic buffer (upload heap on DX12, Map/Unmap on DX11)
	virtual void UpdateBuffer(GfxBuffer* buffer, const void* data, size_t sizeBytes) = 0;

	// Write data into a default-usage (non-mappable) texture subresource
	virtual void UpdateSubresource(GfxTexture* texture, uint32_t subresource,
		const GfxBox* destBox, const void* data, uint32_t rowPitch, uint32_t depthPitch) = 0;

	// Write data into a default-usage (non-mappable) buffer subresource
	virtual void UpdateSubresource(GfxBuffer* buffer, uint32_t subresource,
		const GfxBox* destBox, const void* data, uint32_t rowPitch, uint32_t depthPitch) = 0;

	// -----------------------------------------------------------------------
	// Resource barriers (DX12: explicit transitions; DX11: no-op)

	virtual void TransitionBuffer(GfxBuffer* buffer, ResourceState before, ResourceState after) = 0;
	virtual void TransitionTexture(GfxTexture* texture, ResourceState before, ResourceState after) = 0;

	// Batch-flush all pending barriers (useful for DX12 barrier coalescing)
	virtual void FlushBarriers() = 0;

	// -----------------------------------------------------------------------
	// Copy / resolve

	virtual void CopyResource(GfxTexture* dest, GfxTexture* source) = 0;

	// Copy a subresource region between textures (per-mip, per-array-slice)
	virtual void CopyTextureRegion(GfxTexture* dest, uint32_t destSubresource,
		uint32_t destX, uint32_t destY, uint32_t destZ,
		GfxTexture* source, uint32_t sourceSubresource,
		const GfxBox* sourceBox) = 0;

	virtual void GenerateMips(GfxSRV* srv) = 0;

	// Resolve MSAA texture to a non-MSAA destination
	virtual void ResolveSubresource(GfxTexture* dest, uint32_t destSubresource,
		GfxTexture* source, uint32_t sourceSubresource, DXGI_FORMAT format) = 0;

	// -----------------------------------------------------------------------
	// Query operations (GPU profiling)

	virtual void BeginQuery(GfxQueryHeap* /*heap*/, uint32_t /*index*/) {}
	virtual void EndQuery(GfxQueryHeap* /*heap*/, uint32_t /*index*/) {}

	// Returns S_OK on success, S_FALSE if data not ready, or error HRESULT.
	virtual HRESULT GetQueryData(GfxQueryHeap* /*heap*/, uint32_t /*index*/,
		void* /*data*/, uint32_t /*dataSize*/, uint32_t /*flags*/ = 0)
	{ return E_NOTIMPL; }

	// -----------------------------------------------------------------------
	// Mesh shader dispatch (DX12 only; no-op on DX11)

	// Dispatch an AS+MS+PS pipeline.  Groups are launched in amplification
	// shader thread groups; each group can emit 0..1 mesh shader group.
	virtual void DispatchMesh(uint32_t /*groupsX*/, uint32_t /*groupsY*/, uint32_t /*groupsZ*/) {}

	// -----------------------------------------------------------------------
	// Raytracing commands (DXR — DX12 only; no-op on DX11)

	// Build or update a BLAS from its pre-created geometry buffers.
	// Must be called after all vertex/index buffers are in their final state.
	virtual void BuildBLAS(GfxBLAS* /*blas*/, bool /*update*/ = false) {}

	// Write per-frame instance descriptors into the TLAS upload buffer and
	// record the AS build command.  count must not exceed TLASDesc::maxInstances.
	virtual void BuildTLAS(GfxTLAS* /*tlas*/, const RaytracingInstance* /*instances*/,
	                        uint32_t /*count*/, bool /*update*/ = false) {}

	// Bind the raytracing pipeline and dispatch rays.
	virtual void DispatchRays(GfxRaytracingPipeline* /*pipeline*/,
	                           uint32_t /*width*/, uint32_t /*height*/,
	                           uint32_t /*depth*/ = 1) {}
};

} // namespace EngineCore::RHI
