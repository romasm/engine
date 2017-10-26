#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "CollisionSystem.h"
#include "CollisionMgr.h"
#include "TypeMgr.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace EngineCore
{
	enum TriggerFilterType
	{
		FilterNone = 0,
		FilterByType = 1,
		FilterByTypeInv = 2,
		FilterByName = 3,
		FilterByNameInv = 4,
		FilterByNamePart = 5,
		FilterByNamePartInv = 6,
	};

	struct OverlappedEntity
	{
		float time;
		uint32_t frameID; 

		OverlappedEntity() : time(0.0f), frameID(0) {}
		OverlappedEntity(float t, uint32_t f) : time(t), frameID(f) {}
	};

	struct TriggerComponent
	{
		ENTITY_IN_COMPONENT
		
		bool dirty;
		bool active;

		btGhostObject* object;
		TriggerFilterType filter; 
		string filterString; 
		float reactionDelay;
		
		unordered_map<uint32_t, OverlappedEntity> overlappingMap;

		LuaRef* startTouch;
		LuaRef* endTouch;
		LuaRef* endTouchAll;

		TriggerComponent()
		{
			dirty = false;
			active = false;
			object = nullptr;
			filter = TriggerFilterType::FilterNone;
			startTouch = nullptr;
			endTouch = nullptr;
			endTouchAll = nullptr;
			reactionDelay = 0;
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
		
		void CheckOverlaps(float dt, uint32_t frameID);
		void UpdateTransformations();

		void UpdateState(Entity e);
		
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

		float GetDelay(Entity e);
		void SetDelay(Entity e, float d);

		void SetFuncStartTouch(Entity e, LuaRef func);
		void SetFuncEndTouch(Entity e, LuaRef func);
		void SetFuncEndTouchAll(Entity e, LuaRef func);

		float GetTouchingTime(Entity e, Entity touching);
		inline bool IsTouching(Entity e, Entity touching) {return GetTouchingTime(e, touching) != 0;}

		inline void _AddComponent(Entity e) {AddComponent(e);}
		
		static void RegLuaClass();

	private:
		bool FilterEntity(TriggerComponent& comp, Entity ent);
		void _DeleteComponent(TriggerComponent* comp);

		ComponentRArray<TriggerComponent> components;
				
		BaseWorld* world;
		TransformSystem* transformSystem;
		CollisionSystem* collisionSystem;
		TypeMgr* typeMgr;
		NameMgr* nameMgr;

		btDiscreteDynamicsWorld* dynamicsWorld;
	};
}