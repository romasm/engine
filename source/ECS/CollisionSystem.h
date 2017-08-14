#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "PhysicsSystem.h"
#include "CollisionMgr.h"

#define COLLISION_RESOURCE_NULL 0

namespace EngineCore
{
	struct CollisionComponent
	{
		ENTITY_IN_COMPONENT
		
		bool dirty;
		
		//TODO

		CollisionComponent()
		{
			dirty = false;
		}
	};

	class BaseWorld;

	class CollisionSystem
	{
	public:
		CollisionSystem(BaseWorld* w, btDiscreteDynamicsWorld* collisionWorld, uint32_t maxCount);
		~CollisionSystem();

		CollisionComponent* AddComponent(Entity e);

		void CopyComponent(Entity src, Entity dest);
		void DeleteComponent(Entity e);
		bool HasComponent(Entity e) const { return components.has(e.index()); }
		size_t ComponentsCount() {return components.dataSize();}

		inline CollisionComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		void UpdateTransformations();

	#ifdef _DEV
		void DebugRegToDraw();
	#endif

		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
				
		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool IsActive(Entity e);
		void SetActive(Entity e, bool active);
		
		inline void _AddComponent(Entity e) {AddComponent(e);}

		void SetDebugDraw(bool draw)
		{
			b_debugDraw = draw;
		}

		static void RegLuaClass();

	private:

		inline void _DeleteComponent(CollisionComponent* comp)
		{
			// TODO
		}

		ComponentRArray<CollisionComponent> components;
				
		BaseWorld* world;
		TransformSystem* transformSystem;
		PhysicsSystem* physicsSystem;

		btDiscreteDynamicsWorld* collisionWorld;

		bool b_debugDraw;
	};
}