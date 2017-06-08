#pragma once
#include "stdafx.h"
#include "RenderState.h"
#include "DataTypes.h"
#include "EngineSettings.h"
#include "WindowsMgr.h"
#include "Buffer.h"
#include "ShaderCodeMgr.h"
#include "TexMgr.h"
#include "ResourceProcessor.h"

#define RENDER Render::Get()
#define DEVICE RENDER->m_pd3dDevice
#define CONTEXT RENDER->m_pImmediateContext

namespace EngineCore
{
//------------------------------------------------------------------

	//static const float blendFactor[4] = {0.0f};

	enum IA_TOPOLOGY 
	{
		LINELIST = D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
		TRISLIST = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		PATCH3LIST = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
	};

	class Render
	{
		friend class Image2D;
		friend class Font;
		friend class Text;
		friend class RenderTarget;
		friend class ScreenPlane;
		friend class ShaderMgr;
		friend class MaterialMgr;
		friend class StMeshMgr;
		friend class FontMgr;
		friend class ScenePipeline;
		friend class WorldMgr;
	public:
		Render();
		virtual ~Render();
		
		inline static Render* Get(){return m_instance;}
		
		bool Init();
		void Close();

		void* operator new(size_t i)
		{
			return _aligned_malloc(i,16);
		}

		void operator delete(void* p)
		{
			_aligned_free(p);
		}
		
		RenderStateMgr* renderStateMgr;
		SamplerStateMgr* samplerStateMgr;

		ResourceProcessor* resourceProc;
		
		GlobalColor gl_color;

		ID3D11Device *m_pd3dDevice;
		ID3D11DeviceContext *m_pImmediateContext;

		IDXGIDevice* m_pDXGIDevice;
		IDXGIAdapter1* m_pDxgiAdapter;
		IDXGIFactory2* m_pDxgiFactory;

		Window* CurrentHudWindow;

		Buffer* bufferMgr;

		static void RegLuaClass();

	protected:
		static Render *m_instance;

		bool m_createdevice();

		void m_resize();
		
	public:
		// isolate gapi calls
		inline static ID3D11Device* Device() {return m_instance->m_pd3dDevice;}
		inline static ID3D11DeviceContext* Context() {return m_instance->m_pImmediateContext;}

		// subresource
		inline static void UpdateDynamicResource(ID3D11Resource* resource, void* data, size_t size)
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			if(FAILED(Render::Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
				return;
			memcpy(mappedResource.pData, data, size);
			Render::Unmap(resource, 0);
		}

		inline static void UpdateSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, 
			const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
		{m_instance->m_pImmediateContext->UpdateSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);}

		inline static void ResolveSubresource(ID3D11Resource *pDstResource, UINT DstSubresource, ID3D11Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
		{m_instance->m_pImmediateContext->ResolveSubresource(pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);}

		inline static HRESULT Map(ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource)
		{return m_instance->m_pImmediateContext->Map(pResource, Subresource, MapType, MapFlags, pMappedResource);}
		inline static void Unmap(ID3D11Resource *pResource, UINT Subresource)
		{m_instance->m_pImmediateContext->Unmap(pResource, Subresource);}

		// RTV
		inline static void ClearUnorderedAccessViewFloat(ID3D11UnorderedAccessView *pUnorderedAccessView, XMFLOAT4 ColorRGBA)
		{const float color[4] = {ColorRGBA.x, ColorRGBA.y, ColorRGBA.z, ColorRGBA.w};
		m_instance->m_pImmediateContext->ClearUnorderedAccessViewFloat(pUnorderedAccessView, color);}

		inline static void ClearUnorderedAccessViewUint(ID3D11UnorderedAccessView *pUnorderedAccessView, XMFLOAT4 ColorRGBA)
		{const UINT color[4] = {UINT(ColorRGBA.x), UINT(ColorRGBA.y), UINT(ColorRGBA.z), UINT(ColorRGBA.w)};
		m_instance->m_pImmediateContext->ClearUnorderedAccessViewUint(pUnorderedAccessView, color);}

		inline static void ClearRenderTargetView(ID3D11RenderTargetView *pRenderTargetView, XMFLOAT4 ColorRGBA)
		{const float color[4] = {ColorRGBA.x, ColorRGBA.y, ColorRGBA.z, ColorRGBA.w};
		m_instance->m_pImmediateContext->ClearRenderTargetView(pRenderTargetView, color);}

		inline static void ClearDepthStencilView(ID3D11DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
		{m_instance->m_pImmediateContext->ClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil);}

		inline static void GenerateMips(ID3D11ShaderResourceView *pShaderResourceView)
		{m_instance->m_pImmediateContext->GenerateMips(pShaderResourceView);}

		inline static void CopyResource(ID3D11Resource *pDstResource, ID3D11Resource *pSrcResource)
		{m_instance->m_pImmediateContext->CopyResource(pDstResource, pSrcResource);}

		inline static HRESULT CreateRenderTargetView(ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView **ppRTView)
		{return m_instance->m_pd3dDevice->CreateRenderTargetView(pResource, pDesc, ppRTView);}

		inline static HRESULT CreateShaderResourceView(ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView)
		{return m_instance->m_pd3dDevice->CreateShaderResourceView(pResource, pDesc, ppSRView);}

		inline static HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture2D)
		{return m_instance->m_pd3dDevice->CreateTexture2D(pDesc, pInitialData, ppTexture2D);}
		
		inline static HRESULT CreateTexture3D(const D3D11_TEXTURE3D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture3D **ppTexture3D)
		{return m_instance->m_pd3dDevice->CreateTexture3D(pDesc, pInitialData, ppTexture3D);}

		inline static HRESULT CreateUnorderedAccessView(ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView)
		{return m_instance->m_pd3dDevice->CreateUnorderedAccessView(pResource, pDesc, ppUAView);}

		inline static HRESULT CreateDepthStencilView(ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView)
		{return m_instance->m_pd3dDevice->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView);}

		// OM
		inline static void OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView)
		{m_instance->m_pImmediateContext->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);}

		static ID3D11RenderTargetView** rts_null;
		inline static void OMUnsetRenderTargets()
		{m_instance->m_pImmediateContext->OMSetRenderTargets(8, rts_null, nullptr);}
	
		inline static void OMSetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, 
			ID3D11RenderTargetView *const *ppRenderTargetViews,
			ID3D11DepthStencilView *pDepthStencilView,
			UINT UAVStartSlot, UINT NumUAVs,
			ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
			const UINT *pUAVInitialCounts)
		{m_instance->m_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(NumRTVs, 
			ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);}

		inline static void OMSetDepthState(uint16_t id, uint32_t stencilRef = 1)
		{m_instance->m_pImmediateContext->OMSetDepthStencilState(RenderStateMgr::GetDepthStatePtr(id), stencilRef);}
		inline static void OMSetBlendState(uint16_t id)
		{m_instance->m_pImmediateContext->OMSetBlendState(RenderStateMgr::GetBlendStatePtr(id), NULL, 0xffffffff);}

		// IA
		inline static void IASetInputLayout(ID3D11InputLayout *pInputLayout)
		{m_instance->m_pImmediateContext->IASetInputLayout(pInputLayout);}
		
		inline static void SetTopology(IA_TOPOLOGY topo)
		{m_instance->m_pImmediateContext->IASetPrimitiveTopology((D3D_PRIMITIVE_TOPOLOGY)topo);}

		// RS
		inline static void RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT *pViewports)
		{m_instance->m_pImmediateContext->RSSetViewports(NumViewports, pViewports);}
		inline static void RSSetState(uint16_t id)
		{m_instance->m_pImmediateContext->RSSetState(RenderStateMgr::GetRSStatePtr(id));}

		// PS
		inline static void PSSetShader(ID3D11PixelShader* pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{m_instance->m_pImmediateContext->PSSetShader(pPixelShader, ppClassInstances, NumClassInstances);}
		inline static void PSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{m_instance->m_pImmediateContext->PSSetSamplers(StartSlot, NumSamplers, ppSamplers);}
		inline static void PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
		{m_instance->m_pImmediateContext->PSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);}
		inline static void PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
		{m_instance->m_pImmediateContext->PSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);}
		inline static void PSSetShaderTexture(UINT slot, uint32_t texId)
		{auto tex = TexMgr::GetTexturePtr(texId);
			m_instance->m_pImmediateContext->PSSetShaderResources(slot, 1, &tex);}

		// VS
		inline static void VSSetShader(ID3D11VertexShader* pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{m_instance->m_pImmediateContext->VSSetShader(pVertexShader, ppClassInstances, NumClassInstances);}
		inline static void VSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{m_instance->m_pImmediateContext->VSSetSamplers(StartSlot, NumSamplers, ppSamplers);}
		inline static void VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
		{m_instance->m_pImmediateContext->VSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);}
		inline static void VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
		{m_instance->m_pImmediateContext->VSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);}
		inline static void VSSetShaderTexture(UINT slot, uint32_t texId)
		{auto tex = TexMgr::GetTexturePtr(texId);
			m_instance->m_pImmediateContext->VSSetShaderResources(slot, 1, &tex);}

		// HS
		inline static void HSSetShader(ID3D11HullShader* pHullShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{m_instance->m_pImmediateContext->HSSetShader(pHullShader, ppClassInstances, NumClassInstances);}
		inline static void HSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{m_instance->m_pImmediateContext->HSSetSamplers(StartSlot, NumSamplers, ppSamplers);}
		inline static void HSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
		{m_instance->m_pImmediateContext->HSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);}
		inline static void HSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
		{m_instance->m_pImmediateContext->HSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);}
		inline static void HSSetShaderTexture(UINT slot, uint32_t texId)
		{auto tex = TexMgr::GetTexturePtr(texId);
			m_instance->m_pImmediateContext->HSSetShaderResources(slot, 1, &tex);}

		// DS
		inline static void DSSetShader(ID3D11DomainShader* pDomainShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{m_instance->m_pImmediateContext->DSSetShader(pDomainShader, ppClassInstances, NumClassInstances);}
		inline static void DSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{m_instance->m_pImmediateContext->DSSetSamplers(StartSlot, NumSamplers, ppSamplers);}
		inline static void DSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
		{m_instance->m_pImmediateContext->DSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);}
		inline static void DSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
		{m_instance->m_pImmediateContext->DSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);}
		inline static void DSSetShaderTexture(UINT slot, uint32_t texId)
		{auto tex = TexMgr::GetTexturePtr(texId);
			m_instance->m_pImmediateContext->DSSetShaderResources(slot, 1, &tex);}

		// GS
		inline static void GSSetShader(ID3D11GeometryShader* pGeometryShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{m_instance->m_pImmediateContext->GSSetShader(pGeometryShader, ppClassInstances, NumClassInstances);}
		inline static void GSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{m_instance->m_pImmediateContext->GSSetSamplers(StartSlot, NumSamplers, ppSamplers);}
		inline static void GSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
		{m_instance->m_pImmediateContext->GSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);}
		inline static void GSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
		{m_instance->m_pImmediateContext->GSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);}
		inline static void GSSetShaderTexture(UINT slot, uint32_t texId)
		{auto tex = TexMgr::GetTexturePtr(texId);
			m_instance->m_pImmediateContext->GSSetShaderResources(slot, 1, &tex);}

		//CS
		inline static void CSSetShader(ID3D11ComputeShader* pComputeShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{m_instance->m_pImmediateContext->CSSetShader(pComputeShader, ppClassInstances, NumClassInstances);}
		inline static void CSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{m_instance->m_pImmediateContext->CSSetSamplers(StartSlot, NumSamplers, ppSamplers);}
		inline static void CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
		{m_instance->m_pImmediateContext->CSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);}
		inline static void CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
		{m_instance->m_pImmediateContext->CSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);}
		

	};

//------------------------------------------------------------------
}