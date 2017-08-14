#pragma once
#include "stdafx.h"
#include "Common.h"
#include "MeshLoader.h"

namespace EngineCore
{
#define COLLISION_FILE_VERSION 102
	
	struct CollisionHeader
	{
		uint32_t version;
	};

	namespace CollisionLoader
	{		
		btCompoundShape* LoadCollisionFromMemory(string& resName, uint8_t* data, uint32_t size);
		btCompoundShape* LoadCollisionFromFile(string& filename);
		
		void ConvertCollisionToEngineFormat(string& filename);
		bool IsSupported(string filename);

		btCompoundShape* loadEngineCollisionFromMemory(string& filename, uint8_t* data, uint32_t size);
		btCompoundShape* loadNoNativeCollisionFromMemory(string& filename, uint8_t* data, uint32_t size, bool onlyConvert = false);

		btCompoundShape* loadAIScene(string& filename, const aiScene* scene, bool convert, bool noInit);

		void saveCollision(string& filename, btCompoundShape* collision, uint32_t** indices, uint32_t* trisCount, Vector3** vertices, uint32_t* verticesCount);
	};
}