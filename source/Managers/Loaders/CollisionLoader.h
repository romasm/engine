#pragma once
#include "stdafx.h"
#include "Common.h"
#include "MeshLoader.h"

namespace EngineCore
{
#define COLLISION_FILE_VERSION 103
	
	struct CollisionHeader
	{
		uint32_t version;
		uint32_t type;
	};

	struct CollisionData
	{
#ifdef _EDITOR
		uint32_t sourceDate;
		ImportInfo importInfo;
#endif

		btCollisionShape* data;

		CollisionData() : data(nullptr) {}

		// multilayer compound collisions is NOT supported
		~CollisionData()
		{
			CollisionLoader::CollisionDeallocate(data);
		}
	};

	namespace CollisionLoader
	{		
		void CollisionDeallocate(btCollisionShape*& resource);

		CollisionData* LoadCollision(string& resName);
		
		void ConvertCollisionToEngineFormat(string& filename);
		bool IsSupported(string filename);

		CollisionData* loadEngineCollisionFromMemory(string& filename, uint8_t* data, uint32_t size);
		CollisionData* loadNoNativeCollisionFromMemory(string& filename, uint8_t* data, uint32_t size, bool onlyConvert = false);

		CollisionData* loadAIScene(string& filename, const aiScene* scene, bool convert);

		void saveCollision(string& filename, CollisionData* collision);
	};
}