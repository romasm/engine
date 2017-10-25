#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "CollisionSystem.h"
#include "CollisionMgr.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace EngineCore
{
	enum TriggerFilterType
	{
		FilterNone = 0,
		FilterByType = 1,
		FilterByName = 2,
		FilterByNamePart = 3,
	};

	struct TriggerComponent
	{
		ENTITY_IN_COMPONENT
		
		bool dirty;
		bool active;

		btGhostObject* object;
		TriggerFilterType filter; 
		string filterString; 
		
		unordered_map<Entity, btCollisionObject*> overlappingMap;

		TriggerComponent()
		{
			dirty = false;
			active = false;
			object = nullptr;
			filter = TriggerFilterType::FilterNone;
		}
	};

	class BaseWorld;

	class TriggerSystem
	{
	public:
		TriggerSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount);
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
		
		void CheckOverlaps();
		void UpdateTransformations();
		
		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
				
		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		int32_t GetFilterType(Entity e);
		void SetFilterType(Entity e, int32_t type);

		string GetFilterString(Entity e);
		void SetFilterString(Entity e, string str);

		bool IsActive(Entity e);
		void SetActive(Entity e, bool active);
		
		inline void _AddComponent(Entity e) {AddComponent(e);}
		
		static void RegLuaClass();

	private:

		void _DeleteComponent(TriggerComponent* comp);

		ComponentRArray<TriggerComponent> components;
				
		BaseWorld* world;
		TransformSystem* transformSystem;
		CollisionSystem* collisionSystem;

		btDiscreteDynamicsWorld* dynamicsWorld;
	};
}