#include "stdafx.h"
#include "ResourceProcessor.h"

#include "WorldMgr.h"
#include "FontMgr.h"
#include "MaterialMgr.h"
#include "ShaderCodeMgr.h"
#include "ShaderMgr.h"
#include "StMeshMgr.h"
#include "TexMgr.h"

using namespace EngineCore;

ResourceProcessor *ResourceProcessor::instance = nullptr;

ResourceProcessor::ResourceProcessor()
{
	if(!instance)
	{
		instance = this;

		loaderRunning = true;
		loader = new thread(&ResourceProcessor::Update, &(*this));

		updateDelay = 0;
		updateTime = UPDATE_PERIOD_DEFAULT;

		shaderMgr = new ShaderMgr;
		shaderCodeMgr = new ShaderCodeMgr;
		fontMgr = new FontMgr;
		worldMgr = new WorldMgr;
		texMgr = new TexMgr;
		materialMgr = new MaterialMgr;
		stmeshMgr = new StMeshMgr;
	}
	else
		ERR("Only one instance of ResourceProcessor is allowed!");
}

ResourceProcessor::~ResourceProcessor()
{
	loaderRunning = false;
	v_updateRequest.notify_all();

	if(loader->joinable())
		loader->join();
	delete loader;

	_DELETE(worldMgr);
	_DELETE(fontMgr);
	_DELETE(stmeshMgr);		
	_DELETE(materialMgr);
	_DELETE(shaderMgr);
	_DELETE(texMgr);
	_DELETE(shaderCodeMgr);
	
	instance = nullptr;
}

void ResourceProcessor::Tick(float dt)
{
	updateDelay += dt;

	if(updateDelay >= updateTime)
	{
		updateDelay = 0;
		v_updateRequest.notify_one();
	}
}

void ResourceProcessor::ForceUpdate()
{
	updateDelay = 0;
	v_updateRequest.notify_one();
}

void ResourceProcessor::Update()
{
	DBG_SHORT("Start loader tread %u ", JobSystem::GetThreadID());

	while(loaderRunning)
	{
		// wait loading request
		{
			unique_lock<mutex> l(m_update);
			v_updateRequest.wait(l);
			l.unlock();
		}

		// MGRs update
#ifdef _DEV
		shaderMgr->UpdateShaders();
		shaderCodeMgr->UpdateShadersCode();
#endif
		texMgr->UpdateTextures();
		stmeshMgr->UpdateStMeshes();
	}

	DBG_SHORT("End loading tread %u ", JobSystem::GetThreadID());
}

// TODO: move preloading managment to Lua
void ResourceProcessor::Preload()
{
	shaderCodeMgr->PreloadPureCodes();
	shaderMgr->PreloadShaders();

	texMgr->PreloadTextures();

	stmeshMgr->PreloadStMeshes();
		
	fontMgr->PreloadFonts();
	
	// force update
	texMgr->UpdateTextures();
}