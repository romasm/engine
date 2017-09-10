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

	class CollisionMgr : public BaseMgr<btCollisionShape, RESOURCE_MAX_COUNT>
	{
	public:
		CollisionMgr() : BaseMgr<btCollisionShape, RESOURCE_MAX_COUNT>()
		{
			null_resource = new btBoxShape( btVector3(0.5f, 0.5f, 0.5f) );
			resType = ResourceType::COLLISION;	
		}
		
		inline static CollisionMgr* Get(){return (CollisionMgr*)BaseMgr<btCollisionShape, RESOURCE_MAX_COUNT>::Get();}

		// multilayer compound collisions is NOT supported
		virtual void ResourceDeallocate(btCollisionShape*& resource)
		{
			if(!resource)
				return;

			if( resource->getShapeType() == COMPOUND_SHAPE_PROXYTYPE )
			{
				btCompoundShape* shape = (btCompoundShape*)resource;

				auto childrenCount = shape->getNumChildShapes();
				auto childrenPtrs = shape->getChildList();

				while( childrenCount > 0 )
				{
					auto child = childrenPtrs->m_childShape;
					shape->removeChildShapeByIndex(0);
					_DELETE(child);
					childrenCount--;
				}
			}

			_DELETE(resource);
		}
	};
}