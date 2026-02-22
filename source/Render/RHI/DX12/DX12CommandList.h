#pragma once
#include "GfxCommandList.h"
#include "DX12Types.h"

// DX12 command list — wraps ID3D12GraphicsCommandList7.
// Per-frame command allocators avoid CPU/GPU synchronization on Reset().
// Resource binding (constant buffers, SRVs, UAVs) is NYI pending root
// signature and descriptor heap design in Phase 4.

namespace EngineCore::RHI::DX12
{

class DX12CommandList : public IGfxCommandList
{
public:
	// Called once at startup to create the command list and per-frame allocators.
	bool Init(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type,
	          uint32_t framesInFlight)
	{
		m_device       = device;
		m_framesInFlight = framesInFlight;
		m_allocators.resize(framesInFlight);

		for(uint32_t i = 0; i < framesInFlight; ++i)
		{
			HRESULT hr = device->CreateCommandAllocator(type,
				IID_PPV_ARGS(&m_allocators[i]));
			if(FAILED(hr)) return false;
		}

		// Start closed; Open() resets and opens.
		HRESULT hr = device->CreateCommandList(0, type,
			m_allocators[0], nullptr, IID_PPV_ARGS(&m_commandList));
		if(FAILED(hr)) return false;
		m_commandList->Close();

		return true;
	}

	void Destroy()
	{
		if(m_commandList) { m_commandList->Release(); m_commandList = nullptr; }
		for(auto* alloc : m_allocators)
			if(alloc) alloc->Release();
		m_allocators.clear();
	}

	~DX12CommandList() override { Destroy(); }

	// Called by DX12FrameScheduler::BeginFrame() after waiting for the
	// previous frame's GPU work to complete. Resets the allocator for
	// the given frame index so commands can be recorded into it.
	void PrepareForFrame(uint32_t frameIndex)
	{
		m_currentFrameIndex = frameIndex;
		m_allocators[frameIndex]->Reset();
	}

	ID3D12GraphicsCommandList7* GetCommandList() const { return m_commandList; }

	// Configure the dynamic descriptor ring for SRV/UAV binding.
	// Called once after Init() by the frame scheduler.
	void InitBindingState(ID3D12DescriptorHeap* cbvSrvUavHeap,
	                      uint32_t descSize,
	                      uint32_t ringStart, uint32_t ringSize,
	                      D3D12_CPU_DESCRIPTOR_HANDLE nullSrvCpu,
	                      D3D12_CPU_DESCRIPTOR_HANDLE nullUavCpu,
	                      ID3D12RootSignature* graphicsRootSig,
	                      ID3D12RootSignature* computeRootSig)
	{
		m_cbvSrvUavHeap     = cbvSrvUavHeap;
		m_cbvSrvUavDescSize = descSize;
		m_ringStart         = ringStart;
		m_ringSize          = ringSize;
		m_nullSrvCpu        = nullSrvCpu;
		m_nullUavCpu        = nullUavCpu;
		m_graphicsRootSig   = graphicsRootSig;
		m_computeRootSig    = computeRootSig;
	}

	// -----------------------------------------------------------------------
	// Lifecycle

	void Open() override
	{
		m_commandList->Reset(m_allocators[m_currentFrameIndex], nullptr);
		m_pendingBarriers.clear();

		// Reset descriptor ring and binding state for the new frame.
		m_ringOffset  = 0;
		m_maxSRVSlot  = 0;
		m_maxUAVSlot  = 0;
		m_srvDirty    = false;
		m_uavDirty    = false;
		m_isCompute   = false;
		memset(m_boundSRVs, 0, sizeof(m_boundSRVs));
		memset(m_boundUAVs, 0, sizeof(m_boundUAVs));

		// Set default root signature so CBV/SRV/UAV binding is always valid.
		if(m_graphicsRootSig)
			m_commandList->SetGraphicsRootSignature(m_graphicsRootSig);
	}

	void Close() override
	{
		FlushBarriers();
		m_commandList->Close();
	}

	// -----------------------------------------------------------------------
	// Output merger

	void SetRenderTargets(uint32_t count, GfxRTV* const* rtvs, GfxDSV* dsv) override
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dRTVs[8] = {};
		for(uint32_t i = 0; i < count && i < 8; ++i)
			d3dRTVs[i] = rtvs[i] ? Cast(rtvs[i])->handle : D3D12_CPU_DESCRIPTOR_HANDLE{};

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
			dsv ? Cast(dsv)->handle : D3D12_CPU_DESCRIPTOR_HANDLE{};

		m_commandList->OMSetRenderTargets(count, d3dRTVs, FALSE, dsv ? &dsvHandle : nullptr);
	}

	void ClearRenderTarget(GfxRTV* rtv, float r, float g, float b, float a) override
	{
		const float color[4] = { r, g, b, a };
		m_commandList->ClearRenderTargetView(Cast(rtv)->handle, color, 0, nullptr);
	}

	void ClearDepthStencil(GfxDSV* dsv, float depth, uint8_t stencil) override
	{
		m_commandList->ClearDepthStencilView(Cast(dsv)->handle,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			depth, stencil, 0, nullptr);
	}

	void ClearUAVFloat(GfxUAV* uav, float r, float g, float b, float a) override
	{
		// Phase 4: requires shader-visible heap handle
		(void)uav; (void)r; (void)g; (void)b; (void)a;
	}

	void ClearUAVUint(GfxUAV* uav, uint32_t r, uint32_t g, uint32_t b, uint32_t a) override
	{
		// Phase 4: requires shader-visible heap handle
		(void)uav; (void)r; (void)g; (void)b; (void)a;
	}

	// -----------------------------------------------------------------------
	// Rasterizer

	void SetViewport(const GfxViewport& vp) override
	{
		D3D12_VIEWPORT d3dVP;
		d3dVP.TopLeftX = vp.topLeftX;
		d3dVP.TopLeftY = vp.topLeftY;
		d3dVP.Width    = vp.width;
		d3dVP.Height   = vp.height;
		d3dVP.MinDepth = vp.minDepth;
		d3dVP.MaxDepth = vp.maxDepth;
		m_commandList->RSSetViewports(1, &d3dVP);

		// DX12 also requires a scissor rect; default to full viewport.
		D3D12_RECT scissor = { 0, 0,
			static_cast<LONG>(vp.topLeftX + vp.width),
			static_cast<LONG>(vp.topLeftY + vp.height) };
		m_commandList->RSSetScissorRects(1, &scissor);
	}

	void SetTopology(Topology topo) override
	{
		m_commandList->IASetPrimitiveTopology(
			static_cast<D3D12_PRIMITIVE_TOPOLOGY>(topo));
	}

	// -----------------------------------------------------------------------
	// Pipeline state — implemented in DX12CommandList.cpp (needs ShaderCodeMgr)

	void SetPipelineState(GfxPipelineState* pso) override;

	// -----------------------------------------------------------------------
	// Input assembly

	void SetVertexBuffer(uint32_t slot, GfxBuffer* buffer, uint32_t stride,
	                     uint32_t offset = 0) override
	{
		auto* dx12Buffer = Cast(buffer);
		D3D12_VERTEX_BUFFER_VIEW view;
		view.BufferLocation = dx12Buffer->resource->GetGPUVirtualAddress() + offset;
		view.SizeInBytes    = static_cast<UINT>(
			dx12Buffer->resource->GetDesc().Width - offset);
		view.StrideInBytes  = stride;
		m_commandList->IASetVertexBuffers(slot, 1, &view);
	}

	void SetIndexBuffer(GfxBuffer* buffer, bool index32bit,
	                    uint32_t offset = 0) override
	{
		auto* dx12Buffer = Cast(buffer);
		D3D12_INDEX_BUFFER_VIEW view;
		view.BufferLocation = dx12Buffer->resource->GetGPUVirtualAddress() + offset;
		view.SizeInBytes    = static_cast<UINT>(
			dx12Buffer->resource->GetDesc().Width - offset);
		view.Format         = index32bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		m_commandList->IASetIndexBuffer(&view);
	}

	void SetInputLayout(GfxInputLayout* /*layout*/) override {}  // baked into PSO on DX12

	// -----------------------------------------------------------------------
	// Constant buffers — bound as root CBVs (GPU virtual address, 2 DWORDs each).
	// Root param indices must match DX12Device::GfxRP_* / CsRP_* constants.

	void SetVSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{
		if(slot >= 4) return; // VS b0-b3
		D3D12_GPU_VIRTUAL_ADDRESS addr = buffer ? Cast(buffer)->resource->GetGPUVirtualAddress() : 0;
		m_commandList->SetGraphicsRootConstantBufferView(RP_VS_CBV + slot, addr);
	}
	void SetHSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{
		if(slot > 0) return; // HS b0 only
		D3D12_GPU_VIRTUAL_ADDRESS addr = buffer ? Cast(buffer)->resource->GetGPUVirtualAddress() : 0;
		m_commandList->SetGraphicsRootConstantBufferView(RP_HS_CBV, addr);
	}
	void SetDSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{
		if(slot > 0) return; // DS b0 only
		D3D12_GPU_VIRTUAL_ADDRESS addr = buffer ? Cast(buffer)->resource->GetGPUVirtualAddress() : 0;
		m_commandList->SetGraphicsRootConstantBufferView(RP_DS_CBV, addr);
	}
	void SetGSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{
		if(slot > 0) return; // GS b0 only
		D3D12_GPU_VIRTUAL_ADDRESS addr = buffer ? Cast(buffer)->resource->GetGPUVirtualAddress() : 0;
		m_commandList->SetGraphicsRootConstantBufferView(RP_GS_CBV, addr);
	}
	void SetPSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{
		if(slot >= 10) return; // PS b0-b9
		D3D12_GPU_VIRTUAL_ADDRESS addr = buffer ? Cast(buffer)->resource->GetGPUVirtualAddress() : 0;
		m_commandList->SetGraphicsRootConstantBufferView(RP_PS_CBV + slot, addr);
	}
	void SetCSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{
		if(slot >= 4) return; // CS b0-b3
		D3D12_GPU_VIRTUAL_ADDRESS addr = buffer ? Cast(buffer)->resource->GetGPUVirtualAddress() : 0;
		m_commandList->SetComputeRootConstantBufferView(RP_CS_CBV + slot, addr);
	}

	void SetVSConstantBuffers(uint32_t start, uint32_t count, GfxBuffer* const* buffers) override
	{ for(uint32_t i = 0; i < count; ++i) SetVSConstantBuffer(start + i, buffers[i]); }
	void SetHSConstantBuffers(uint32_t start, uint32_t count, GfxBuffer* const* buffers) override
	{ for(uint32_t i = 0; i < count; ++i) SetHSConstantBuffer(start + i, buffers[i]); }
	void SetDSConstantBuffers(uint32_t start, uint32_t count, GfxBuffer* const* buffers) override
	{ for(uint32_t i = 0; i < count; ++i) SetDSConstantBuffer(start + i, buffers[i]); }
	void SetGSConstantBuffers(uint32_t start, uint32_t count, GfxBuffer* const* buffers) override
	{ for(uint32_t i = 0; i < count; ++i) SetGSConstantBuffer(start + i, buffers[i]); }
	void SetPSConstantBuffers(uint32_t start, uint32_t count, GfxBuffer* const* buffers) override
	{ for(uint32_t i = 0; i < count; ++i) SetPSConstantBuffer(start + i, buffers[i]); }
	void SetCSConstantBuffers(uint32_t start, uint32_t count, GfxBuffer* const* buffers) override
	{ for(uint32_t i = 0; i < count; ++i) SetCSConstantBuffer(start + i, buffers[i]); }

	// -----------------------------------------------------------------------
	// Shader resources — dirty-tracked; flushed to contiguous descriptor
	// ring region before each Draw/Dispatch via FlushDescriptorTables().

	void SetVSResource(uint32_t slot, GfxSRV* srv) override { BindSRV(slot, srv); }
	void SetHSResource(uint32_t slot, GfxSRV* srv) override { BindSRV(slot, srv); }
	void SetDSResource(uint32_t slot, GfxSRV* srv) override { BindSRV(slot, srv); }
	void SetGSResource(uint32_t slot, GfxSRV* srv) override { BindSRV(slot, srv); }
	void SetPSResource(uint32_t slot, GfxSRV* srv) override { BindSRV(slot, srv); }
	void SetCSResource(uint32_t slot, GfxSRV* srv) override { BindSRV(slot, srv); }

	void SetVSResources(uint32_t start, uint32_t count, GfxSRV* const* srvs) override
	{ for(uint32_t i = 0; i < count; ++i) BindSRV(start + i, srvs[i]); }
	void SetHSResources(uint32_t start, uint32_t count, GfxSRV* const* srvs) override
	{ for(uint32_t i = 0; i < count; ++i) BindSRV(start + i, srvs[i]); }
	void SetDSResources(uint32_t start, uint32_t count, GfxSRV* const* srvs) override
	{ for(uint32_t i = 0; i < count; ++i) BindSRV(start + i, srvs[i]); }
	void SetGSResources(uint32_t start, uint32_t count, GfxSRV* const* srvs) override
	{ for(uint32_t i = 0; i < count; ++i) BindSRV(start + i, srvs[i]); }
	void SetPSResources(uint32_t start, uint32_t count, GfxSRV* const* srvs) override
	{ for(uint32_t i = 0; i < count; ++i) BindSRV(start + i, srvs[i]); }
	void SetCSResources(uint32_t start, uint32_t count, GfxSRV* const* srvs) override
	{ for(uint32_t i = 0; i < count; ++i) BindSRV(start + i, srvs[i]); }

	// -----------------------------------------------------------------------
	// UAVs — dirty-tracked like SRVs.

	void SetCSUnorderedAccessViews(uint32_t start, uint32_t count,
	                               GfxUAV* const* uavs) override
	{ for(uint32_t i = 0; i < count; ++i) BindUAV(start + i, uavs[i]); }

	void SetOMUnorderedAccessViews(uint32_t start, uint32_t count,
	                               GfxUAV* const* uavs,
	                               const uint32_t* /*initialCounts*/) override
	{ for(uint32_t i = 0; i < count; ++i) BindUAV(start + i, uavs[i]); }

	// -----------------------------------------------------------------------
	// Samplers — Phase 4: static samplers embedded in root signature.

	void SetVSSampler(uint32_t /*slot*/, GfxSampler* /*sampler*/) override {}
	void SetHSSampler(uint32_t /*slot*/, GfxSampler* /*sampler*/) override {}
	void SetDSSampler(uint32_t /*slot*/, GfxSampler* /*sampler*/) override {}
	void SetGSSampler(uint32_t /*slot*/, GfxSampler* /*sampler*/) override {}
	void SetPSSampler(uint32_t /*slot*/, GfxSampler* /*sampler*/) override {}
	void SetCSSampler(uint32_t /*slot*/, GfxSampler* /*sampler*/) override {}

	void SetVSSamplers(uint32_t /*start*/, uint32_t /*count*/, GfxSampler* const* /*samplers*/) override {}
	void SetHSSamplers(uint32_t /*start*/, uint32_t /*count*/, GfxSampler* const* /*samplers*/) override {}
	void SetDSSamplers(uint32_t /*start*/, uint32_t /*count*/, GfxSampler* const* /*samplers*/) override {}
	void SetGSSamplers(uint32_t /*start*/, uint32_t /*count*/, GfxSampler* const* /*samplers*/) override {}
	void SetPSSamplers(uint32_t /*start*/, uint32_t /*count*/, GfxSampler* const* /*samplers*/) override {}
	void SetCSSamplers(uint32_t /*start*/, uint32_t /*count*/, GfxSampler* const* /*samplers*/) override {}

	// -----------------------------------------------------------------------
	// Draw / dispatch

	void Draw(uint32_t vertexCount, uint32_t startVertex = 0) override
	{ FlushDescriptorTables(false); m_commandList->DrawInstanced(vertexCount, 1, startVertex, 0); }

	void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0,
	                 int32_t baseVertex = 0) override
	{ FlushDescriptorTables(false); m_commandList->DrawIndexedInstanced(indexCount, 1, startIndex, baseVertex, 0); }

	void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
	                   uint32_t startVertex = 0, uint32_t startInstance = 0) override
	{ FlushDescriptorTables(false); m_commandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance); }

	void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
	                          uint32_t startIndex = 0, int32_t baseVertex = 0,
	                          uint32_t startInstance = 0) override
	{ FlushDescriptorTables(false); m_commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance); }

	void Dispatch(uint32_t x, uint32_t y, uint32_t z) override
	{ FlushDescriptorTables(true); m_commandList->Dispatch(x, y, z); }

	// Dispatch an amplification+mesh+pixel shader pipeline.
	// ID3D12GraphicsCommandList6::DispatchMesh is available on commandList7.
	void DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override
	{ FlushDescriptorTables(false); m_commandList->DispatchMesh(groupsX, groupsY, groupsZ); }

	// -----------------------------------------------------------------------
	// Dynamic updates
	// DX12: dynamic (upload-heap) buffers can be mapped directly.
	// Default-heap buffers require a staging copy (Phase 3 ring buffer).

	void UpdateBuffer(GfxBuffer* buffer, const void* data, size_t sizeBytes) override
	{
		auto* dx12Buffer = Cast(buffer);
		void* mapped = nullptr;
		D3D12_RANGE readRange = { 0, 0 }; // CPU is not reading back
		if(SUCCEEDED(dx12Buffer->resource->Map(0, &readRange, &mapped)))
		{
			memcpy(mapped, data, sizeBytes);
			dx12Buffer->resource->Unmap(0, nullptr);
		}
	}

	void UpdateSubresource(GfxTexture* /*texture*/, uint32_t /*subresource*/,
		const GfxBox* /*destBox*/, const void* /*data*/,
		uint32_t /*rowPitch*/, uint32_t /*depthPitch*/) override
	{
		// DX12: requires staging upload buffer + CopyTextureRegion. NYI.
	}

	void UpdateSubresource(GfxBuffer* /*buffer*/, uint32_t /*subresource*/,
		const GfxBox* /*destBox*/, const void* /*data*/,
		uint32_t /*rowPitch*/, uint32_t /*depthPitch*/) override
	{
		// DX12: requires staging upload buffer + CopyBufferRegion. NYI.
	}

	// -----------------------------------------------------------------------
	// Resource barriers (explicit on DX12)

	void TransitionBuffer(GfxBuffer* buffer,
	                      ResourceState before, ResourceState after) override
	{
		auto* dx12Buffer = Cast(buffer);
		m_pendingBarriers.push_back(
			CD3DX12_RESOURCE_BARRIER::Transition(
				dx12Buffer->resource,
				ToD3D12State(before), ToD3D12State(after)));
		dx12Buffer->currentState = ToD3D12State(after);
	}

	void TransitionTexture(GfxTexture* texture,
	                       ResourceState before, ResourceState after) override
	{
		auto* dx12Tex = Cast(texture);
		m_pendingBarriers.push_back(
			CD3DX12_RESOURCE_BARRIER::Transition(
				dx12Tex->resource,
				ToD3D12State(before), ToD3D12State(after)));
		dx12Tex->currentState = ToD3D12State(after);
	}

	void FlushBarriers() override
	{
		if(m_pendingBarriers.empty())
			return;
		m_commandList->ResourceBarrier(
			static_cast<uint32_t>(m_pendingBarriers.size()),
			m_pendingBarriers.data());
		m_pendingBarriers.clear();
	}

	// -----------------------------------------------------------------------
	// Copy / resolve

	void CopyResource(GfxTexture* dest, GfxTexture* source) override
	{
		m_commandList->CopyResource(
			Cast(dest)->resource, Cast(source)->resource);
	}

	void GenerateMips(GfxSRV* /*srv*/) override
	{
		// DX12 has no built-in GenerateMips; Phase 4 implements this via a
		// compute pass (similar to DirectXTex GenerateMipMaps3D approach).
	}

	void ResolveSubresource(GfxTexture* dest, uint32_t destSubresource,
		GfxTexture* source, uint32_t sourceSubresource, DXGI_FORMAT format) override
	{
		m_commandList->ResolveSubresource(
			Cast(dest)->resource, destSubresource,
			Cast(source)->resource, sourceSubresource, format);
	}

	// -----------------------------------------------------------------------
	// Raytracing commands (Phase 6)

	void BuildBLAS(GfxBLAS* blas, bool update = false) override
	{
		auto* dx12blas = Cast(blas);
		if(!dx12blas || !dx12blas->resultBuffer) return;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		buildDesc.DestAccelerationStructureData    = dx12blas->resultBuffer->GetGPUVirtualAddress();
		buildDesc.Inputs                           = dx12blas->buildInputs;
		buildDesc.ScratchAccelerationStructureData = dx12blas->scratchBuffer->GetGPUVirtualAddress();

		if(update)
		{
			buildDesc.Inputs.Flags |=
			    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			buildDesc.SourceAccelerationStructureData =
			    dx12blas->resultBuffer->GetGPUVirtualAddress();
		}

		m_commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		// UAV barrier ensures the BLAS is fully written before any TLAS reads it.
		D3D12_RESOURCE_BARRIER barrier =
		    CD3DX12_RESOURCE_BARRIER::UAV(dx12blas->resultBuffer);
		m_commandList->ResourceBarrier(1, &barrier);
	}

	void BuildTLAS(GfxTLAS* tlas, const RaytracingInstance* instances,
	               uint32_t count, bool update = false) override
	{
		auto* dx12tlas = Cast(tlas);
		if(!dx12tlas || !dx12tlas->resultBuffer || !instances || count == 0) return;

		const uint32_t actualCount = min(count, dx12tlas->maxInstances);

		// Write instance descriptors into the upload buffer for this frame.
		D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = nullptr;
		D3D12_RANGE readRange = { 0, 0 };
		if(FAILED(dx12tlas->instanceBuffer->Map(0, &readRange,
		    reinterpret_cast<void**>(&instanceDescs))))
			return;

		for(uint32_t i = 0; i < actualCount; ++i)
		{
			const auto& src = instances[i];
			auto&       dst = instanceDescs[i];
			memcpy(dst.Transform, src.transform, sizeof(dst.Transform));
			dst.InstanceID                          = src.instanceID & 0x00FFFFFF;
			dst.InstanceMask                        = src.mask;
			dst.InstanceContributionToHitGroupIndex = src.hitGroupIndex & 0x00FFFFFF;
			dst.Flags                               = src.flags;
			dst.AccelerationStructure               = src.blas
			    ? Cast(src.blas)->resultBuffer->GetGPUVirtualAddress()
			    : 0;
		}

		dx12tlas->instanceBuffer->Unmap(0, nullptr);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs =
		    dx12tlas->buildInputsTemplate;
		inputs.NumDescs      = actualCount;
		inputs.InstanceDescs = dx12tlas->instanceBuffer->GetGPUVirtualAddress();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		buildDesc.DestAccelerationStructureData    = dx12tlas->resultBuffer->GetGPUVirtualAddress();
		buildDesc.Inputs                           = inputs;
		buildDesc.ScratchAccelerationStructureData = dx12tlas->scratchBuffer->GetGPUVirtualAddress();

		if(update)
		{
			buildDesc.Inputs.Flags |=
			    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			buildDesc.SourceAccelerationStructureData =
			    dx12tlas->resultBuffer->GetGPUVirtualAddress();
		}

		m_commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		D3D12_RESOURCE_BARRIER barrier =
		    CD3DX12_RESOURCE_BARRIER::UAV(dx12tlas->resultBuffer);
		m_commandList->ResourceBarrier(1, &barrier);
	}

	void DispatchRays(GfxRaytracingPipeline* pipeline,
	                  uint32_t width, uint32_t height, uint32_t depth = 1) override
	{
		auto* rtPipeline = Cast(pipeline);
		if(!rtPipeline || !rtPipeline->stateObject) return;

		m_commandList->SetPipelineState1(rtPipeline->stateObject);

		const D3D12_GPU_VIRTUAL_ADDRESS tableBase =
		    rtPipeline->shaderTable->GetGPUVirtualAddress();
		const uint64_t recordSize = rtPipeline->shaderRecordSize;

		D3D12_DISPATCH_RAYS_DESC rayDesc = {};
		rayDesc.RayGenerationShaderRecord.StartAddress = tableBase + rtPipeline->raygenOffset;
		rayDesc.RayGenerationShaderRecord.SizeInBytes  = recordSize;
		rayDesc.MissShaderTable.StartAddress           = tableBase + rtPipeline->missOffset;
		rayDesc.MissShaderTable.SizeInBytes            = recordSize;
		rayDesc.MissShaderTable.StrideInBytes          = recordSize;
		rayDesc.HitGroupTable.StartAddress             = tableBase + rtPipeline->hitGroupOffset;
		rayDesc.HitGroupTable.SizeInBytes              = recordSize;
		rayDesc.HitGroupTable.StrideInBytes            = recordSize;
		rayDesc.Width  = width;
		rayDesc.Height = height;
		rayDesc.Depth  = depth;

		m_commandList->DispatchRays(&rayDesc);
	}

private:
	// Convert RHI resource state enum to the DX12 flag set.
	static D3D12_RESOURCE_STATES ToD3D12State(ResourceState state)
	{
		using RS = ResourceState;
		if(state == RS::Common) return D3D12_RESOURCE_STATE_COMMON;

		D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;
		const uint32_t bits = static_cast<uint32_t>(state);

		if(bits & static_cast<uint32_t>(RS::VertexBuffer))
			result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if(bits & static_cast<uint32_t>(RS::IndexBuffer))
			result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if(bits & static_cast<uint32_t>(RS::RenderTarget))
			result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if(bits & static_cast<uint32_t>(RS::UnorderedAccess))
			result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if(bits & static_cast<uint32_t>(RS::DepthWrite))
			result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if(bits & static_cast<uint32_t>(RS::DepthRead))
			result |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if(bits & static_cast<uint32_t>(RS::ShaderResource))
			result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			        | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		if(bits & static_cast<uint32_t>(RS::IndirectArgument))
			result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if(bits & static_cast<uint32_t>(RS::CopyDest))
			result |= D3D12_RESOURCE_STATE_COPY_DEST;
		if(bits & static_cast<uint32_t>(RS::CopySource))
			result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if(bits & static_cast<uint32_t>(RS::Present))
			result |= D3D12_RESOURCE_STATE_PRESENT;
		if(bits & static_cast<uint32_t>(RS::AccelerationStructure))
			result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		return result;
	}

	// Root parameter indices (must match DX12Device::GfxRP_* / CsRP_* constants).
	static constexpr uint32_t RP_VS_CBV = 0;   // params 0-3
	static constexpr uint32_t RP_PS_CBV = 4;   // params 4-13
	static constexpr uint32_t RP_GS_CBV = 14;
	static constexpr uint32_t RP_HS_CBV = 15;
	static constexpr uint32_t RP_DS_CBV = 16;
	static constexpr uint32_t RP_GFX_SRV_Table = 17;
	static constexpr uint32_t RP_GFX_UAV_Table = 18;
	static constexpr uint32_t RP_CS_CBV = 0;   // params 0-3
	static constexpr uint32_t RP_CS_SRV_Table  = 4;
	static constexpr uint32_t RP_CS_UAV_Table  = 5;

	// SRV/UAV dirty-tracking arrays.  All stages share one namespace (the
	// descriptor table uses VISIBILITY_ALL).
	static constexpr uint32_t MaxSRV = 128;
	static constexpr uint32_t MaxUAV = 32;
	GfxSRV*  m_boundSRVs[MaxSRV] = {};
	GfxUAV*  m_boundUAVs[MaxUAV] = {};
	uint32_t m_maxSRVSlot = 0;
	uint32_t m_maxUAVSlot = 0;
	bool     m_srvDirty   = false;
	bool     m_uavDirty   = false;
	bool     m_isCompute  = false;

	// Dynamic descriptor ring — per-frame region within the shader-visible
	// CBV/SRV/UAV heap.  Ring resets each frame in Open().
	uint32_t              m_ringStart          = 0;  // first slot of dynamic region
	uint32_t              m_ringSize           = 0;  // total slots in dynamic region
	uint32_t              m_ringOffset         = 0;  // next free slot within ring
	uint32_t              m_cbvSrvUavDescSize  = 0;
	ID3D12DescriptorHeap* m_cbvSrvUavHeap     = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_nullSrvCpu  = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_nullUavCpu  = {};

	void BindSRV(uint32_t slot, GfxSRV* srv)
	{
		if(slot >= MaxSRV) return;
		m_boundSRVs[slot] = srv;
		if(slot + 1 > m_maxSRVSlot) m_maxSRVSlot = slot + 1;
		m_srvDirty = true;
	}

	void BindUAV(uint32_t slot, GfxUAV* uav)
	{
		if(slot >= MaxUAV) return;
		m_boundUAVs[slot] = uav;
		if(slot + 1 > m_maxUAVSlot) m_maxUAVSlot = slot + 1;
		m_uavDirty = true;
	}

	// Copy bound SRV/UAV descriptors into a contiguous ring region and bind
	// the descriptor table to the command list.
	void FlushDescriptorTables(bool isCompute)
	{
		if(!m_cbvSrvUavHeap) return;

		if(m_srvDirty && m_maxSRVSlot > 0)
		{
			uint32_t count = m_maxSRVSlot;
			if(m_ringOffset + count > m_ringSize) return; // ring exhausted

			uint32_t heapSlot = m_ringStart + m_ringOffset;
			D3D12_CPU_DESCRIPTOR_HANDLE dstStart = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
			dstStart.ptr += static_cast<SIZE_T>(heapSlot) * m_cbvSrvUavDescSize;

			for(uint32_t i = 0; i < count; ++i)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE src = m_boundSRVs[i]
					? Cast(m_boundSRVs[i])->cpuHandle
					: m_nullSrvCpu;
				D3D12_CPU_DESCRIPTOR_HANDLE dst = dstStart;
				dst.ptr += static_cast<SIZE_T>(i) * m_cbvSrvUavDescSize;
				m_device->CopyDescriptorsSimple(1, dst, src,
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}

			D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
			gpuStart.ptr += static_cast<UINT64>(heapSlot) * m_cbvSrvUavDescSize;

			if(isCompute)
				m_commandList->SetComputeRootDescriptorTable(RP_CS_SRV_Table, gpuStart);
			else
				m_commandList->SetGraphicsRootDescriptorTable(RP_GFX_SRV_Table, gpuStart);

			m_ringOffset += count;
			m_srvDirty = false;
		}

		if(m_uavDirty && m_maxUAVSlot > 0)
		{
			uint32_t count = m_maxUAVSlot;
			if(m_ringOffset + count > m_ringSize) return; // ring exhausted

			uint32_t heapSlot = m_ringStart + m_ringOffset;
			D3D12_CPU_DESCRIPTOR_HANDLE dstStart = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
			dstStart.ptr += static_cast<SIZE_T>(heapSlot) * m_cbvSrvUavDescSize;

			for(uint32_t i = 0; i < count; ++i)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE src = m_boundUAVs[i]
					? Cast(m_boundUAVs[i])->cpuHandle
					: m_nullUavCpu;
				D3D12_CPU_DESCRIPTOR_HANDLE dst = dstStart;
				dst.ptr += static_cast<SIZE_T>(i) * m_cbvSrvUavDescSize;
				m_device->CopyDescriptorsSimple(1, dst, src,
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}

			D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
			gpuStart.ptr += static_cast<UINT64>(heapSlot) * m_cbvSrvUavDescSize;

			if(isCompute)
				m_commandList->SetComputeRootDescriptorTable(RP_CS_UAV_Table, gpuStart);
			else
				m_commandList->SetGraphicsRootDescriptorTable(RP_GFX_UAV_Table, gpuStart);

			m_ringOffset += count;
			m_uavDirty = false;
		}
	}

	// Root signatures (set in Open() so binding is always valid).
	ID3D12RootSignature* m_graphicsRootSig = nullptr;
	ID3D12RootSignature* m_computeRootSig  = nullptr;

	ID3D12GraphicsCommandList7*          m_commandList      = nullptr;
	std::vector<ID3D12CommandAllocator*> m_allocators;
	std::vector<D3D12_RESOURCE_BARRIER>  m_pendingBarriers;
	uint32_t                             m_currentFrameIndex = 0;
	uint32_t                             m_framesInFlight    = 0;
	ID3D12Device*                        m_device            = nullptr;
};

} // namespace EngineCore::RHI::DX12
