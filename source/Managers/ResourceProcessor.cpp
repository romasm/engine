#include "stdafx.h"
#include "ResourceProcessor.h"

#include "WorldMgr.h"
#include "FontMgr.h"
#include "MaterialMgr.h"
#include "ShaderCodeMgr.h"
#include "ShaderMgr.h"
#include "StMeshMgr.h"
#include "TexMgr.h"
#include "TexLoader.h"

using namespace EngineCore;

ResourceProcessor *ResourceProcessor::instance = nullptr;

ResourceProcessor::ResourceProcessor()
{
	if(!instance)
	{
		instance = this;

		loadingQueue = new RQueueLockfree<ResourceSlot>(LOADING_QUEUE_SIZE);
		postLoadingQueue = new RQueueLockfree<ResourceSlot>(LOADING_QUEUE_SIZE);

		loaderRunning = true;
		loader = new thread(&ResourceProcessor::Loading, &(*this));
		
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
#ifdef _DEV
	JOBSYSTEM->deletePeriodicalJob(SHADER_JOB_NAME);
	JOBSYSTEM->deletePeriodicalJob(SHADERCODE_JOB_NAME);
#endif
	JOBSYSTEM->deletePeriodicalJob(TEXTURE_JOB_NAME);
	JOBSYSTEM->deletePeriodicalJob(STMESH_JOB_NAME);

	loaderRunning = false;
	v_loadingRequest.notify_all();

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

	_DELETE(loadingQueue);
	_DELETE(postLoadingQueue);
	
	instance = nullptr;
}

void ResourceProcessor::Tick()
{
	ResourceSlot loadedSlot;
	while(loadingQueue->pop(loadedSlot))
	{
		switch(loadedSlot.type)
		{
		case ResourceType::TEXTURE:
			texMgr->OnPostLoadMainThread(loadedSlot.id, loadedSlot.callback);
			break;

		case ResourceType::MESH:
			stmeshMgr->OnPostLoadMainThread(loadedSlot.id, loadedSlot.callback);
			break;

		default:
			continue;
		}
	}
}

void ResourceProcessor::Loading()
{
	DBG_SHORT("Start loading tread %u ", JobSystem::GetThreadID());

	while(loaderRunning)
	{
		// wait loading request
		{
			unique_lock<mutex> l(m_loading);
			v_loadingRequest.wait(l);
			l.unlock();
		}

		ResourceSlot loadingSlot;
		while(loadingQueue->pop(loadingSlot))
		{
			loadingSlot.status = LoadingStatus::FAILED;

			switch(loadingSlot.type)
			{
			case ResourceType::TEXTURE:
				{
					string& fileName = texMgr->GetName(loadingSlot.id);
					uint32_t size = 0;
					uint8_t* data = FileIO::ReadFileData(fileName, &size);
					if(data)
					{
						auto loadedData = TexLoader::LoadFromMemory(fileName, data, size);
						_DELETE_ARRAY(data);

						if(loadedData)
						{
							texMgr->OnLoad(loadingSlot.id, loadedData);
							loadingSlot.status = LOADED;
						}
					}
				}
				break;

			case ResourceType::MESH:
				{
					string& fileName = stmeshMgr->GetName(loadingSlot.id);
					uint32_t size = 0;
					uint8_t* data = FileIO::ReadFileData(fileName, &size);
					if(data)
					{
						auto loadedData = StMeshLoader::LoadFromMemory(fileName, data, size);
						_DELETE_ARRAY(data);

						if(loadedData)
						{
							stmeshMgr->OnLoad(loadingSlot.id, loadedData);
							loadingSlot.status = LOADED;
						}
					}
				}
				break;

			default:
				continue;
			}

			if(!postLoadingQueue->push(loadingSlot))
				WRN("Resource post loading queue overflow!");
		}
	}

	DBG_SHORT("End loading tread %u ", JobSystem::GetThreadID());
}

bool ResourceProcessor::QueueLoad(uint32_t id, ResourceType type, onLoadCallback callback)
{
	if(!loadingQueue->push(ResourceSlot(id, type, callback)))
	{
		WRN("Resource loading queue overflow!");
		return false;
	}
	v_loadingRequest.notify_one();
	return true;
}

void ResourceProcessor::AddUpdateJobs()
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