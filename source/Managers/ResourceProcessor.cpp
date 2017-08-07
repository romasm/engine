#include "stdafx.h"
#include "ResourceProcessor.h"

#include "WorldMgr.h"
#include "FontMgr.h"
#include "MaterialMgr.h"
#include "ShaderCodeMgr.h"
#include "ShaderMgr.h"
#include "MeshMgr.h"
#include "CollisionMgr.h"
#include "TexMgr.h"
#include "TexLoader.h"
#include "MeshLoader.h"
#include "CollisionLoader.h"

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
		loadingComplete = true;
		loader = new thread(&ResourceProcessor::ThreadMain, &(*this));
		
		shaderMgr = new ShaderMgr;
		shaderCodeMgr = new ShaderCodeMgr;
		fontMgr = new FontMgr;
		worldMgr = new WorldMgr;
		texMgr = new TexMgr;
		materialMgr = new MaterialMgr;
		meshMgr = new MeshMgr;
		collisionMgr = new CollisionMgr;
	}
	else
		ERR("Only one instance of ResourceProcessor is allowed!");
}

void ResourceProcessor::DeleteUpdateJobs()
{
#ifdef _DEV
	JOBSYSTEM->deletePeriodicalJob(SHADER_JOB_NAME);
	JOBSYSTEM->deletePeriodicalJob(SHADERCODE_JOB_NAME);
#endif
	JOBSYSTEM->deletePeriodicalJob(TEXTURE_JOB_NAME);
	JOBSYSTEM->deletePeriodicalJob(STMESH_JOB_NAME);
}

ResourceProcessor::~ResourceProcessor()
{
	loaderRunning = false;
	v_loadingRequest.notify_all();

	if(loader->joinable())
		loader->join();
	delete loader;

	_DELETE(worldMgr);
	_DELETE(fontMgr);
	_DELETE(meshMgr);	
	_DELETE(collisionMgr);		
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
#ifdef _EDITOR
	if( meshMgr->IsBBoxesDirty() )
		worldMgr->PostMeshesReload();
#endif

	ResourceSlot loadedSlot;
	while(postLoadingQueue->pop(loadedSlot))
	{
		switch(loadedSlot.type)
		{
		case ResourceType::TEXTURE:
			texMgr->OnPostLoadMainThread(loadedSlot.id, loadedSlot.callback, loadedSlot.status);
			break;

		case ResourceType::MESH:
			meshMgr->OnPostLoadMainThread(loadedSlot.id, loadedSlot.callback, loadedSlot.status);
			break;

		case ResourceType::COLLISION:
			collisionMgr->OnPostLoadMainThread(loadedSlot.id, loadedSlot.callback, loadedSlot.status);
			break;

		default:
			continue;
		}
	}
}

void ResourceProcessor::ThreadMain()
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

		loadingComplete = false;
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
					string& fileName = meshMgr->GetName(loadingSlot.id);
					uint32_t size = 0;
					uint8_t* data = FileIO::ReadFileData(fileName, &size);
					if(data)
					{
						auto loadedData = MeshLoader::LoadStaticMeshFromMemory(fileName, data, size);
						_DELETE_ARRAY(data);

						if(loadedData)
						{
							meshMgr->OnLoad(loadingSlot.id, loadedData);
							loadingSlot.status = LOADED;
						}
					}
				}
				break;

			case ResourceType::COLLISION:
				{
					string& fileName = collisionMgr->GetName(loadingSlot.id);
					uint32_t size = 0;
					uint8_t* data = FileIO::ReadFileData(fileName, &size);
					if(data)
					{
						auto loadedData = CollisionLoader::LoadCollisionFromMemory(fileName, data, size);
						_DELETE_ARRAY(data);

						if(loadedData)
						{
							collisionMgr->OnLoad(loadingSlot.id, loadedData);
							loadingSlot.status = LOADED;
						}
					}
				}
				break;

			case ResourceType::SHADER:
				ERR("TODO: Move shader & shader code loading\\compiling in ResourceProcessor");
				break;
				
			default:
				continue;
			}

			if(!postLoadingQueue->push(loadingSlot))
				WRN("Resource post loading queue overflow!");
		}

		loadingComplete = true;
		v_loadingComplete.notify_all();

		// Deallocate old data
		texMgr->DefferedDeallocate();
		meshMgr->DefferedDeallocate();
		collisionMgr->DefferedDeallocate();
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

// TODO: forever lock? Hacked for unlock after 10 sec
void ResourceProcessor::WaitLoadingComplete()
{
	if( !loadingComplete )
	{
		// wait loading compete
		unique_lock<mutex> l(m_complete);
		if( v_loadingComplete.wait_for(l, chrono::seconds(10)) == cv_status::timeout )
			ERR("TODO: Waiting stucked and has been released after 10 sec!");
		l.unlock();
	}
}

void ResourceProcessor::AddUpdateJobs()
{
#ifdef _EDITOR

	#ifdef _DEV
		JOBSYSTEM->addPeriodicalJob(SHADER_JOB_NAME, JOB_F_MEMBER(ShaderMgr, ShaderMgr::Get(), CheckForReload), 
			SHADERS_UPDATE_PERIOD, JobPriority::BACKGROUND);
		JOBSYSTEM->addPeriodicalJob(SHADERCODE_JOB_NAME, JOB_F_MEMBER(ShaderCodeMgr, ShaderCodeMgr::Get(), CheckForReload), 
			SHADERS_UPDATE_PERIOD, JobPriority::BACKGROUND);
	#endif

	JOBSYSTEM->addPeriodicalJob(TEXTURE_JOB_NAME, JOB_F_MEMBER(TexMgr, TexMgr::Get(), CheckForReload), 
		TEXTURES_UPDATE_PERIOD, JobPriority::BACKGROUND);
	JOBSYSTEM->addPeriodicalJob(STMESH_JOB_NAME, JOB_F_MEMBER(MeshMgr, MeshMgr::Get(), CheckForReload), 
		STMESHES_UPDATE_PERIOD, JobPriority::BACKGROUND);

#endif
}

void ResourceProcessor::Preload(string& filename, ResourceType type)
{
	bool reload = false;
#ifdef _DEV
	reload = true;
#endif

	switch(type)
	{
	case EngineCore::TEXTURE:
		texMgr->GetResource(filename, reload);
		break;
	case EngineCore::MESH:
		meshMgr->GetResource(filename, reload);
		break;
	case EngineCore::COLLISION:
		break;
	case EngineCore::SKELETON:
		break;
	case EngineCore::ANIMATION:
		break;
	case EngineCore::SHADER:
		shaderMgr->GetResource(filename, false);
		break;
	case EngineCore::GUI_SHADER:
		shaderMgr->GetResource(filename, true);
		break;
	case EngineCore::COMPUTE:
		{
			auto del = filename.find("@");
			if( del == string::npos )
			{
				ERR("Wrong compute shader path@entry!");
			}
			else
			{
				string file = filename.substr(0, del);
				string entry = filename.substr(del + 1);
				Compute::Preload( file, entry );
			}
		}
		break;
	case EngineCore::FONT:
		fontMgr->GetFont(filename);
		break;
	case EngineCore::MATERIAL:
		break;
	}
}

// LUA FUNCTIONS

uint32_t GetTextureLua(string path)
{
	return RELOADABLE_TEXTURE(path, CONFIG(bool, reload_resources));
}

uint32_t GetTextureCallbackLua(string path, LuaRef func, LuaRef self)
{
	return TexMgr::Get()->GetResource(path, CONFIG(bool, reload_resources), 
		[func, self](uint32_t id, bool status) -> void
	{
		if(func.isFunction())
			func(self, id, status);
	});
}

void DropTextureLua(string path)
{
	TEXTURE_NAME_DROP(path);
}

void ConvertMeshToEngineFormat(string file)
{
	MeshLoader::ConvertStaticMeshToEngineFormat(file);
}

Material* GetMaterialLua(string name)
{
	return MaterialMgr::Get()->GetMaterial(name);
}

void DropMaterialLua(string name)
{
	MaterialMgr::Get()->DeleteMaterial(name);
}

void PreloadResource(string filename, uint32_t type)
{
	ResourceProcessor::Get()->Preload(filename, ResourceType(type));
}

void WaitLoadingCompleteLua()
{
	ResourceProcessor::Get()->WaitLoadingComplete();
}

void ResourceProcessor::RegLuaFunctions()
{
	getGlobalNamespace(LSTATE)
		.beginNamespace("Resource")
		.addFunction("PreloadResource", &PreloadResource)
		.addFunction("WaitLoadingComplete", &WaitLoadingCompleteLua)
		.addFunction("GetTexture", &GetTextureLua)
		.addFunction("GetTextureCallback", &GetTextureCallbackLua)
		.addFunction("DropTexture", &DropTextureLua)
		.addFunction("GetMaterial", &GetMaterialLua)
		.addFunction("DropMaterial", &DropMaterialLua)
		.addFunction("IsMeshSupported", &MeshLoader::IsSupported)
		.addFunction("IsTextureSupported", &TexLoader::IsSupported)

		.addFunction("ConvertMeshToEngineFormat", &ConvertMeshToEngineFormat)
		.endNamespace();
}