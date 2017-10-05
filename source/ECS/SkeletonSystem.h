#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "MeshMgr.h"

namespace EngineCore
{
	struct SkeletonComponent
	{
		ENTITY_IN_COMPONENT

		bool dirty;

		SkeletonComponent()
		{
			parent.setnull();
			dirty = true;

		}
	};

	class BaseWorld;

	class SkeletonSystem
	{
		friend BaseWorld;
	public:
		SkeletonSystem(BaseWorld* w, uint32_t maxCount);
		~SkeletonSystem();

		SkeletonComponent* AddComponent(Entity e);

		void CopyComponent(Entity src, Entity dest);
		void DeleteComponent(Entity e);

		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		SkeletonComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
		
		void RegToDraw();

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		inline bool _AddComponent(Entity e)
		{
			if(AddComponent(e))	return true;
			else return false;
		}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<SkeletonSystem>("SkeletonSystem")
					.addFunction("AddComponent", &SkeletonSystem::_AddComponent)
					.addFunction("DeleteComponent", &SkeletonSystem::DeleteComponent)
					.addFunction("HasComponent", &SkeletonSystem::HasComponent)
					
				.endClass();
		}

	private:
		ComponentRArray<SkeletonComponent> components;

		TransformSystem* transformSys;
		BaseWorld* world;
	};
}