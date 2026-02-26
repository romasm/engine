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
#define DEVICE  Render::Device()
#define DEVICE3 Render::Device3()
#define CONTEXT  Render::Context()
#define CONTEXT3 Render::Context3()

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
		POINTLIST  = (int)RHI::Topology::PointList,
		LINELIST   = (int)RHI::Topology::LineList,
		TRISLIST   = (int)RHI::Topology::TriangleList,
		PATCH3LIST = (int)RHI::Topology::Patch3,
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

		void m_resize();

		// Deferred resize state (DX12 only)
		bool     m_pendingResize  = false;
		uint32_t m_pendingResizeW = 0;
		uint32_t m_pendingResizeH = 0;

	public:
		// Raw DX11 device/context accessors (route through RHI backend; nullptr when DX12)
		inline static ID3D11Device*         Device()   { return m_instance->gfxDevice->GetDX11Device(); }
		inline static ID3D11Device3*        Device3()  { return m_instance->gfxDevice->GetDX11Device3(); }
		inline static ID3D11DeviceContext*  Context()  { return m_instance->gfxDevice->GetDX11Context(); }
		inline static ID3D11DeviceContext3* Context3() { return m_instance->gfxDevice->GetDX11Context3(); }

		// DX12 frame lifecycle: call at frame start (after PERF_GPU_FRAME_BEGIN).
		// On DX11 this is a no-op.
		static void BeginFrameDX12();

		// Deferred DX12 swap chain resize (set from WM_SIZE, applied next frame).
		static void SetPendingResize(uint32_t width, uint32_t height)
		{
			m_instance->m_pendingResizeW = width;
			m_instance->m_pendingResizeH = height;
			m_instance->m_pendingResize  = true;
		}

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