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
			null_resource = new CollisionData;
			null_resource->hulls.create(1);
			auto hull = null_resource->hulls.push_back();
			hull->collider = new rp3d::BoxShape(Vector3(0.5f, 0.5f, 0.5f));

			resType = ResourceType::COLLISION;	
		}
		
		inline static CollisionMgr* Get(){return (CollisionMgr*)BaseMgr<CollisionData, RESOURCE_MAX_COUNT>::Get();}

	};
}