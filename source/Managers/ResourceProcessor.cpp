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
		
		materialMgr = nullptr;
		texMgr = nullptr;
		stmeshMgr = nullptr;
		
		shaderMgr = new ShaderMgr;
		shaderCodeMgr = new ShaderCodeMgr;
		fontMgr = new FontMgr;
		worldMgr = new WorldMgr;
	}
	else
		ERR("Only one instance of ResourceProcessor is allowed!");
}

ResourceProcessor::~ResourceProcessor()
{
#ifdef _DEV
	JOBSYSTEM->deletePeriodicalJob(SHADER_JOB_NAME);
	JOBSYSTEM->deletePeriodicalJob(SHADERCODE_JOB_NAME);
#endif
	JOBSYSTEM->deletePeriodicalJob(TEXTURE_JOB_NAME);
	JOBSYSTEM->deletePeriodicalJob(STMESH_JOB_NAME);
	
	_DELETE(worldMgr);
	_DELETE(fontMgr);
	_DELETE(stmeshMgr);		
	_DELETE(materialMgr);
	_DELETE(shaderMgr);
	_DELETE(texMgr);
	_DELETE(shaderCodeMgr);
}

void ResourceProcessor::StartUpdate()
{
#ifdef _DEV
	JOBSYSTEM->addPeriodicalJob(SHADER_JOB_NAME, JOB_F_MEMBER(ShaderMgr, ShaderMgr::Get(), UpdateShaders), 
		SHADERS_UPDATE_PERIOD, JobPriority::BACKGROUND);
	JOBSYSTEM->addPeriodicalJob(SHADERCODE_JOB_NAME, JOB_F_MEMBER(ShaderCodeMgr, ShaderCodeMgr::Get(), UpdateShadersCode), 
		SHADERS_UPDATE_PERIOD, JobPriority::BACKGROUND);
#endif
	JOBSYSTEM->addPeriodicalJob(TEXTURE_JOB_NAME, JOB_F_MEMBER(TexMgr, TexMgr::Get(), UpdateTextures), 
		TEXTURES_UPDATE_PERIOD, JobPriority::BACKGROUND);
	JOBSYSTEM->addPeriodicalJob(STMESH_JOB_NAME, JOB_F_MEMBER(StMeshMgr, StMeshMgr::Get(), UpdateStMeshes), 
		STMESHES_UPDATE_PERIOD, JobPriority::BACKGROUND);
}

void ResourceProcessor::Preload()
{
	shaderCodeMgr->PreloadPureCodes();
	shaderMgr->PreloadShaders();

	texMgr = new TexMgr;
	texMgr->PreloadTextures();

	materialMgr = new MaterialMgr;
	
	stmeshMgr = new StMeshMgr;
	stmeshMgr->PreloadStMeshes();
		
	fontMgr->PreloadFonts();
	
	// force update
	texMgr->UpdateTextures();
}