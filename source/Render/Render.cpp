#include "stdafx.h"
#include "Render.h"
#include "macros.h"
#include "Image.h"
#include "Font.h"
#include "Text.h"
#include "Log.h"
#include "ShaderMgr.h"
#include "MaterialMgr.h"
#include "StMeshMgr.h"
#include "FontMgr.h"
#include "ScenePipeline.h"
#include "WorldMgr.h"
#include "Utils/Profiler.h"

#include "ResourceProcessor.h"

namespace EngineCore
{
//------------------------------------------------------------------
	
	uint32_t GetTextureLua(string path)
	{
		return RELOADABLE_TEXTURE(path, true); // todo
	}
	
	void DropTextureLua(string path)
	{
		TEXTURE_NAME_DROP(path);
	}

	void ConvertMeshToSTM(string file)
	{
		StMeshMgr::Get()->SaveSTMFile(file);
	}

	Material* GetMaterialLua(string name)
	{
		return MaterialMgr::Get()->GetMaterial(name);
	}

	void DropMaterialLua(string name)
	{
		MaterialMgr::Get()->DeleteMaterial(name);
	}

	void PreloadSystemResources()
	{
		ResourceProcessor::Get()->Preload();
	}

	void Render::RegLuaClass()
	{
		GlobalColor::RegLuaFunctions();

		getGlobalNamespace(LSTATE)
			.beginNamespace("Resource")
				.addFunction("PreloadSystemResources", &PreloadSystemResources)
				.addFunction("GetTexture", &GetTextureLua)
				.addFunction("DropTexture", &DropTextureLua)
				.addFunction("GetMaterial", &GetMaterialLua)
				.addFunction("DropMaterial", &DropMaterialLua)

				.addFunction("ConvertMeshToSTM", &ConvertMeshToSTM)
			.endNamespace();
	}
	
	Render* Render::m_instance = nullptr;
	ID3D11RenderTargetView** Render::rts_null = nullptr;

	Render::Render()
	{
		if(!m_instance)
		{
			m_instance = this;
			m_pd3dDevice = nullptr;
			m_pDXGIDevice = nullptr;
			m_pDxgiAdapter = nullptr;
			m_pDxgiFactory = nullptr;
			m_pImmediateContext = nullptr;
			renderStateMgr = nullptr;
			samplerStateMgr = nullptr;
			bufferMgr = nullptr;

			// remove
			CurrentHudWindow = nullptr;

			rts_null = new ID3D11RenderTargetView*[8];
			for(uint8_t i = 0; i < 8; i++)
				rts_null[i] = nullptr;

			RegLuaClass();
		}
		else
		{
			ERR("Повтороное создание Render");
		}
	}

	Render::~Render()
	{
		m_instance = nullptr;
		_DELETE(rts_null);
	}
	
	bool Render::Init()
	{
		// DEVICE
		if ( !m_createdevice() )
		{
			ERR("Cant create DirectX Device!");
			return false;
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
		UINT createDeviceFlags = 0;
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
		UINT numFeatureLevels = ARRAYSIZE( featureLevels );
	
		HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, 
			featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &m_pd3dDevice, nullptr, &m_pImmediateContext );
		if( FAILED(hr) )
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

	void Render::Close()
	{
	#ifdef _DEV
		Profiler::Get()->ReleaseQueries();
	#endif

		if( m_pImmediateContext ) 
			m_pImmediateContext->ClearState();
	
		_DELETE(renderStateMgr);
		_DELETE(samplerStateMgr);

		_DELETE(bufferMgr);

		_RELEASE(m_pImmediateContext);
		_RELEASE(m_pd3dDevice);
		_RELEASE(m_pDXGIDevice);
		_RELEASE(m_pDxgiAdapter);
		_RELEASE(m_pDxgiFactory);
	}
//------------------------------------------------------------------
}

