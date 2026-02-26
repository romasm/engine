#include "stdafx.h"
#include "Render.h"
#include "DX11Device.h"
#include "DX12Device.h"
#include "macros.h"
#include "Image.h"
#include "Font.h"
#include "Text.h"
#include "Log.h"
#include "ShaderMgr.h"
#include "MaterialMgr.h"
#include "MeshMgr.h"
#include "FontMgr.h"
#include "ScenePipeline.h"
#include "WorldMgr.h"
#include "Profiler.h"
#include "TexLoader.h"

#include "ResourceProcessor.h"

namespace EngineCore
{
//------------------------------------------------------------------
	
	extern "C"
	{
		__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
		__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	}

	void Render::RegLuaClass()
	{
		GlobalColor::RegLuaFunctions();
		ResourceProcessor::RegLuaFunctions();
	}
	
	Render* Render::m_instance = nullptr;

	Render::Render()
	{
		if(!m_instance)
		{
			m_instance = this;
			renderStateMgr = nullptr;
			samplerStateMgr = nullptr;
			bufferMgr = nullptr;
			gfxDevice = nullptr;
			gfxCommandList = nullptr;
			gfxFrameScheduler = nullptr;
			CurrentHudWindow = nullptr;

			RegLuaClass();
		}
		else
		{
			ERR("Can't init Render");
		}
	}

	Render::~Render()
	{
		m_instance = nullptr;
	}
	
	bool Render::Init()
	{
		// RHI backend — DX12 if config requests it, otherwise DX11.
		// The DX12 device is created without a swap chain here; Window::CreateSwapChain()
		// calls GFX_DEVICE->InitSwapChain() once the HWND is available.
		if(CONFIG(bool, dx12_backend))
		{
			auto* dx12Device = new RHI::DX12::DX12Device();
			if(!dx12Device->Init(nullptr))
			{
				ERR("Failed to initialise DX12 device!");
				delete dx12Device;
				return false;
			}
			gfxDevice         = dx12Device;
			gfxFrameScheduler = dx12Device->GetFrameScheduler();
			gfxCommandList    = gfxFrameScheduler->AllocateCommandList();
		}
		else
		{
			auto* dx11Device = new RHI::DX11::DX11Device();
			if(!dx11Device->Init(nullptr))
			{
				ERR("Cant create DirectX Device!");
				delete dx11Device;
				return false;
			}
			gfxDevice = dx11Device;

			gfxFrameScheduler = dx11Device->CreateFrameScheduler();
			gfxCommandList    = gfxFrameScheduler->AllocateCommandList();

			// Log DX11 device info
			LOG("------ GPU Device Info ------");
			LOG("  API:             DirectX 11.3");
			LOG("  GPU:             %s", gfxDevice->GetAdapterName());
			LOG("  Raytracing:      not supported (DX11)");
			LOG("  Mesh shaders:    not supported (DX11)");
			LOG("-----------------------------");
		}

		// RENDER STATE
		renderStateMgr = new RenderStateMgr;
		if ( !renderStateMgr->SetDefault())
		{
			ERR("Cant initilize default render states!");
			return false;
		}

		// SAMPLER STATE
		samplerStateMgr = new SamplerStateMgr;
		if ( !samplerStateMgr->LoadSamplers())
		{
			ERR("Cant load predefined sampler states!");
			return false;
		}

	#ifdef _DEV
		if( !Profiler::Get()->InitQueries() )
		{
			ERR("Cant init profiler queries!");
			return false;
		}
	#endif

		bufferMgr = new Buffer;

		return true;
	}

	// m_createdevice() removed — device creation moved into DX11Device::Init() and DX12Device::Init()

	void Render::BeginFrameDX12()
	{
		if(!m_instance || !m_instance->gfxDevice || !m_instance->gfxDevice->IsDX12())
			return;

		static uint32_t s_frameCounter = 0;
		++s_frameCounter;

		auto* dx12Device = static_cast<RHI::DX12::DX12Device*>(m_instance->gfxDevice);

		// Check device health BEFORE waiting for the GPU fence.
		// This catches device removal from the PREVIOUS frame's GPU work.
		if(!dx12Device->CheckDeviceHealth())
		{
			static bool s_reported = false;
			if(!s_reported)
			{
				ERR("DX12: Device removed at start of frame %u!", s_frameCounter);
				s_reported = true;
			}
			return;  // Don't try to continue with a dead device
		}

		// Apply deferred swap chain resize before starting the new frame.
		// This is safe here because the previous frame's GPU work has been
		// presented and no command list is open yet.
		if(m_instance->m_pendingResize)
		{
			m_instance->m_pendingResize = false;
			dx12Device->ResizeSwapChain(
				m_instance->m_pendingResizeW,
				m_instance->m_pendingResizeH);
		}

		// Begin new frame: wait for prior GPU work, reset allocators
		m_instance->gfxFrameScheduler->BeginFrame();

		// Open the command list for recording
		m_instance->gfxCommandList->Open();

		// Set the shader-visible descriptor heaps (required before any binding)
		auto* dx12CmdList = static_cast<RHI::DX12::DX12CommandList*>(m_instance->gfxCommandList);
		ID3D12DescriptorHeap* heaps[] = {
			dx12Device->GetCbvSrvUavHeap(),
			dx12Device->GetSamplerHeap()
		};
		dx12CmdList->GetCommandList()->SetDescriptorHeaps(2, heaps);
	}

	void Render::Close()
	{
	#ifdef _DEV
		Profiler::Get()->ReleaseQueries();
	#endif

		// Destroy state managers and buffers before RHI backend
		// (their destructors may route through GFX_DEVICE)
		_DELETE(bufferMgr);
		_DELETE(renderStateMgr);
		_DELETE(samplerStateMgr);

		if(gfxDevice && gfxDevice->IsDX12())
		{
			// For DX12: the frame scheduler is owned by DX12Device; deleting the
			// device shuts everything down (swap chain, queues, heaps).
			gfxCommandList    = nullptr;
			gfxFrameScheduler = nullptr;
			_DELETE(gfxDevice);
		}
		else
		{
			_DELETE(gfxFrameScheduler);
			_DELETE(gfxDevice);	// DX11Device destructor releases DX11 device/context/DXGI
			gfxCommandList = nullptr;
		}
	}
//------------------------------------------------------------------
}

