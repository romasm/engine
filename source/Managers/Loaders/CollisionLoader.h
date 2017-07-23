#pragma once
#include "stdafx.h"
#include "Common.h"
#include "reactphysics3d.h"
#include "MeshLoader.h"

namespace EngineCore
{
#define COLLISION_FILE_VERSION 101

	struct ConvexHull
	{
		uint32_t boneId;
		rp3d::CollisionShape* collider;
		Vector3 pos;
		Quaternion rot;

	#ifdef _DEV
		GPUMeshBuffer vertex;
		GPUMeshBuffer index;
	#endif

		ConvexHull() : boneId(0), collider(nullptr) {}
	};

	struct CollisionData
	{
		RArray<ConvexHull> hulls;

		CollisionData() {}
		~CollisionData()
		{
			for(uint32_t i = 0; i < (uint32_t)hulls.size(); i++)
			{
				_DELETE(hulls[i].collider);
				hulls[i].boneId = 0;
				hulls[i].pos = Vector3::Zero;
				hulls[i].rot = Quaternion::Identity;

			#ifdef _DEV
				hulls[i].vertex.size = 0;
				hulls[i].index.size = 0;
				_RELEASE(hulls[i].vertex.buffer);
				_RELEASE(hulls[i].index.buffer);
			#endif
			}
			hulls.destroy();
		}
	};

	struct CollisionHeader
	{
		uint32_t version;
		uint32_t hullsCount;
	};

	namespace CollisionLoader
	{		
		CollisionData* LoadCollisionFromMemory(string& resName, uint8_t* data, uint32_t size);
		CollisionData* LoadCollisionFromFile(string& filename);
		
		void ConvertCollisionToEngineFormat(string& filename);
		bool IsSupported(string filename);

		CollisionData* loadEngineCollisionFromMemory(string& filename, uint8_t* data, uint32_t size);
		CollisionData* loadNoNativeCollisionFromMemory(string& filename, uint8_t* data, uint32_t size, bool onlyConvert = false);

		CollisionData* loadAIScene(string& filename, const aiScene* scene, bool convert, bool noInit);

		void saveCollision(string& filename, CollisionData* collision, uint32_t** indices, uint32_t* trisCount, Vector3** vertices, uint32_t* verticesCount);
	};
}