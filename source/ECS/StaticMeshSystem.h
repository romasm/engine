#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "VisibilitySystem.h"
#include "EarlyVisibilitySystem.h"
#include "MeshMgr.h"
#include "MaterialMgr.h"

namespace EngineCore
{
	struct StaticMeshComponent // TODO
	{
		ENTITY_IN_COMPONENT

		bool dirty;

		bool cast_shadow;

		// static data
		uint32_t stmesh;
		DArray<Material*> materials;

		// update on transform
		Vector3 center;
		ID3D11Buffer* constantBuffer;

		StaticMeshComponent()
		{
			Entity e;
			e.setnull();
			parent = e;
			dirty = true;
			cast_shadow = true;
			stmesh = MeshMgr::nullres;
			constantBuffer = nullptr;
			center = Vector3::Zero;
		}
	};

	class BaseWorld;

	class StaticMeshSystem
	{
		friend BaseWorld;
	public:
		StaticMeshSystem(BaseWorld* w, uint32_t maxCount);
		~StaticMeshSystem();

		StaticMeshComponent* AddComponent(Entity e);

		void CopyComponent(Entity src, Entity dest);
		void DeleteComponent(Entity e);

		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		StaticMeshComponent* GetComponent(Entity e)
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

		bool SetMesh(Entity e, string mesh);
		bool SetMeshAndCallback(Entity e, string mesh, LuaRef func);
		
		bool SetShadow(Entity e, bool cast);
		bool GetShadow(Entity e);

		uint32_t GetMeshID(Entity e);
		string GetMeshLua(Entity e) {return MeshMgr::GetName(GetMeshID(e));}

		bool SetMaterial(Entity e, int32_t i, string matname);

		Material* GetMaterial(Entity e, int32_t i);
		Material* GetMaterialObjectLua(Entity e, int32_t i) {return GetMaterial(e, i);}
		string GetMaterialLua(Entity e, int32_t i) {return GetMaterial(e, i)->GetName();}
		uint16_t GetMaterialsCount(Entity e);

		Vector3 GetCenter(Entity e);

	#ifdef _EDITOR
		void FixBBoxes();
	#endif

		inline bool _AddComponent(Entity e)
		{
			if(AddComponent(e))	return true;
			else return false;
		}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<StaticMeshSystem>("StaticMeshSystem")
					.addFunction("AddComponent", &StaticMeshSystem::_AddComponent)
					.addFunction("DeleteComponent", &StaticMeshSystem::DeleteComponent)
					.addFunction("HasComponent", &StaticMeshSystem::HasComponent)
					
					.addFunction("SetShadow", &StaticMeshSystem::SetShadow)
					.addFunction("GetShadow", &StaticMeshSystem::GetShadow)
					.addFunction("SetMesh", &StaticMeshSystem::SetMesh)
					.addFunction("SetMeshAndCallback", &StaticMeshSystem::SetMeshAndCallback)
					.addFunction("GetMesh", &StaticMeshSystem::GetMeshLua)
					.addFunction("SetMaterial", &StaticMeshSystem::SetMaterial)
					.addFunction("GetMaterial", &StaticMeshSystem::GetMaterialLua)
					.addFunction("GetMaterialObject", &StaticMeshSystem::GetMaterialObjectLua)
					.addFunction("GetMaterialsCount", &StaticMeshSystem::GetMaterialsCount)

					.addFunction("GetCenter", &StaticMeshSystem::GetCenter)
				.endClass();
		}

	private:
		inline void destroyMeshData(StaticMeshComponent& comp);
		bool setMeshMats(StaticMeshComponent* comp, string& mesh, DArray<string>& mats);
		bool setMesh(StaticMeshComponent* comp, string& mesh, LuaRef func);

		ComponentRArray<StaticMeshComponent> components;

		TransformSystem* transformSys;
		VisibilitySystem* visibilitySys;
		EarlyVisibilitySystem* earlyVisibilitySys;

		FrustumMgr* frustumMgr;
		BaseWorld* world;
	};
}