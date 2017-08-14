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

	class CollisionMgr : public BaseMgr<btCompoundShape, RESOURCE_MAX_COUNT>
	{
	public:
		CollisionMgr() : BaseMgr<btCompoundShape, RESOURCE_MAX_COUNT>()
		{
			null_resource = new btCompoundShape();
			btTransform identity;
			identity.setIdentity();
			null_resource->addChildShape(identity, new btBoxShape( btVector3(0.5f, 0.5f, 0.5f) ));

			resType = ResourceType::COLLISION;	
		}

		~CollisionMgr()
		{
			auto shape = null_resource->getChildShape(0);
			_DELETE(shape);
		}
		
		inline static CollisionMgr* Get(){return (CollisionMgr*)BaseMgr<btCompoundShape, RESOURCE_MAX_COUNT>::Get();}
	};
}