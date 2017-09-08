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

	namespace CollisionLoader
	{		
		btCollisionShape* LoadCollisionFromMemory(string& resName, uint8_t* data, uint32_t size);
		btCollisionShape* LoadCollisionFromFile(string& filename);
		
		void ConvertCollisionToEngineFormat(string& filename);
		bool IsSupported(string filename);

		btCollisionShape* loadEngineCollisionFromMemory(string& filename, uint8_t* data, uint32_t size);
		btCollisionShape* loadNoNativeCollisionFromMemory(string& filename, uint8_t* data, uint32_t size, bool onlyConvert = false);

		btCollisionShape* loadAIScene(string& filename, const aiScene* scene, bool convert);

		void saveCollision(string& filename, btCollisionShape* collision);
	};
}