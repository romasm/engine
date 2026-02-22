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
			m_pd3dDevice = nullptr;
			m_pd3dDevice3 = nullptr;
			m_pDXGIDevice = nullptr;
			m_pDxgiAdapter = nullptr;
			m_pDxgiFactory = nullptr;
			m_pImmediateContext = nullptr;
			m_pImmediateContext3 = nullptr;
			renderStateMgr = nullptr;
			samplerStateMgr = nullptr;
			bufferMgr = nullptr;
			gfxDevice = nullptr;
			gfxCommandList = nullptr;
			gfxFrameScheduler = nullptr;

			// remove
			CurrentHudWindow = nullptr;

			RegLuaClass();
		}
		else
		{
			ERR("���������� �������� Render");
		}
	}

	Render::~Render()
	{
		m_instance = nullptr;
	}
	
	bool Render::Init()
	{
		// DEVICE
		if ( !m_createdevice() )
		{
			ERR("Cant create DirectX Device!");
			return false;
		}

		// RHI backend — must be created before state managers so GFX_DEVICE/GFX_CMD are available.
		// DX12 if the config requests it, otherwise DX11.
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
			auto* dx11Device = new RHI::DX11::DX11Device(
				m_pd3dDevice, m_pd3dDevice3,
				m_pImmediateContext, m_pImmediateContext3);
			gfxDevice = dx11Device;

			auto* scheduler = new RHI::DX11::DX11FrameScheduler(
				m_pImmediateContext, m_pImmediateContext3, nullptr);
			gfxFrameScheduler = scheduler;
			gfxCommandList    = scheduler->AllocateCommandList();

			// Log DX11 device info
			DXGI_ADAPTER_DESC1 adapterDesc = {};
			if(m_pDxgiAdapter && SUCCEEDED(m_pDxgiAdapter->GetDesc1(&adapterDesc)))
			{
				char gpuName[128] = {};
				WideCharToMultiByte(CP_ACP, 0, adapterDesc.Description, -1,
					gpuName, sizeof(gpuName), nullptr, nullptr);
				size_t vramMB = adapterDesc.DedicatedVideoMemory / (1024 * 1024);

				LOG("------ GPU Device Info ------");
				LOG("  API:             DirectX 11.3");
				LOG("  GPU:             %s", gpuName);
				LOG("  VRAM:            %zu MB", vramMB);
				LOG("  Raytracing:      not supported (DX11)");
				LOG("  Mesh shaders:    not supported (DX11)");
				LOG("-----------------------------");
			}
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

	bool Render::m_createdevice()
	{
		uint32_t createDeviceFlags = 0;
	#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif
	
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};
		uint32_t numFeatureLevels = ARRAYSIZE( featureLevels );
	
		HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, 
			featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &m_pd3dDevice, nullptr, &m_pImmediateContext );
		if( FAILED(hr) )
			return false;	

		hr = m_pd3dDevice->QueryInterface(__uuidof(ID3D11Device3), reinterpret_cast<void**>(&m_pd3dDevice3));
		if (FAILED(hr))
			return false;
		hr = m_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext3), reinterpret_cast<void**>(&m_pImmediateContext3));
		if (FAILED(hr))
			return false;

		hr = m_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&m_pDXGIDevice));
		if( FAILED(hr) )
			return false;	
		hr = m_pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&m_pDxgiAdapter));
		if( FAILED(hr) )
			return false;
		hr = m_pDxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&m_pDxgiFactory));
		if( FAILED(hr) )
			return false;	

		return true;
	}

	void Render::BeginFrameDX12()
	{
		if(!m_instance || !m_instance->gfxDevice || !m_instance->gfxDevice->IsDX12())
			return;

		// Begin new frame: wait for prior GPU work, reset allocators
		m_instance->gfxFrameScheduler->BeginFrame();

		// Open the command list for recording
		m_instance->gfxCommandList->Open();

		// Set the shader-visible descriptor heaps (required before any binding)
		auto* dx12Device  = static_cast<RHI::DX12::DX12Device*>(m_instance->gfxDevice);
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

		if( m_pImmediateContext )
			m_pImmediateContext->ClearState();

		// Destroy state managers and buffers before RHI backend
		// (their destructors may route through GFX_DEVICE)
		_DELETE(bufferMgr);
		_DELETE(renderStateMgr);
		_DELETE(samplerStateMgr);

		if(gfxDevice && gfxDevice->IsDX12())
		{
			// For DX12: the frame scheduler is owned by DX12Device; deleting the
			// device shuts everything down (swap chain, queues, heaps).
			gfxCommandList    = nullptr; // owned by pool inside frame scheduler
			gfxFrameScheduler = nullptr; // owned by DX12Device
			_DELETE(gfxDevice);
		}
		else
		{
			_DELETE(gfxFrameScheduler); // owns the DX11CommandList
			_DELETE(gfxDevice);
			gfxCommandList = nullptr;
		}

		_RELEASE(m_pImmediateContext);
		_RELEASE(m_pImmediateContext3);
		_RELEASE(m_pd3dDevice);
		_RELEASE(m_pd3dDevice3);
		_RELEASE(m_pDXGIDevice);
		_RELEASE(m_pDxgiAdapter);
		_RELEASE(m_pDxgiFactory);
	}
//------------------------------------------------------------------
}

