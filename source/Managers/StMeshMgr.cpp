#include "stdafx.h"
#include "StMeshMgr.h"
#include "Common.h"
#include "Render.h"
#include "Buffer.h"
#include "Material.h"
#include "WorldMgr.h"
#include "MeshLoader.h"

using namespace EngineCore;

StMeshMgr* StMeshMgr::instance = nullptr;
MeshData* StMeshMgr::null_mesh = nullptr;
string StMeshMgr::null_name = "";

StMeshMgr::StMeshMgr()
{
	if(!instance)
	{
		instance = this;

		mesh_array.resize(STMESH_MAX_COUNT);
		mesh_free.resize(STMESH_MAX_COUNT);
		for(uint32_t i=0; i<STMESH_MAX_COUNT; i++)
			mesh_free[i] = i;
		mesh_map.reserve(STMESH_INIT_COUNT);
		mesh_reloaded.resize(STMESH_MAX_COUNT);
		mesh_reloaded.assign(0);
		
		null_mesh = MeshLoader::LoadStaticMeshFromFile(string(PATH_STMESH_NULL));
	}
	else
		ERR("Only one instance of StMeshMgr is allowed!");
}

StMeshMgr::~StMeshMgr()
{
	for(uint32_t i=0; i<STMESH_MAX_COUNT; i++)
	{
		_CLOSE(mesh_array[i].mesh);
		mesh_array[i].name.clear();
	}
	_CLOSE(null_mesh);
	null_name.clear();

	instance = nullptr;
}

void StMeshMgr::PreloadStMeshes()
{
	bool reload = false;
#ifdef _DEV
	reload = true;
#endif

	GetStMesh(string(ENV_MESH), reload);
	GetStMesh(string(MAT_MESH), reload);
}

uint32_t StMeshMgr::GetStMesh(string& name, bool reload)
{
	uint32_t res = STMESH_NULL;
	if(name.length() == 0)
		return res;

	res = FindStMeshInList(name);
	if(res != STMESH_NULL)
		return res;

	res = AddStMeshToList(name, reload);
	if(res != STMESH_NULL)
		return res;

	ERR("Cant load static mesh %s", name.c_str());

	return res;
}

uint32_t StMeshMgr::AddStMeshToList(string& name, bool reload)
{
	if(mesh_free.size() == 0)
	{
		ERR("Static meshes resources amount overflow!");
		return STMESH_NULL;
	}

	uint32_t idx = mesh_free.front();
	auto& handle = mesh_array[idx];
	
	if(!FileIO::IsExist(name))
		return STMESH_NULL;

	handle.mesh = MeshLoader::LoadStaticMeshFromFile( name ); // TODO: loading in background
	if( !handle.mesh || handle.mesh->vertexBuffers.empty() )
	{
		_CLOSE(handle.mesh);
		handle.mesh = null_mesh;
	}
	
	handle.name = name;
	handle.refcount = 1;

	if(reload)
		handle.filedate = FileIO::GetDateModifRaw(name);
	else
		handle.filedate = ReloadingType::RELOAD_NONE;

	mesh_map.insert(make_pair(name, idx));
	mesh_free.pop_front();

	return idx;
}

uint32_t StMeshMgr::FindStMeshInList(string& name)
{
	auto it = mesh_map.find(name);
	if(it == mesh_map.end())
		return STMESH_NULL;

	auto& handle = mesh_array[it->second];
	handle.refcount++;
	return it->second;
}

void StMeshMgr::DeleteStMesh(uint32_t id)
{
	if(id == STMESH_NULL)
		return;
	
	auto& handle = mesh_array[id];

	if(handle.refcount == 1)
	{
		if(handle.mesh != null_mesh)
		{
			_CLOSE(handle.mesh);
			LOG("Static mesh droped %s", handle.name.c_str());
		}
		
		handle.refcount = 0;
		handle.filedate = 0;

		mesh_free.push_back(id);

		mesh_map.erase(handle.name);

		handle.name.clear();
	}
	else if(handle.refcount == 0)
	{
		WRN("Static mesh %s has already deleted!", handle.name.c_str());
	}
	else
		handle.refcount--;
}

void StMeshMgr::DeleteStMeshByName(string& name)
{
	if(name.length() == 0)
		return;
	
	auto it = mesh_map.find(name);
	if(it == mesh_map.end())
		return;

	auto& handle = mesh_array[it->second];

	if(handle.refcount == 1)
	{
		if(handle.mesh != null_mesh)
		{
			_CLOSE(handle.mesh);
			LOG("Static mesh droped %s", handle.name.c_str());
		}
		
		handle.refcount = 0;
		handle.filedate = 0;

		mesh_free.push_back(it->second);

		mesh_map.erase(name);

		handle.name.clear();
	}
	else
		handle.refcount--;
}

void StMeshMgr::UpdateStMeshes()
{
	bool something_reloaded = false;
	auto it = mesh_map.begin();
	while(it != mesh_map.end())
	{
		auto& handle = mesh_array[it->second];

		if(handle.filedate == ReloadingType::RELOAD_NONE)
		{
			it++;
			continue;
		}

		if( handle.filedate == ReloadingType::RELOAD_ONCE )
			handle.filedate = ReloadingType::RELOAD_NONE;
		else
		{
			uint32_t last_date = FileIO::GetDateModifRaw(handle.name);
			if(last_date == handle.filedate || handle.filedate == ReloadingType::RELOAD_NONE)
			{
				it++;
				continue;
			}
			handle.filedate = last_date;
		}

		ResourceProcessor::Get()->QueueLoad(it->second, ResourceType::MESH);
		
		something_reloaded = true;
		mesh_reloaded[it->second] = handle.refcount;
		it++;
	}

	if(something_reloaded) // TODO: valid bbox update
		WorldMgr::Get()->PostStMeshesReload();
}