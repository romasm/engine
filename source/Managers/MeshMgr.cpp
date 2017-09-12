#include "stdafx.h"
#include "MeshMgr.h"
#include "Common.h"
#include "Render.h"
#include "Buffer.h"
#include "Material.h"
#include "WorldMgr.h"

using namespace EngineCore;

MeshMgr::MeshMgr() : BaseMgr<MeshData, RESOURCE_MAX_COUNT>()
{
	null_resource = MeshLoader::LoadMesh(string(PATH_STMESH_NULL));
	resType = ResourceType::MESH;
	resExt = EXT_MESH;

#ifdef _EDITOR
	mesh_reloaded.resize(RESOURCE_MAX_COUNT);
	mesh_reloaded.assign(0);
	something_reloaded = false;
#endif		
}

void MeshMgr::OnLoad(uint32_t id, MeshData* data, ImportInfo& info, uint32_t& date)
{
	BaseMgr<MeshData, RESOURCE_MAX_COUNT>::OnLoad(id, data, info, date);

#ifdef _EDITOR
	something_reloaded = true;
	mesh_reloaded[id] = resource_array[id].refcount;
#endif
}