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
				if( ImportResource(importSlot.info) )
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

bool ResourceProcessor::ImportResource(ImportInfo& info)
{
	bool status = false;

	uint32_t sourceDate = FileIO::GetDateModifRaw(info.filePath);

	if( (info.importBytes & IMP_BYTE_TEXTURE) > 0 )
	{
		string resFile = info.resourceName + EXT_TEXTURE;
		if(CheckImportNeeded(info, sourceDate, resFile))
		{
			status = status || TexLoader::ConvertTextureToEngineFormat(info.filePath, resFile, info.genMips, info.genMipsFilter);

			ImportInfo imp = info;
			imp.importBytes = IMP_BYTE_TEXTURE;
			SaveImportInfo(resFile, imp);
		}
	}
	else if( (info.importBytes & IMP_BYTE_MESH) > 0 || 
		(info.importBytes & IMP_BYTE_SKELETON) > 0 || 
		(info.importBytes & IMP_BYTE_COLLISION) > 0 || 
		(info.importBytes & IMP_BYTE_ANIMATION) > 0 )
	{
		if( (info.importBytes & IMP_BYTE_MESH) > 0 )
		{
			string resFile = info.resourceName + EXT_MESH;
			if(CheckImportNeeded(info, sourceDate, resFile))
			{
				status = status || MeshLoader::ConvertMeshToEngineFormat(info.filePath, resFile, info.isSkinnedMesh);

				ImportInfo imp = info;
				imp.importBytes = IMP_BYTE_MESH;
				SaveImportInfo(resFile, imp);
			}
		}

		if( (info.importBytes & IMP_BYTE_COLLISION) > 0 )
		{
			string resFile = info.resourceName + EXT_COLLISION;
			if(CheckImportNeeded(info, sourceDate, resFile))
			{
				status = status || CollisionLoader::ConvertCollisionToEngineFormat(info.filePath, resFile);

				ImportInfo imp = info;
				imp.importBytes = IMP_BYTE_COLLISION;
				SaveImportInfo(resFile, imp);
			}
		}

		// TODO
	}
	else
	{
		WRN("Nothing to import for %s", info.filePath.c_str());
		return false;
	}

	return true;
}

bool ResourceProcessor::loadResource(const ResourceSlot& loadingSlot)
{
	ImportInfo info;
	uint32_t sourceDate;

	switch(loadingSlot.type)
	{
	case ResourceType::TEXTURE:
		{
			string& name = texMgr->GetName(loadingSlot.id);
			auto loadedData = TexLoader::LoadTexture(name);
			if(loadedData)
			{
				LoadImportInfo(name, info, sourceDate);
				texMgr->OnLoad(loadingSlot.id, loadedData, info, sourceDate);
				return true;
			}
		}
		break;

	case ResourceType::MESH:
		{
			string& name = meshMgr->GetName(loadingSlot.id);
			auto loadedData = MeshLoader::LoadMesh(name);
			if(loadedData)
			{
				LoadImportInfo(name, info, sourceDate);
				meshMgr->OnLoad(loadingSlot.id, loadedData, info, sourceDate);
				return true;
			}
		}
		break;

	case ResourceType::COLLISION:
		{
			string& name = collisionMgr->GetName(loadingSlot.id);
			auto loadedData = CollisionLoader::LoadCollision(name);
			if(loadedData)
			{
				LoadImportInfo(name, info, sourceDate);
				collisionMgr->OnLoad(loadingSlot.id, loadedData, info, sourceDate);
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

bool ResourceProcessor::SaveImportInfo(string& resFile, ImportInfo& info)
{
#ifdef _EDITOR
	string impFile = resFile + EXT_IMPORT;
	
	uint32_t fileSize = sizeof(uint32_t) + sizeof(uint32_t);
	fileSize += sizeof(uint32_t) + (uint32_t)info.filePath.size();
	fileSize += sizeof(uint32_t) + (uint32_t)info.resourceName.size();
	fileSize += ImportInfo::sizeNoString();

	uint8_t* data = new uint8_t[fileSize];
	uint8_t* t_data = data;

	*(uint32_t*)t_data = IMPORT_FILE_VERSION;
	t_data += sizeof(uint32_t);

	*(uint32_t*)t_data = FileIO::GetDateModifRaw(info.filePath);
	t_data += sizeof(uint32_t);

	*(uint32_t*)t_data = (uint32_t)info.filePath.size();
	t_data += sizeof(uint32_t);

	memcpy(t_data, info.filePath.data(), info.filePath.size());
	t_data += info.filePath.size();

	*(uint32_t*)t_data = (uint32_t)info.resourceName.size();
	t_data += sizeof(uint32_t);

	memcpy(t_data, info.resourceName.data(), info.resourceName.size());
	t_data += info.resourceName.size();

	memcpy(t_data, &info.importBytes, ImportInfo::sizeNoString());
	t_data += ImportInfo::sizeNoString();

	bool status;
	if( !(status = FileIO::WriteFileData(impFile, data, fileSize)) )
	{
		ERR("Cant write import file %s", impFile.c_str() );
	}
	_DELETE_ARRAY(data);
	return status;
#endif
}

void ResourceProcessor::LoadImportInfo(string& resName, ImportInfo& info, uint32_t& date)
{
#ifdef _EDITOR
	string impFile = resName + EXT_IMPORT;

	uint32_t size = 0;
	uint8_t* fdata = FileIO::ReadFileData(impFile, &size, true);
	uint8_t* t_data = fdata;

	if(!fdata || (*(uint32_t*)t_data) != IMPORT_FILE_VERSION )
	{
		date = FileIO::GetDateModifRaw(resName);
		info.filePath = resName;
		info.resourceName = RemoveExtension(resName);
	}
	else
	{
		t_data += sizeof(uint32_t);

		date = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		uint32_t stringSize = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		info.filePath = string((char*)t_data, stringSize);
		t_data += sizeof(char) * stringSize;

		stringSize = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		info.resourceName = string((char*)t_data, stringSize);
		t_data += sizeof(char) * stringSize;

		memcpy(&info.importBytes, t_data, ImportInfo::sizeNoString());
		t_data += ImportInfo::sizeNoString();
	}		

	_DELETE_ARRAY(fdata);
#endif
}

bool ResourceProcessor::CheckImportNeeded(ImportInfo& info, uint32_t date, string& resFile)
{
#ifdef _EDITOR
	ImportInfo oldInfo;
	uint32_t oldDate;
	LoadImportInfo(resFile, oldInfo, oldDate);

	if(oldInfo.importBytes == 0)
		return true;

	if( oldInfo.filePath != info.filePath )
		return true;

	if( oldDate != date )
		return true;

	uint8_t* oldSettings = ((uint8_t*)&oldInfo.importBytes) + 4;
	uint8_t* newSettings = ((uint8_t*)&info.importBytes) + 4;

	for(uint32_t i = 0; i < ImportInfo::sizeNoString() - 4; i++)
	{
		if( *(oldSettings + i) != *(newSettings + i) )
			return true;
	}

	LOG_GOOD("Importing %s has already done, skipped", resFile.data());

#endif
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
		collisionMgr->GetResource(filename, reload);
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

void ImportInfo::parseFromLua(LuaRef params)
{
	if(!params.isTable())
	{
		ERR("Import params table is expected here");
		return;
	}

	LuaRef filePath = params["filePath"];
	this->filePath = filePath.type() == LUA_TSTRING ? filePath.cast<string>() : "";
	LuaRef resourceName = params["resourceName"];
	this->resourceName = resourceName.type() == LUA_TSTRING ? resourceName.cast<string>() : "";

	LuaRef importTexture = params["importTexture"];
	if( importTexture.type() == LUA_TBOOLEAN && params["importTexture"].cast<bool>() )
		this->importBytes |= IMP_BYTE_TEXTURE;
	LuaRef importMesh = params["importMesh"];
	if( importMesh.type() == LUA_TBOOLEAN && params["importMesh"].cast<bool>() )
		this->importBytes |= IMP_BYTE_MESH;
	LuaRef importCollision = params["importCollision"];
	if( importCollision.type() == LUA_TBOOLEAN && params["importCollision"].cast<bool>() )
		this->importBytes |= IMP_BYTE_COLLISION;
	LuaRef importSkeleton = params["importSkeleton"];
	if( importSkeleton.type() == LUA_TBOOLEAN && params["importSkeleton"].cast<bool>() )
		this->importBytes |= IMP_BYTE_SKELETON;
	LuaRef importAnimation = params["importAnimation"];
	if( importAnimation.type() == LUA_TBOOLEAN && params["importAnimation"].cast<bool>() )
		this->importBytes |= IMP_BYTE_ANIMATION;

	LuaRef isSkinnedMesh = params["isSkinnedMesh"];
	this->isSkinnedMesh = isSkinnedMesh.type() == LUA_TBOOLEAN ? isSkinnedMesh.cast<bool>() : false;
	LuaRef textureFormat = params["textureFormat"];
	this->textureFormat = textureFormat.type() == LUA_TNUMBER ? (DXGI_FORMAT)textureFormat.cast<int32_t>() : DXGI_FORMAT_R8G8B8A8_UNORM;
	LuaRef genMips = params["genMips"];
	this->genMips = genMips.type() == LUA_TBOOLEAN ? genMips.cast<bool>() : false;
	LuaRef genMipsFilter = params["genMipsFilter"];
	this->genMipsFilter = genMipsFilter.type() == LUA_TNUMBER ? (uint32_t)genMipsFilter.cast<int32_t>() : TEX_FILTER_DEFAULT;
}

// LUA FUNCTIONS

bool ImportResourceLua(LuaRef params)
{
	ImportInfo info;
	info.parseFromLua(params);
	return ResourceProcessor::Get()->QueueImport(info, nullptr, false);
}

bool ImportResourceCallbackLua(LuaRef params, LuaRef func, LuaRef data)
{
	ImportInfo info;
	info.parseFromLua(params);

	// TODO: potential memory leak
	// This fixes wrong LuaRef capture by lambda
	LuaRef* luaRef = new LuaRef(func);
	LuaRef* luaRefData = new LuaRef(data);

	return ResourceProcessor::Get()->QueueImport(info,
		[luaRef, luaRefData](const ImportInfo& info, bool status) -> void
	{
		if(luaRef->isFunction())
			(*luaRef)(info.resourceName, status, (*luaRefData));
		_DELETE((LuaRef*)luaRef);
		_DELETE((LuaRef*)luaRefData);
	}, false);
}

/*
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
}*/

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

		.addFunction("ImportResource", &ImportResourceLua)
		.addFunction("ImportResourceCallback", &ImportResourceCallbackLua)

		//.addFunction("GetTexture", &GetTextureLua)
		//.addFunction("GetTextureCallback", &GetTextureCallbackLua)
		//.addFunction("DropTexture", &DropTextureLua)

		.addFunction("GetMaterial", &GetMaterialLua)
		.addFunction("DropMaterial", &DropMaterialLua)

		.addFunction("IsMeshSupported", &MeshLoader::IsSupported)
		.addFunction("IsTextureSupported", &TexLoader::IsSupported)
		.addFunction("IsCollisionSupported", &CollisionLoader::IsSupported)
		.addFunction("IsMeshNative", &MeshLoader::IsNative)
		.addFunction("IsTextureNative", &TexLoader::IsNative)
		.addFunction("IsCollisionNative", &CollisionLoader::IsNative)
	.endNamespace();
}