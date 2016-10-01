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

#include "ResourceProcessor.h"

namespace EngineCore
{
//------------------------------------------------------------------
	
	uint32_t GetTextureLua(string path)
	{
		return RELOADABLE_TEXTURE(path, true); // todo
	}

	void ForceTextureReload()
	{
		TexMgr::Get()->UpdateTextures();
	}

	void ForceTextureReloadBackground()
	{
		JOBSYSTEM->periodicalJobFillTimer(TEXTURE_JOB_NAME);
	}

	void ConvertMeshToSTM(string file)
	{
		StMeshMgr::Get()->SaveSTMFile(file);
	}

	void Render::RegLuaClass()
	{
		getGlobalNamespace(LSTATE)
			.beginNamespace("Resource")
				.addFunction("GetTexture", &GetTextureLua)
				.addFunction("ForceTextureReload", &ForceTextureReload)
				.addFunction("ForceTextureReloadBackground", &ForceTextureReloadBackground)

				.addFunction("ConvertMeshToSTM", &ConvertMeshToSTM)
			.endNamespace();
	}
	
	Render *Render::m_instance = nullptr;

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

			CurrentHudWindow = nullptr;

			materialMgr = nullptr;
			texMgr = nullptr;

			shaderMgr = new ShaderMgr;

			shaderCodeMgr = new ShaderCodeMgr;
			renderStateMgr = nullptr;
			samplerStateMgr = nullptr;

			m_stmeshmgr = nullptr;
			m_fontmgr = new FontMgr;
			m_worldmgr = new WorldMgr;
			
			resourceProc = new ResourceProcessor;

			BufferObj = nullptr;

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
	}

	bool Render::Draw() // todo: remove?
	{	
		m_worldmgr->UpdateWorlds();

		return true;
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
		
		shaderMgr->PreloadShaders();

		texMgr = new TexMgr;
		texMgr->PreloadTextures();

		materialMgr = new MaterialMgr;

		m_stmeshmgr = new StMeshMgr;

		wim_map = WindowsMgr::Get()->GetMap();

		BufferObj = new Buffer();
		
		return true;
	}
	
	void Render::BeginFrame()
	{
		for(auto& it : *wim_map)
		{
			it.second->ClearRenderTarget();
		}
	}

	void Render::EndFrame()
	{
		for(auto& it : *wim_map)
		{
			it.second->Swap();
		}
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
		if( m_pImmediateContext ) 
			m_pImmediateContext->ClearState();
	
		_DELETE(m_fontmgr);
		_DELETE(m_stmeshmgr);
		
		_DELETE(materialMgr);
		_DELETE(shaderMgr);
		_DELETE(texMgr);

		_DELETE(shaderCodeMgr);

		_DELETE(renderStateMgr);
		_DELETE(samplerStateMgr);

		_DELETE(resourceProc);

		_DELETE(BufferObj);

		_RELEASE(m_pImmediateContext);
		_RELEASE(m_pd3dDevice);
		_RELEASE(m_pDXGIDevice);
		_RELEASE(m_pDxgiAdapter);
		_RELEASE(m_pDxgiFactory);
	}
//------------------------------------------------------------------
}

