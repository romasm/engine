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

#ifdef _EDITOR
		importQueue = new RQueueLockfree<ImportSlot>(IMPORT_QUEUE_SIZE);
		postImportQueue = new RQueueLockfree<ImportSlot>(IMPORT_QUEUE_SIZE);
#endif

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
	_CLOSE(meshMgr);	
	_CLOSE(collisionMgr);		
	_DELETE(materialMgr);
	_CLOSE(shaderMgr);
	_CLOSE(texMgr);
	_DELETE(shaderCodeMgr);

	_DELETE(loadingQueue);
	_DELETE(postLoadingQueue);

#ifdef _EDITOR
	_DELETE(importQueue);
	_DELETE(postImportQueue);
#endif

	instance = nullptr;
}

void ResourceProcessor::Tick()
{
#ifdef _EDITOR
	if( meshMgr->IsBBoxesDirty() )
		worldMgr->PostMeshesReload();

	ImportSlot importSlot;
	while(postImportQueue->pop(importSlot))
	{
		if(importSlot.callback)
			importSlot.callback( importSlot.info, importSlot.status == LoadingStatus::LOADED );
	}
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

#ifdef _EDITOR
		ImportSlot importSlot;
		while(importQueue->pop(importSlot))
		{
			if( importSlot.status != LoadingStatus::LOADED )
			{
				if( importResource(importSlot.info) )
					importSlot.status = LoadingStatus::LOADED;
			}

			if(!postImportQueue->push(importSlot))
				WRN("Resource post impoting queue overflow!");
		}
#endif

		loadingComplete = false;
		ResourceSlot loadingSlot;
		while(loadingQueue->pop(loadingSlot))
		{
			if( loadingSlot.status != LoadingStatus::LOADED )
			{
				if( loadResource(loadingSlot) )
					loadingSlot.status = LoadingStatus::LOADED;
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

bool ResourceProcessor::ImportResource(const ImportInfo& info)
{
	// TODO
	return true;
}

bool ResourceProcessor::loadResource(const ResourceSlot& loadingSlot)
{
	switch(loadingSlot.type)
	{
	case ResourceType::TEXTURE:
		{
			auto loadedData = TexLoader::LoadTexture(texMgr->GetName(loadingSlot.id));
			if(loadedData)
			{
				texMgr->OnLoad(loadingSlot.id, loadedData);
				return true;
			}
		}
		break;

	case ResourceType::MESH:
		{
			auto loadedData = MeshLoader::LoadMesh(meshMgr->GetName(loadingSlot.id));
			if(loadedData)
			{
				meshMgr->OnLoad(loadingSlot.id, loadedData);
				return true;
			}
		}
		break;

	case ResourceType::COLLISION:
		{
			auto loadedData = CollisionLoader::LoadCollision(collisionMgr->GetName(loadingSlot.id));
			if(loadedData)
			{
				collisionMgr->OnLoad(loadingSlot.id, loadedData);
				return true;
			}
		}
		break;

	case ResourceType::SHADER:
		ERR("TODO: Move shader & shader code loading\\compiling in ResourceProcessor");
		break;
	}

	return false;
}

bool ResourceProcessor::QueueLoad(uint32_t id, ResourceType type, onLoadCallback callback, bool clone)
{
	ResourceSlot slot(id, type, callback);
	if(clone)
		slot.status = LoadingStatus::LOADED;

	if(!loadingQueue->push(slot))
	{
		WRN("Resource loading queue overflow!");
		return false;
	}
	v_loadingRequest.notify_one();
	return true;
}

bool ResourceProcessor::QueueImport(ImportInfo info, onImportCallback callback, bool clone)
{
#ifdef _EDITOR
	ImportSlot slot(info, callback);
	if(clone)
		slot.status = LoadingStatus::LOADED;

	if(!importQueue->push(slot))
	{
		WRN("Resource importing queue overflow!");
		return false;
	}
	v_loadingRequest.notify_one();
#endif
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

uint32_t GetTextureCallbackLua(string path, LuaRef func)
{
	// TODO: potential memory leak
	// This fixes wrong LuaRef capture by lambda
	LuaRef* luaRef = new LuaRef(func);

	return TexMgr::Get()->GetResource(path, CONFIG(bool, reload_resources), 
		[luaRef](uint32_t id, bool status) -> void
	{
		if(luaRef->isFunction())
			(*luaRef)(id, status);
		_DELETE((LuaRef*)luaRef);
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