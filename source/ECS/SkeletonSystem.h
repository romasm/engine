#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "VisibilitySystem.h"
#include "EarlyVisibilitySystem.h"
#include "MeshMgr.h"

namespace EngineCore
{
	struct SkeletonComponent
	{
		ENTITY_IN_COMPONENT

		bool dirty;

		uint32_t skeletonID;
		DArray<uint32_t> bones;

		StructBuf gpuMatrixBuffer;
		DArrayAligned<XMMATRIX> matrixBuffer;

		SkeletonComponent()
		{
			parent.setnull();
			dirty = true;
			skeletonID = SkeletonMgr::nullres;
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
		
		void Animate();
		void UpdateBuffers();

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool SetSkeleton(Entity e, string mesh);
		bool SetSkeletonAndCallback(Entity e, string mesh, LuaRef func);

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

					.addFunction("SetSkeleton", &SkeletonSystem::SetSkeleton)
					.addFunction("SetSkeletonAndCallback", &SkeletonSystem::SetSkeletonAndCallback)					
				.endClass();
		}

		bool updateSkeleton(SkeletonComponent& comp);

	private:
		bool setSkeleton(SkeletonComponent* comp, string& skeleton, LuaRef func);
		inline void destroySkeleton(SkeletonComponent& comp)
		{
			for(int32_t i = (int32_t)comp.bones.size() - 1; i >= 0; i--)
				sceneGraph->DeleteNode(comp.bones[i]);
			SkeletonMgr::Get()->DeleteResource(comp.skeletonID);
			comp.skeletonID = SkeletonMgr::nullres;
			comp.bones.destroy();
			comp.matrixBuffer.destroy();
			comp.gpuMatrixBuffer.Release();
		};

		ComponentRArray<SkeletonComponent> components;

		SceneGraph* sceneGraph;
		TransformSystem* transformSys;
		VisibilitySystem* visibilitySys;
		EarlyVisibilitySystem* earlyVisibilitySys;
		BaseWorld* world;
	};
}