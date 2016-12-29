#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "..\Frustum.h"

namespace EngineCore
{
	enum BoundingType
	{
		BT_BOX = 0,
		BT_SPHERE = 1,
		BT_FRUSTUM = 2,
		BT_FRUSTUM_SPHERE = 3
	};

	struct EarlyVisibilityComponent // separete shere ????
	{
		ENTITY_IN_COMPONENT

		bool dirty;
		bitset<FRUSTUM_MAX_COUNT> inFrust;

		// static data
		BoundingType type;

		BoundingBox localBox;
		BoundingSphere localSphere;
		BoundingFrustum localFrustum;

		// update on transform
		BoundingOrientedBox worldBox;
		BoundingSphere worldSphere;
		BoundingFrustum worldFrustum;
	};

	class BaseWorld;

	class EarlyVisibilitySystem
	{
	public:
		EarlyVisibilitySystem(BaseWorld* w, uint32_t maxCount);

		EarlyVisibilityComponent* AddComponent(Entity e)
		{
			EarlyVisibilityComponent* res = components.add(e.index());
			res->dirty = true;
			res->parent = e;
			res->inFrust = 0;
			res->type = BT_BOX;
			return res;
		}
		void AddComponent(Entity e, EarlyVisibilityComponent D)
		{
			D.dirty = true;
			D.parent = e;
			components.add(e.index(), D);
		}
		void CopyComponent(Entity src, Entity dest)
		{
			auto comp = GetComponent(src);
			if(!comp) return;
			AddComponent(dest, *comp);
		}
		void DeleteComponent(Entity e) {components.remove(e.index());}
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		inline EarlyVisibilityComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		inline uint32_t Serialize(Entity e, uint8_t* data) {return 0;}
		inline uint32_t Deserialize(Entity e, uint8_t* data)
		{
			AddComponent(e);
			return 0;
		}
		
		void CheckEarlyVisibility();

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		void SetType(Entity e, BoundingType type);

		bool SetBBox(Entity e, BoundingBox box);
		bool AddToBBox(Entity e, BoundingBox box);
		bool AddToBBox(Entity e, BoundingSphere sphere);
		BoundingBox GetBBoxL(Entity e);
		BoundingOrientedBox GetBBoxW(Entity e);

		bool SetBSphere(Entity e, BoundingSphere sphere);
		bool AddToBSphere(Entity e, BoundingBox box);
		bool AddToBSphere(Entity e, BoundingSphere sphere);
		BoundingSphere GetBSphereL(Entity e);
		BoundingSphere GetBSphereW(Entity e);

		bool SetBFrustum(Entity e, BoundingFrustum frust);
		BoundingFrustum GetBFrustumL(Entity e);
		BoundingFrustum GetBFrustumW(Entity e);

		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<EarlyVisibilitySystem>("EarlyVisibilitySystem")
					.addFunction("AddComponent", &EarlyVisibilitySystem::_AddComponent)
					.addFunction("DeleteComponent", &EarlyVisibilitySystem::DeleteComponent)
					.addFunction("HasComponent", &EarlyVisibilitySystem::HasComponent)
				.endClass();
		}

	private:
		ComponentRArray<EarlyVisibilityComponent> components;

		TransformSystem* transformSys;
		FrustumMgr* frustumMgr;
		BaseWorld* world;
	};
}