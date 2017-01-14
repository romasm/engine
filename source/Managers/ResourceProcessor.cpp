#include "stdafx.h"
#include "ResourceProcessor.h"
#include "ShaderCodeMgr.h"
#include "ShaderMgr.h"
#include "StMeshMgr.h"
#include "TexMgr.h"
#include "Common.h"

using namespace EngineCore;

ResourceProcessor *ResourceProcessor::instance = nullptr;

ResourceProcessor::ResourceProcessor()
{
	if(!instance)
	{
		instance = this;
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