#include "stdafx.h"
#include "Render.h"
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
	
	void Render::RegLuaClass()
	{
		GlobalColor::RegLuaFunctions();
		ResourceProcessor::RegLuaFunctions();
	}
	
	Render* Render::m_instance = nullptr;
	ID3D11RenderTargetView** Render::rts_null = nullptr;

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
		_RELEASE(m_pImmediateContext3);
		_RELEASE(m_pd3dDevice);
		_RELEASE(m_pd3dDevice3);
		_RELEASE(m_pDXGIDevice);
		_RELEASE(m_pDxgiAdapter);
		_RELEASE(m_pDxgiFactory);
	}
//------------------------------------------------------------------
}

