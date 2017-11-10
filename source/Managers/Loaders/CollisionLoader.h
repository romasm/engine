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
		btCollisionShape* LoadCollision(string& resName);
		btCollisionShape* loadEngineCollisionFromMemory(string& filename, uint8_t* data, uint32_t size);
		btCollisionShape* copyCollision(btCollisionShape* original);
				
		bool ConvertCollisionToEngineFormat(string& sourceFile, string& resFile);
		bool IsNative(string filename);
		bool IsSupported(string filename);
		bool SaveCollision(string& filename, btCollisionShape* collision);

		btCollisionShape* convertAIScene(string& sourceFile, string& resFile, const aiScene* scene);
	};
}