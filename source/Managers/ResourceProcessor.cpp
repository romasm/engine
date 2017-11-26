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

		MeshLoader::Configurate();

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
		skeletonMgr = new SkeletonMgr;
		animationMgr = new AnimationMgr;
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
	_CLOSE(skeletonMgr);	
	_CLOSE(animationMgr);
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

		case ResourceType::SKELETON:
			skeletonMgr->OnPostLoadMainThread(loadedSlot.id, loadedSlot.callback, loadedSlot.status);
			break;

		case ResourceType::ANIMATION:
			animationMgr->OnPostLoadMainThread(loadedSlot.id, loadedSlot.callback, loadedSlot.status);
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
		skeletonMgr->DefferedDeallocate();
		animationMgr->DefferedDeallocate();
		collisionMgr->DefferedDeallocate();
	}

	DBG_SHORT("End loading tread %u ", JobSystem::GetThreadID());
}

bool ResourceProcessor::ImportResource(ImportInfo& info, bool force)
{
	bool status = false;

	uint32_t sourceDate = FileIO::GetDateModifRaw(info.filePath);

	if( (info.importBytes & IMP_BYTE_TEXTURE) > 0 )
	{
		string resFile = info.resourceName + EXT_TEXTURE;
		if( force || CheckImportNeeded(info, sourceDate, resFile) )
		{
			bool currentStatus = TexLoader::ConvertTextureToEngineFormat(info.filePath, resFile, info.genMips, info.genMipsFilter);
			status = status || currentStatus;

			if(currentStatus)
			{
				ImportInfo imp = info;
				imp.importBytes = IMP_BYTE_TEXTURE;
				SaveImportInfo(resFile, imp);
			}
		}
	}

	if( (info.importBytes & IMP_BYTE_MESH) > 0 )
	{
		string resFile = info.resourceName + EXT_MESH;
		if( force || CheckImportNeeded(info, sourceDate, resFile) )
		{
			bool currentStatus = MeshLoader::ConvertMeshToEngineFormat(info.filePath, resFile, info.isSkinnedMesh);
			status = status || currentStatus;

			if(currentStatus)
			{
				ImportInfo imp = info;
				imp.importBytes = IMP_BYTE_MESH;
				SaveImportInfo(resFile, imp);
			}
		}
	}

	if( (info.importBytes & IMP_BYTE_SKELETON) > 0 )
	{
		string resFile = info.resourceName + EXT_SKELETON;
		if( force || CheckImportNeeded(info, sourceDate, resFile) )
		{
			bool currentStatus = MeshLoader::ConvertSkeletonToEngineFormat(info.filePath, resFile);
			status = status || currentStatus;

			if(currentStatus)
			{
				ImportInfo imp = info;
				imp.importBytes = IMP_BYTE_SKELETON;
				SaveImportInfo(resFile, imp);
			}
		}
	}

	if( (info.importBytes & IMP_BYTE_ANIMATION) > 0 )
	{
		string resFile = info.resourceName + EXT_ANIMATION;
		if( force || CheckImportNeeded(info, sourceDate, resFile) )
		{
			bool currentStatus = MeshLoader::ConverAnimationToEngineFormat(info.filePath, resFile);
			status = status || currentStatus;

			if(currentStatus)
			{
				ImportInfo imp = info;
				imp.importBytes = IMP_BYTE_ANIMATION;
				SaveImportInfo(resFile, imp);
			}
		}
	}

	if( (info.importBytes & IMP_BYTE_COLLISION) > 0 )
	{
		string resFile = info.resourceName + EXT_COLLISION;
		if( force || CheckImportNeeded(info, sourceDate, resFile) )
		{
			bool currentStatus = CollisionLoader::ConvertCollisionToEngineFormat(info.filePath, resFile);
			status = status || currentStatus;

			if(currentStatus)
			{
				ImportInfo imp = info;
				imp.importBytes = IMP_BYTE_COLLISION;
				SaveImportInfo(resFile, imp);
			}
		}
	}
	
	if(!status)
	{
		WRN("Nothing was imported from %s", info.filePath.c_str());
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

	case ResourceType::SKELETON:
		{
			string& name = skeletonMgr->GetName(loadingSlot.id);
			auto loadedData = MeshLoader::LoadSkeleton(name);
			if(loadedData)
			{
				LoadImportInfo(name, info, sourceDate);
				skeletonMgr->OnLoad(loadingSlot.id, loadedData, info, sourceDate);
				return true;
			}
		}
		break;

	case ResourceType::ANIMATION:
		{
			string& name = animationMgr->GetName(loadingSlot.id);
			auto loadedData = MeshLoader::LoadAnimation(name);
			if(loadedData)
			{
				LoadImportInfo(name, info, sourceDate);
				animationMgr->OnLoad(loadingSlot.id, loadedData, info, sourceDate);
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

// TODO: different res pathes
bool ResourceProcessor::SaveImportInfo(string& resFile, ImportInfo& info)
{
	bool status = false;
#ifdef _EDITOR
	string impFileName = resFile + EXT_IMPORT;
	FileIO impFile(impFileName, true);

	auto general = impFile.CreateNode("import", impFile.Root());
	impFile.WriteInt("version", IMPORT_FILE_VERSION, general);
	// TODO: make it human readable
	impFile.WriteUint("fileDate", FileIO::GetDateModifRaw(info.filePath), general);
	impFile.WriteString("filePath", info.filePath, general);

	if(info.importBytes & IMP_BYTE_TEXTURE)
	{
		auto node = impFile.CreateNode("texture", impFile.Root());
		impFile.WriteString("resourceName", info.resourceName, node);
		impFile.WriteUint("textureFormat", info.textureFormat, node);
		impFile.WriteBool("genMips", info.genMips, node);
		impFile.WriteUint("genMipsFilter", info.genMipsFilter, node);
	}

	if(info.importBytes & IMP_BYTE_MESH)
	{
		auto node = impFile.CreateNode("mesh", impFile.Root());
		impFile.WriteString("resourceName", info.resourceName, node);
		impFile.WriteBool("isSkinnedMesh", info.isSkinnedMesh, node);
	}

	if(info.importBytes & IMP_BYTE_SKELETON)
	{
		auto node = impFile.CreateNode("skeleton", impFile.Root());
		impFile.WriteString("resourceName", info.resourceName, node);
	}

	if(info.importBytes & IMP_BYTE_ANIMATION)
	{
		auto node = impFile.CreateNode("animation", impFile.Root());
		impFile.WriteString("resourceName", info.resourceName, node);
	}

	if(info.importBytes & IMP_BYTE_COLLISION)
	{
		auto node = impFile.CreateNode("collision", impFile.Root());
		impFile.WriteString("resourceName", info.resourceName, node);
	}
	
	if( !(status = impFile.Save()) )
		ERR("Cant write import file %s", impFileName.c_str() );
#endif
	return status;
}

// TODO: different res pathes
void ResourceProcessor::LoadImportInfo(string& resName, ImportInfo& info, uint32_t& date)
{
#ifdef _EDITOR
	string impFileName = resName + EXT_IMPORT;
	FileIO impFile(impFileName);

	auto general = impFile.Node("import", impFile.Root());
	if( impFile.ReadInt("version", general) != IMPORT_FILE_VERSION )
	{
		date = FileIO::GetDateModifRaw(resName);
		info.filePath = resName;
		info.resourceName = RemoveExtension(resName);
		return;
	}

	date = impFile.ReadUint("fileDate", general);
	info.filePath = impFile.ReadString("filePath", general);

	auto textureNode = impFile.Node("texture", impFile.Root());
	if(textureNode)
	{
		info.importBytes |= IMP_BYTE_TEXTURE;
		info.resourceName = impFile.ReadString("resourceName", textureNode);
		info.textureFormat = (DXGI_FORMAT)impFile.ReadUint("textureFormat", textureNode);
		info.genMips = impFile.ReadBool("genMips", textureNode);
		info.genMipsFilter = impFile.ReadUint("genMipsFilter", textureNode);
	}

	auto meshNode = impFile.Node("mesh", impFile.Root());
	if(meshNode)
	{
		info.importBytes |= IMP_BYTE_MESH;
		info.resourceName = impFile.ReadString("resourceName", meshNode);
		info.isSkinnedMesh = impFile.ReadBool("isSkinnedMesh", meshNode);
	}

	auto skeletonNode = impFile.Node("skeleton", impFile.Root());
	if(skeletonNode)
	{
		info.importBytes |= IMP_BYTE_SKELETON;
		info.resourceName = impFile.ReadString("resourceName", skeletonNode);
	}

	auto animationNode = impFile.Node("animation", impFile.Root());
	if(animationNode)
	{
		info.importBytes |= IMP_BYTE_ANIMATION;
		info.resourceName = impFile.ReadString("resourceName", animationNode);
	}

	auto collisionNode = impFile.Node("collision", impFile.Root());
	if(collisionNode)
	{
		info.importBytes |= IMP_BYTE_COLLISION;
		info.resourceName = impFile.ReadString("resourceName", collisionNode);	
	}	

#endif
}

bool ResourceProcessor::CheckImportNeeded(ImportInfo& info, uint32_t date, string& resFile)
{
#ifdef _EDITOR
	if(!FileIO::IsExist(resFile))
		return true;

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
		skeletonMgr->GetResource(filename, reload);
		break;
	case EngineCore::ANIMATION:
		animationMgr->GetResource(filename, reload);
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
		materialMgr->GetMaterial(filename);
		break;
	}
}

void ResourceProcessor::Drop(string& filename, ResourceType type)
{
	switch(type)
	{
	case EngineCore::TEXTURE:
		texMgr->DeleteResourceByName(filename);
		break;
	case EngineCore::MESH:
		meshMgr->DeleteResourceByName(filename);
		break;
	case EngineCore::COLLISION:
		collisionMgr->DeleteResourceByName(filename);
		break;
	case EngineCore::SKELETON:
		skeletonMgr->DeleteResourceByName(filename);
		break;
	case EngineCore::ANIMATION:
		animationMgr->DeleteResourceByName(filename);
		break;
	case EngineCore::SHADER:
		shaderMgr->DeleteResourceByName(filename);
		break;
	case EngineCore::GUI_SHADER:
		shaderMgr->DeleteResourceByName(filename);
		break;
	case EngineCore::COMPUTE: // TODO
		{
			ERR("Compute shaders is unloadable for now! Name: path@entry");
		}
		break;
	case EngineCore::FONT:
		fontMgr->DeleteFont(filename);
		break;
	case EngineCore::MATERIAL:
		materialMgr->DeleteMaterial(filename);
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
			LUA_CALL((*luaRef)(info.resourceName, status, (*luaRefData)),);
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
			LUA_CALL((*luaRef)(id, status),);
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

void DropResource(string filename, uint32_t type)
{
	ResourceProcessor::Get()->Drop(filename, ResourceType(type));
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
		.addFunction("DropResource", &DropResource)

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