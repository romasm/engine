#pragma once
#include "stdafx.h"
#include "Common.h"
#include "BaseMgr.h"
#include "CollisionLoader.h"

namespace EngineCore
{
	enum CollisionStorageType
	{
		LOCAL = 0,
		RESOURCE,
	};

	class CollisionMgr : public BaseMgr<CollisionData, RESOURCE_MAX_COUNT>
	{
	public:
		CollisionMgr() : BaseMgr<CollisionData, RESOURCE_MAX_COUNT>()
		{
			null_resource = new CollisionData();
			null_resource->shape = new btBoxShape( btVector3(0.5f, 0.5f, 0.5f) );
			resType = ResourceType::COLLISION;	
		}
		
		inline static CollisionMgr* Get(){return (CollisionMgr*)BaseMgr<CollisionData, RESOURCE_MAX_COUNT>::Get();}

	};
}