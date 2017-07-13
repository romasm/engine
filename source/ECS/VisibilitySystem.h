#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "..\Frustum.h"

namespace EngineCore
{
	struct VisibilityComponent
	{
		ENTITY_IN_COMPONENT

		bool dirty;
		bitset<FRUSTUM_MAX_COUNT> inFrust;

		// static data
		BoundingBox localBox;

		// update on transform
		BoundingOrientedBox worldBox;
	};

	class BaseWorld;

	class VisibilitySystem
	{
	public:
		VisibilitySystem(BaseWorld* w, uint32_t maxCount);

		VisibilityComponent* AddComponent(Entity e)
		{
			VisibilityComponent* res = components.add(e.index());
			res->dirty = true;
			res->parent = e;
			res->inFrust = 0;
			return res;
		}
		void AddComponent(Entity e, VisibilityComponent D)
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

		inline VisibilityComponent* GetComponent(Entity e)
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

		void CheckVisibility();

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool SetBBox(Entity e, BoundingBox box);
		bool AddToBBox(Entity e, BoundingBox box);
		bool AddToBBox(Entity e, BoundingSphere sphere);
		BoundingBox GetBBoxL(Entity e);
		BoundingOrientedBox GetBBoxW(Entity e);

		Entity CollideRay(Vector3 origin, Vector3 ray, int frust_id);
		Vector4 CollideRayCoords(Vector3 origin, Vector3 ray, int frust_id);

		Vector3 GetBoxSizeW(Entity e)
		{
			BoundingOrientedBox box = GetBBoxW(e);
			return VECTOR3_CAST(box.Extents);
		}
		Vector3 GetBoxSizeL(Entity e)
		{
			BoundingBox box = GetBBoxL(e);
			return VECTOR3_CAST(box.Extents);
		}
		Vector3 GetBoxCenterW(Entity e)
		{
			BoundingOrientedBox box = GetBBoxW(e);
			return VECTOR3_CAST(box.Center);
		}
		Vector3 GetBoxCenterL(Entity e)
		{
			BoundingBox box = GetBBoxL(e);
			return VECTOR3_CAST(box.Center);
		}
		
		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<VisibilitySystem>("VisibilitySystem")
					.addFunction("CollideRay", &VisibilitySystem::CollideRay)
					.addFunction("CollideRayCoords", &VisibilitySystem::CollideRayCoords)
					.addFunction("GetBoxSizeL", &VisibilitySystem::GetBoxSizeL)
					.addFunction("GetBoxCenterL", &VisibilitySystem::GetBoxCenterL)
					.addFunction("GetBoxSizeW", &VisibilitySystem::GetBoxSizeW)
					.addFunction("GetBoxCenterW", &VisibilitySystem::GetBoxCenterW)

					.addFunction("AddComponent", &VisibilitySystem::_AddComponent)
					.addFunction("DeleteComponent", &VisibilitySystem::DeleteComponent)
					.addFunction("HasComponent", &VisibilitySystem::HasComponent)
				.endClass();
		}

	private:
		void collide_ray(Vector3 origin, Vector3 ray, int frust_id, Vector4* colide_coord, Entity* ent);

		ComponentRArray<VisibilityComponent> components;

		TransformSystem* transformSys;
		FrustumMgr* frustumMgr;
		BaseWorld* world;
	};
}