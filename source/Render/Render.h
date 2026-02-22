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
#include "GfxDevice.h"
#include "GfxCommandList.h"
#include "GfxSync.h"

#define RENDER Render::Get()
#define DEVICE RENDER->m_pd3dDevice
#define DEVICE3 RENDER->m_pd3dDevice3
#define CONTEXT RENDER->m_pImmediateContext
#define CONTEXT3 RENDER->m_pImmediateContext3

// RHI backend accessors (use these for new rendering code)
#define GFX_DEVICE   Render::GetGfxDevice()
#define GFX_CMD      Render::GetCommandList()
#define GFX_FRAME    Render::GetFrameScheduler()

namespace EngineCore
{
//------------------------------------------------------------------

	//static const float blendFactor[4] = {0.0f};

	enum IA_TOPOLOGY
	{
		POINTLIST = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
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
		friend class MeshMgr;
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
		ID3D11Device3 *m_pd3dDevice3;
		ID3D11DeviceContext *m_pImmediateContext;
		ID3D11DeviceContext3 *m_pImmediateContext3;
		
		IDXGIDevice* m_pDXGIDevice;
		IDXGIAdapter1* m_pDxgiAdapter;
		IDXGIFactory2* m_pDxgiFactory;

		Window* CurrentHudWindow;

		Buffer* bufferMgr;

		// RHI backend (DX11 for now; DX12 in Phase 2)
		RHI::IGfxDevice*        gfxDevice;
		RHI::IGfxCommandList*   gfxCommandList;
		RHI::IGfxFrameScheduler* gfxFrameScheduler;

		inline static RHI::IGfxDevice*         GetGfxDevice()        { return m_instance->gfxDevice; }
		inline static RHI::IGfxCommandList*     GetCommandList()      { return m_instance->gfxCommandList; }
		inline static RHI::IGfxFrameScheduler*  GetFrameScheduler()   { return m_instance->gfxFrameScheduler; }

		static void RegLuaClass();

	protected:
		static Render *m_instance;

		bool m_createdevice();

		void m_resize();
		
	public:
		// Raw DX11 device/context accessors (used by DX11-only systems: ShaderCodeMgr, RenderState, Profiler)
		inline static ID3D11Device* Device() {return m_instance->m_pd3dDevice;}
		inline static ID3D11DeviceContext* Context() {return m_instance->m_pImmediateContext;}

		// DX12 frame lifecycle: call at frame start (after PERF_GPU_FRAME_BEGIN).
		// On DX11 this is a no-op.
		static void BeginFrameDX12();

		// Topology (routes through RHI; IA_TOPOLOGY values match RHI::Topology)
		inline static void SetTopology(IA_TOPOLOGY topo)
		{GFX_CMD->SetTopology(static_cast<RHI::Topology>(topo));}

		// Texture binding by ID (convenience wrappers, route through GFX_CMD)
		inline static void PSSetShaderTexture(uint32_t slot, uint32_t texId)
		{auto tex = TexMgr::GetResourcePtr(texId);
			GFX_CMD->SetPSResource(slot, tex);}

	};

//------------------------------------------------------------------
}