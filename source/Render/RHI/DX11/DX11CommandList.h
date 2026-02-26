#pragma once
#include "GfxCommandList.h"
#include "DX11Types.h"

// DX11 command list — wraps the immediate context.
// All methods delegate directly to ID3D11DeviceContext.
// Open()/Close() are no-ops (DX11 has no concept of deferred recording).

namespace EngineCore::RHI::DX11
{

class DX11CommandList : public IGfxCommandList
{
public:
	explicit DX11CommandList(ID3D11DeviceContext* context, ID3D11DeviceContext3* context3)
		: m_context(context), m_context3(context3) {}

	~DX11CommandList() override {}

	// -----------------------------------------------------------------------
	// Lifecycle

	void Open()  override {}  // no-op for DX11
	void Close() override {}  // no-op for DX11

	// -----------------------------------------------------------------------
	// Output merger

	void SetRenderTargets(uint32_t count, GfxRTV* const* rtvs, GfxDSV* dsv) override
	{
		ID3D11RenderTargetView* d3dRTVs[8] = {};
		for(uint32_t i = 0; i < count && i < 8; ++i)
			d3dRTVs[i] = rtvs[i] ? Cast(rtvs[i])->view : nullptr;
		ID3D11DepthStencilView* d3dDSV = dsv ? Cast(dsv)->view : nullptr;
		m_context->OMSetRenderTargets(count, d3dRTVs, d3dDSV);
	}

	void ClearRenderTarget(GfxRTV* rtv, float r, float g, float b, float a) override
	{
		const float color[4] = { r, g, b, a };
		m_context->ClearRenderTargetView(Cast(rtv)->view, color);
	}

	void ClearDepthStencil(GfxDSV* dsv, float depth, uint8_t stencil) override
	{
		m_context->ClearDepthStencilView(Cast(dsv)->view,
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
	}

	void ClearUAVFloat(GfxUAV* uav, float r, float g, float b, float a) override
	{
		const float color[4] = { r, g, b, a };
		m_context->ClearUnorderedAccessViewFloat(Cast(uav)->view, color);
	}

	void ClearUAVUint(GfxUAV* uav, uint32_t r, uint32_t g, uint32_t b, uint32_t a) override
	{
		const uint32_t color[4] = { r, g, b, a };
		m_context->ClearUnorderedAccessViewUint(Cast(uav)->view, color);
	}

	// -----------------------------------------------------------------------
	// Rasterizer

	void SetViewport(const GfxViewport& vp) override
	{
		D3D11_VIEWPORT d3dVP;
		d3dVP.TopLeftX = vp.topLeftX;
		d3dVP.TopLeftY = vp.topLeftY;
		d3dVP.Width    = vp.width;
		d3dVP.Height   = vp.height;
		d3dVP.MinDepth = vp.minDepth;
		d3dVP.MaxDepth = vp.maxDepth;
		m_context->RSSetViewports(1, &d3dVP);
	}

	void SetTopology(Topology topo) override
	{
		m_context->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topo));
	}

	// -----------------------------------------------------------------------
	// Pipeline state

	void SetPipelineState(GfxPipelineState* pso) override;  // implemented in .cpp

	void SetDepthStencilState(void* state, uint32_t stencilRef) override
	{
		m_context->OMSetDepthStencilState(static_cast<ID3D11DepthStencilState*>(state), stencilRef);
	}

	void SetBlendState(void* state, const float* blendFactor, uint32_t sampleMask) override
	{
		m_context->OMSetBlendState(static_cast<ID3D11BlendState*>(state), blendFactor, sampleMask);
	}

	void SetRasterizerState(void* state) override
	{
		m_context->RSSetState(static_cast<ID3D11RasterizerState*>(state));
	}

	// -----------------------------------------------------------------------
	// Input assembly

	void SetVertexBuffer(uint32_t slot, GfxBuffer* buffer, uint32_t stride, uint32_t offset = 0) override
	{
		ID3D11Buffer* buf = Cast(buffer)->buffer;
		m_context->IASetVertexBuffers(slot, 1, &buf, &stride, &offset);
	}

	void SetIndexBuffer(GfxBuffer* buffer, bool index32bit, uint32_t offset = 0) override
	{
		m_context->IASetIndexBuffer(Cast(buffer)->buffer,
			index32bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT,
			offset);
	}

	void SetInputLayout(GfxInputLayout* layout) override
	{
		m_context->IASetInputLayout(layout ? Cast(layout)->layout : nullptr);
	}

	// -----------------------------------------------------------------------
	// Constant buffers (single slot)

	void SetVSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{ auto* b = buffer ? Cast(buffer)->buffer : nullptr; m_context->VSSetConstantBuffers(slot, 1, &b); }
	void SetHSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{ auto* b = buffer ? Cast(buffer)->buffer : nullptr; m_context->HSSetConstantBuffers(slot, 1, &b); }
	void SetDSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{ auto* b = buffer ? Cast(buffer)->buffer : nullptr; m_context->DSSetConstantBuffers(slot, 1, &b); }
	void SetGSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{ auto* b = buffer ? Cast(buffer)->buffer : nullptr; m_context->GSSetConstantBuffers(slot, 1, &b); }
	void SetPSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{ auto* b = buffer ? Cast(buffer)->buffer : nullptr; m_context->PSSetConstantBuffers(slot, 1, &b); }
	void SetCSConstantBuffer(uint32_t slot, GfxBuffer* buffer) override
	{ auto* b = buffer ? Cast(buffer)->buffer : nullptr; m_context->CSSetConstantBuffers(slot, 1, &b); }

	// Constant buffers (batch)

	void SetVSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) override
	{ auto* arr = ExtractBuffers(buffers, count); m_context->VSSetConstantBuffers(startSlot, count, arr); }
	void SetHSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) override
	{ auto* arr = ExtractBuffers(buffers, count); m_context->HSSetConstantBuffers(startSlot, count, arr); }
	void SetDSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) override
	{ auto* arr = ExtractBuffers(buffers, count); m_context->DSSetConstantBuffers(startSlot, count, arr); }
	void SetGSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) override
	{ auto* arr = ExtractBuffers(buffers, count); m_context->GSSetConstantBuffers(startSlot, count, arr); }
	void SetPSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) override
	{ auto* arr = ExtractBuffers(buffers, count); m_context->PSSetConstantBuffers(startSlot, count, arr); }
	void SetCSConstantBuffers(uint32_t startSlot, uint32_t count, GfxBuffer* const* buffers) override
	{ auto* arr = ExtractBuffers(buffers, count); m_context->CSSetConstantBuffers(startSlot, count, arr); }

	// -----------------------------------------------------------------------
	// Shader resources (single slot)

	void SetVSResource(uint32_t slot, GfxSRV* srv) override
	{ auto* v = srv ? Cast(srv)->view : nullptr; m_context->VSSetShaderResources(slot, 1, &v); }
	void SetHSResource(uint32_t slot, GfxSRV* srv) override
	{ auto* v = srv ? Cast(srv)->view : nullptr; m_context->HSSetShaderResources(slot, 1, &v); }
	void SetDSResource(uint32_t slot, GfxSRV* srv) override
	{ auto* v = srv ? Cast(srv)->view : nullptr; m_context->DSSetShaderResources(slot, 1, &v); }
	void SetGSResource(uint32_t slot, GfxSRV* srv) override
	{ auto* v = srv ? Cast(srv)->view : nullptr; m_context->GSSetShaderResources(slot, 1, &v); }
	void SetPSResource(uint32_t slot, GfxSRV* srv) override
	{ auto* v = srv ? Cast(srv)->view : nullptr; m_context->PSSetShaderResources(slot, 1, &v); }
	void SetCSResource(uint32_t slot, GfxSRV* srv) override
	{ auto* v = srv ? Cast(srv)->view : nullptr; m_context->CSSetShaderResources(slot, 1, &v); }

	// Shader resources (batch)

	void SetVSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) override
	{ auto* arr = ExtractSRVs(srvs, count); m_context->VSSetShaderResources(startSlot, count, arr); }
	void SetHSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) override
	{ auto* arr = ExtractSRVs(srvs, count); m_context->HSSetShaderResources(startSlot, count, arr); }
	void SetDSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) override
	{ auto* arr = ExtractSRVs(srvs, count); m_context->DSSetShaderResources(startSlot, count, arr); }
	void SetGSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) override
	{ auto* arr = ExtractSRVs(srvs, count); m_context->GSSetShaderResources(startSlot, count, arr); }
	void SetPSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) override
	{ auto* arr = ExtractSRVs(srvs, count); m_context->PSSetShaderResources(startSlot, count, arr); }
	void SetCSResources(uint32_t startSlot, uint32_t count, GfxSRV* const* srvs) override
	{ auto* arr = ExtractSRVs(srvs, count); m_context->CSSetShaderResources(startSlot, count, arr); }

	// -----------------------------------------------------------------------
	// UAVs

	void SetCSUnorderedAccessViews(uint32_t startSlot, uint32_t count, GfxUAV* const* uavs) override
	{
		static const uint32_t initialCounts[D3D11_1_UAV_SLOT_COUNT] = {};
		auto* arr = ExtractUAVs(uavs, count);
		m_context->CSSetUnorderedAccessViews(startSlot, count, arr, initialCounts);
	}

	void SetOMUnorderedAccessViews(uint32_t startSlot, uint32_t count, GfxUAV* const* uavs,
		const uint32_t* initialCounts) override
	{
		auto* arr = ExtractUAVs(uavs, count);
		m_context->OMSetRenderTargetsAndUnorderedAccessViews(
			D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
			startSlot, count, arr, initialCounts);
	}

	// -----------------------------------------------------------------------
	// Samplers (single slot)

	void SetVSSampler(uint32_t slot, GfxSampler* sampler) override
	{ auto* s = sampler ? Cast(sampler)->sampler : nullptr; m_context->VSSetSamplers(slot, 1, &s); }
	void SetHSSampler(uint32_t slot, GfxSampler* sampler) override
	{ auto* s = sampler ? Cast(sampler)->sampler : nullptr; m_context->HSSetSamplers(slot, 1, &s); }
	void SetDSSampler(uint32_t slot, GfxSampler* sampler) override
	{ auto* s = sampler ? Cast(sampler)->sampler : nullptr; m_context->DSSetSamplers(slot, 1, &s); }
	void SetGSSampler(uint32_t slot, GfxSampler* sampler) override
	{ auto* s = sampler ? Cast(sampler)->sampler : nullptr; m_context->GSSetSamplers(slot, 1, &s); }
	void SetPSSampler(uint32_t slot, GfxSampler* sampler) override
	{ auto* s = sampler ? Cast(sampler)->sampler : nullptr; m_context->PSSetSamplers(slot, 1, &s); }
	void SetCSSampler(uint32_t slot, GfxSampler* sampler) override
	{ auto* s = sampler ? Cast(sampler)->sampler : nullptr; m_context->CSSetSamplers(slot, 1, &s); }

	// Samplers (batch)

	void SetVSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) override
	{ auto* arr = ExtractSamplers(samplers, count); m_context->VSSetSamplers(startSlot, count, arr); }
	void SetHSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) override
	{ auto* arr = ExtractSamplers(samplers, count); m_context->HSSetSamplers(startSlot, count, arr); }
	void SetDSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) override
	{ auto* arr = ExtractSamplers(samplers, count); m_context->DSSetSamplers(startSlot, count, arr); }
	void SetGSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) override
	{ auto* arr = ExtractSamplers(samplers, count); m_context->GSSetSamplers(startSlot, count, arr); }
	void SetPSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) override
	{ auto* arr = ExtractSamplers(samplers, count); m_context->PSSetSamplers(startSlot, count, arr); }
	void SetCSSamplers(uint32_t startSlot, uint32_t count, GfxSampler* const* samplers) override
	{ auto* arr = ExtractSamplers(samplers, count); m_context->CSSetSamplers(startSlot, count, arr); }

	// -----------------------------------------------------------------------
	// Draw / dispatch

	void Draw(uint32_t vertexCount, uint32_t startVertex = 0) override
	{ m_context->Draw(vertexCount, startVertex); }

	void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) override
	{ m_context->DrawIndexed(indexCount, startIndex, baseVertex); }

	void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
		uint32_t startVertex = 0, uint32_t startInstance = 0) override
	{ m_context->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance); }

	void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
		uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0) override
	{ m_context->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance); }

	void Dispatch(uint32_t x, uint32_t y, uint32_t z) override
	{ m_context->Dispatch(x, y, z); }

	// -----------------------------------------------------------------------
	// Dynamic updates

	void UpdateBuffer(GfxBuffer* buffer, const void* data, size_t sizeBytes) override
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		if(SUCCEEDED(m_context->Map(Cast(buffer)->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			memcpy(mapped.pData, data, sizeBytes);
			m_context->Unmap(Cast(buffer)->buffer, 0);
		}
	}

	void UpdateSubresource(GfxTexture* texture, uint32_t subresource,
		const GfxBox* destBox, const void* data, uint32_t rowPitch, uint32_t depthPitch) override
	{
		const D3D11_BOX* d3dBox = destBox ? reinterpret_cast<const D3D11_BOX*>(destBox) : nullptr;
		m_context->UpdateSubresource(Cast(texture)->AsResource(), subresource, d3dBox,
			data, rowPitch, depthPitch);
	}

	void UpdateSubresource(GfxBuffer* buffer, uint32_t subresource,
		const GfxBox* destBox, const void* data, uint32_t rowPitch, uint32_t depthPitch) override
	{
		const D3D11_BOX* d3dBox = destBox ? reinterpret_cast<const D3D11_BOX*>(destBox) : nullptr;
		m_context->UpdateSubresource(Cast(buffer)->buffer, subresource, d3dBox,
			data, rowPitch, depthPitch);
	}

	// -----------------------------------------------------------------------
	// Queries

	void BeginQuery(GfxQueryHeap* heap, uint32_t index) override
	{
		m_context->Begin(Cast(heap)->queries[index]);
	}

	void EndQuery(GfxQueryHeap* heap, uint32_t index) override
	{
		m_context->End(Cast(heap)->queries[index]);
	}

	HRESULT GetQueryData(GfxQueryHeap* heap, uint32_t index,
		void* data, uint32_t dataSize, uint32_t flags = 0) override
	{
		return m_context->GetData(Cast(heap)->queries[index], data, dataSize, flags);
	}

	// -----------------------------------------------------------------------
	// Barriers (no-op on DX11)

	void TransitionBuffer(GfxBuffer*, ResourceState, ResourceState) override {}
	void TransitionTexture(GfxTexture*, ResourceState, ResourceState) override {}
	void FlushBarriers() override {}

	// -----------------------------------------------------------------------
	// Copy / resolve

	void CopyResource(GfxTexture* dest, GfxTexture* source) override
	{
		m_context->CopyResource(Cast(dest)->AsResource(), Cast(source)->AsResource());
	}

	void CopyTextureRegion(GfxTexture* dest, uint32_t destSubresource,
		uint32_t destX, uint32_t destY, uint32_t destZ,
		GfxTexture* source, uint32_t sourceSubresource,
		const GfxBox* sourceBox) override
	{
		const D3D11_BOX* pBox = nullptr;
		D3D11_BOX d3dBox;
		if(sourceBox)
		{
			d3dBox.left   = sourceBox->left;
			d3dBox.top    = sourceBox->top;
			d3dBox.front  = sourceBox->front;
			d3dBox.right  = sourceBox->right;
			d3dBox.bottom = sourceBox->bottom;
			d3dBox.back   = sourceBox->back;
			pBox = &d3dBox;
		}
		m_context->CopySubresourceRegion(
			Cast(dest)->AsResource(), destSubresource, destX, destY, destZ,
			Cast(source)->AsResource(), sourceSubresource, pBox);
	}

	void GenerateMips(GfxSRV* srv) override
	{
		m_context->GenerateMips(Cast(srv)->view);
	}

	void ResolveSubresource(GfxTexture* dest, uint32_t destSubresource,
		GfxTexture* source, uint32_t sourceSubresource, DXGI_FORMAT format) override
	{
		m_context->ResolveSubresource(Cast(dest)->AsResource(), destSubresource,
			Cast(source)->AsResource(), sourceSubresource, format);
	}

private:
	ID3D11DeviceContext*  m_context  = nullptr;
	ID3D11DeviceContext3* m_context3 = nullptr;

	// Temporary arrays for batch resource extraction (thread-local scratch)

	ID3D11Buffer* const* ExtractBuffers(GfxBuffer* const* buffers, uint32_t count)
	{
		static thread_local ID3D11Buffer* scratch[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
		for(uint32_t i = 0; i < count; ++i)
			scratch[i] = buffers[i] ? Cast(buffers[i])->buffer : nullptr;
		return scratch;
	}

	ID3D11ShaderResourceView* const* ExtractSRVs(GfxSRV* const* srvs, uint32_t count)
	{
		static thread_local ID3D11ShaderResourceView* buffer[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
		for(uint32_t i = 0; i < count; ++i)
			buffer[i] = srvs[i] ? Cast(srvs[i])->view : nullptr;
		return buffer;
	}

	ID3D11UnorderedAccessView* const* ExtractUAVs(GfxUAV* const* uavs, uint32_t count)
	{
		static thread_local ID3D11UnorderedAccessView* buffer[D3D11_1_UAV_SLOT_COUNT];
		for(uint32_t i = 0; i < count; ++i)
			buffer[i] = uavs[i] ? Cast(uavs[i])->view : nullptr;
		return buffer;
	}

	ID3D11SamplerState* const* ExtractSamplers(GfxSampler* const* samplers, uint32_t count)
	{
		static thread_local ID3D11SamplerState* scratch[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
		for(uint32_t i = 0; i < count; ++i)
			scratch[i] = samplers[i] ? Cast(samplers[i])->sampler : nullptr;
		return scratch;
	}
};

} // namespace EngineCore::RHI::DX11
