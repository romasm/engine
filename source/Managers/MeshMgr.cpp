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

#ifdef _EDITOR
bool MeshMgr::MeshBoxOverlap(uint32_t meshID, const Matrix& transform, const BoundingBox& bbox)
{
	auto mesh = MeshMgr::GetResourcePtr(meshID);
	if (!mesh)
		return false;

	Vector3 triVertecies[3];
	for (int32_t i = 0; i < (int32_t)mesh->indices.size(); i++)
	{
		uint8_t* verts = mesh->vertices[i];
		uint32_t* inds = mesh->indices[i];

		for (uint32_t k = 0; k < (uint32_t)mesh->indexBuffers[i].count; k += 3)
		{
			triVertecies[0] = MeshLoader::GetVertexPos(verts, inds[k], mesh->vertexFormat);
			triVertecies[1] = MeshLoader::GetVertexPos(verts, inds[k + 1], mesh->vertexFormat);
			triVertecies[2] = MeshLoader::GetVertexPos(verts, inds[k + 2], mesh->vertexFormat);

			Vector3::Transform(triVertecies[0], transform, triVertecies[0]);
			Vector3::Transform(triVertecies[1], transform, triVertecies[1]);
			Vector3::Transform(triVertecies[2], transform, triVertecies[2]);

			if (TriBoxOverlap(bbox, triVertecies))
				return true;
		}
	}
	return false;
}

float MeshMgr::MeshRayIntersect(uint32_t meshID, const Matrix& transform, const Vector3& origin, const Vector3& dirNormal, float maxDist, TriClipping triClipping, bool& isFront)
{
	auto mesh = MeshMgr::GetResourcePtr(meshID);
	if (!mesh)
		return 0.0f;

	float minDist = 99999990000.0f;
	bool isIntersect = false;

	Vector3 triVertecies[3];
	for (int32_t i = 0; i < (int32_t)mesh->indices.size(); i++)
	{
		uint8_t* verts = mesh->vertices[i];
		uint32_t* inds = mesh->indices[i];

		for (uint32_t k = 0; k < (uint32_t)mesh->indexBuffers[i].count; k += 3)
		{
			triVertecies[0] = MeshLoader::GetVertexPos(verts, inds[k], mesh->vertexFormat);
			triVertecies[1] = MeshLoader::GetVertexPos(verts, inds[k + 1], mesh->vertexFormat);
			triVertecies[2] = MeshLoader::GetVertexPos(verts, inds[k + 2], mesh->vertexFormat);

			Vector3::Transform(triVertecies[0], transform, triVertecies[0]);
			Vector3::Transform(triVertecies[1], transform, triVertecies[1]);
			Vector3::Transform(triVertecies[2], transform, triVertecies[2]);

			bool isFrontTri;
			float dist = TriRayIntersect(origin, dirNormal, triVertecies, isFrontTri);

			if (dist == 0.0f || dist >= maxDist)
				continue;

			if (triClipping == TriClipping::TC_BOTH ||
				(triClipping == TriClipping::TC_FRONT && isFrontTri) ||
				(triClipping == TriClipping::TC_BACK && !isFrontTri))
			{
				if (dist < minDist)
				{
					minDist = dist;
					isFront = isFrontTri;
				}
				isIntersect = true;
			}
		}
	}

	if (!isIntersect)
		return 0.0f;

	return minDist;
}
#endif