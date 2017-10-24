#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "CollisionSystem.h"
#include "CollisionMgr.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace EngineCore
{
	struct TriggerComponent
	{
		ENTITY_IN_COMPONENT
		
		bool dirty;
		
		btGhostObject* object;
		
		TriggerComponent()
		{
			dirty = false;
			object = nullptr;
		}
	};

	class BaseWorld;

	class TriggerSystem
	{
	public:
		TriggerSystem(BaseWorld* w, btDiscreteDynamicsWorld* collisionWorld, uint32_t maxCount);
		~TriggerSystem();

		TriggerComponent* AddComponent(Entity e);

		void CopyComponent(Entity src, Entity dest);
		void DeleteComponent(Entity e);
		bool HasComponent(Entity e) const { return components.has(e.index()); }
		size_t ComponentsCount() {return components.dataSize();}

		inline TriggerComponent* GetComponent(Entity e)
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

		inline void _DeleteComponent(TriggerComponent* comp)
		{
			// TODO
		}

		ComponentRArray<TriggerComponent> components;
				
		BaseWorld* world;
		TransformSystem* transformSystem;
		CollisionSystem* collisionSystem;

		btDiscreteDynamicsWorld* collisionWorld;

		bool b_debugDraw;
	};
}