#pragma once
#include "stdafx.h"
#include "Common.h"
#include "BaseMgr.h"
#include "MeshLoader.h"

namespace EngineCore
{
	enum TriClipping
	{
		TC_BOTH = 0,
		TC_FRONT,
		TC_BACK
	};
	
	class SkeletonMgr : public BaseMgr<SkeletonData, RESOURCE_MAX_COUNT>
	{
	public:
		SkeletonMgr() : BaseMgr<SkeletonData, RESOURCE_MAX_COUNT>()
		{
			null_resource = MeshLoader::LoadSkeleton(string(PATH_SKELETON_NULL));
			resType = ResourceType::SKELETON;
			resExt = EXT_SKELETON;
		}
		inline static SkeletonMgr* Get() { return (SkeletonMgr*)BaseMgr<SkeletonData, RESOURCE_MAX_COUNT>::Get(); }
	};

	class AnimationMgr : public BaseMgr<AnimationData, RESOURCE_MAX_COUNT>
	{
	public:
		AnimationMgr() : BaseMgr<AnimationData, RESOURCE_MAX_COUNT>()
		{
			null_resource = MeshLoader::LoadAnimation(string(PATH_ANIMATION_NULL));
			resType = ResourceType::ANIMATION;
			resExt = EXT_ANIMATION;
		}
		inline static AnimationMgr* Get() { return (AnimationMgr*)BaseMgr<AnimationData, RESOURCE_MAX_COUNT>::Get(); }
	};

	class MeshMgr : public BaseMgr<MeshData, RESOURCE_MAX_COUNT>
	{
	public:
		MeshMgr();

		void OnLoad(uint32_t id, MeshData* data, ImportInfo& info, uint32_t& date);

		inline static MeshMgr* Get() { return (MeshMgr*)BaseMgr<MeshData, RESOURCE_MAX_COUNT>::Get(); }

	#ifdef _EDITOR
		inline bool IsJustReloaded(uint32_t id) 
		{
			if(id == nullres)
				return false;
			bool res = mesh_reloaded[id] > 0;
			if(res)
				mesh_reloaded[id]--;
			return res;
		}

		inline bool IsBBoxesDirty()
		{
			bool res = something_reloaded;
			something_reloaded = false;
			return res;
		}

		static bool MeshBoxOverlap(uint32_t meshID, const Matrix& transform, const BoundingBox& bbox);
		static float MeshRayIntersect(uint32_t meshID, const Matrix& transform, const Vector3& origin, const Vector3& dirNormal, float maxDist, TriClipping triClipping, bool& isFront);
	
		#define VOXELIZE_TRI(XX,YY,ZZ)\
			{int32_t minX = min(triVerteciesInt[0].XX, min(triVerteciesInt[1].XX, triVerteciesInt[2].XX));\
			int32_t minY = min(triVerteciesInt[0].YY, min(triVerteciesInt[1].YY, triVerteciesInt[2].YY));\
			int32_t maxX = max(triVerteciesInt[0].XX, max(triVerteciesInt[1].XX, triVerteciesInt[2].XX));\
			int32_t maxY = max(triVerteciesInt[0].YY, max(triVerteciesInt[1].YY, triVerteciesInt[2].YY));\
			for (int32_t XX = max(0, minX); XX < min((int32_t)voxels.resolution, maxX + 1); XX++)\
			for (int32_t YY = max(0, minY); YY < min((int32_t)voxels.resolution, maxY + 1); YY++){\
				Vector3 vs0(float(triVerteciesInt[2].XX - triVerteciesInt[0].XX), float(triVerteciesInt[1].XX - triVerteciesInt[0].XX), float(triVerteciesInt[0].XX - XX));\
				Vector3 vs1(float(triVerteciesInt[2].YY - triVerteciesInt[0].YY), float(triVerteciesInt[1].YY - triVerteciesInt[0].YY), float(triVerteciesInt[0].YY - YY));\
				Vector3 u = vs0.Cross(vs1);\
				if (abs(u.z) < 1.0f) continue;\
				Vector3 barycentric(1.0f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);\
				if (barycentric.x < 0 || barycentric.y < 0 || barycentric.z < 0) continue;\
				float depth = triVerteciesInt[0].ZZ * barycentric.x + triVerteciesInt[1].ZZ * barycentric.y + triVerteciesInt[2].ZZ * barycentric.z;\
				int32_t ZZ = int32_t(depth);\
				if (ZZ < 0 || ZZ >= (int32_t)voxels.resolution) continue;\
				voxels.voxels[x][y][z] |= voxelValue; voxels.empty = false;}}\

		template<class voxelGrid>
		static void MeshVoxelize(uint32_t meshID, const Matrix& transform, const BoundingBox& bbox, voxelGrid& voxels)
		{
			auto mesh = MeshMgr::GetResourcePtr(meshID);
			if (!mesh)
				return;

			Vector3 bboxCorner = bbox.Center - bbox.Extents;
			Vector3 bboxSizeInv = Vector3(1.0f) / (2.0f * bbox.Extents);

			Vector3 triVertecies[3];
			Vector3Int32 triVerteciesInt[3];
			for (int32_t i = 0; i < (int32_t)mesh->indices.size(); i++)
			{
				uint8_t* verts = mesh->vertices[i];
				uint32_t* inds = mesh->indices[i];

				for (uint32_t k = 0; k < (uint32_t)mesh->indexBuffers[i].count; k += 3)
				{
					triVertecies[0] = MeshLoader::GetVertexPos(verts, inds[k], mesh->vertexFormat);
					triVertecies[1] = MeshLoader::GetVertexPos(verts, inds[k + 1], mesh->vertexFormat);
					triVertecies[2] = MeshLoader::GetVertexPos(verts, inds[k + 2], mesh->vertexFormat);

					for (int32_t h = 0; h < 3; h++)
						Vector3::Transform(triVertecies[h], transform, triVertecies[h]);

					if (!TriBoxOverlap(bbox, triVertecies))
						continue;

					Vector3 triVect0 = triVertecies[1] - triVertecies[0];
					Vector3 triVect1 = triVertecies[2] - triVertecies[0];
					Vector3 normal = triVect0.Cross(triVect1);

					uint8_t voxelValue = 0;
					if (normal.x != 0)
						voxelValue |= normal.x > 0 ? 0b00100000 : 0b00010000;
					if (normal.y != 0)
						voxelValue |= normal.y > 0 ? 0b00001000 : 0b00000100;
					if (normal.z != 0)
						voxelValue |= normal.z > 0 ? 0b00000010 : 0b00000001;

					for (int32_t h = 0; h < 3; h++)
						triVerteciesInt[h] = Vector3Int32((float)voxels.resolution * (triVertecies[h] - bboxCorner) * bboxSizeInv);

					float maxNormal = max(normal.x, max(normal.y, normal.z));
					if (maxNormal == normal.z)
						VOXELIZE_TRI(x, y, z)
					else if (maxNormal == normal.y)
						VOXELIZE_TRI(x, z, y)
					else
						VOXELIZE_TRI(y, z, x)

						/*int32_t minX = min(triVerteciesInt[0].x, min(triVerteciesInt[1].x, triVerteciesInt[2].x));
						int32_t minY = min(triVerteciesInt[0].y, min(triVerteciesInt[1].y, triVerteciesInt[2].y));
						int32_t maxX = max(triVerteciesInt[0].x, max(triVerteciesInt[1].x, triVerteciesInt[2].x));
						int32_t maxY = max(triVerteciesInt[0].y, max(triVerteciesInt[1].y, triVerteciesInt[2].y));

						for (int32_t x = max(0, minX); x < min((int32_t)voxels.resolution, maxX + 1); x++)
						{
						for (int32_t y = max(0, minY); y < min((int32_t)voxels.resolution, maxY + 1); y++)
						{
						Vector3 vs0(float(triVerteciesInt[2].x - triVerteciesInt[0].x), float(triVerteciesInt[1].x - triVerteciesInt[0].x), float(triVerteciesInt[0].x - x));
						Vector3 vs1(float(triVerteciesInt[2].y - triVerteciesInt[0].y), float(triVerteciesInt[1].y - triVerteciesInt[0].y), float(triVerteciesInt[0].y - y));

						Vector3 u = vs0.Cross(vs1);
						if (abs(u.z) < 1.0f)
						continue;

						Vector3 barycentric(1.0f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
						if (barycentric.x < 0 || barycentric.y < 0 || barycentric.z < 0)
						continue;

						float depth = triVerteciesInt[0].z * barycentric.x + triVerteciesInt[1].z * barycentric.y + triVerteciesInt[2].z * barycentric.z;

						int32_t z = int32_t(normal.z > 0 ? ceilf(depth) : floor(depth));
						if (z < 0 || z >= voxels.resolution)
						continue;

						voxels.voxels[x][y][z] |= voxelValue;
						}
						}*/
				}
			}
		}

	private:

		SArray<uint32_t, RESOURCE_MAX_COUNT> mesh_reloaded;
		bool something_reloaded;
#endif
	};
}