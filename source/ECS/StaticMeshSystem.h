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
		RArray<Material*> materials;

		// update on transform
		XMVECTOR center;
		ID3D11Buffer* constantBuffer;

		StaticMeshComponent()
		{
			Entity e;
			e.setnull();
			parent = e;
			dirty = true;
			cast_shadow = true;
			stmesh = STMESH_NULL;
			constantBuffer = nullptr;
			center = XMVectorZero();
		}
	};

	class BaseWorld;

	class StaticMeshSystem
	{
		friend BaseWorld;
	public:
		StaticMeshSystem(BaseWorld* w, uint32_t maxCount);
		~StaticMeshSystem();

		StaticMeshComponent* AddComponent(Entity e)
		{
			StaticMeshComponent* res = components.add(e.index());
			res->parent = e;
			// todo: static alloc???
			res->constantBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(StmMatrixBuffer), true);
			return res;
		}
		StaticMeshComponent* AddComponent(Entity e, string& mesh)
		{
			StaticMeshComponent* res = AddComponent(e);
			if(!SetMesh(e, mesh))
				return nullptr;
			return res;
		}
		StaticMeshComponent* AddComponent(Entity e, string& mesh, RArray<string>& mats)
		{
			StaticMeshComponent* res = AddComponent(e);
			if(!SetMeshMats(e, mesh, mats))
				return nullptr;
			return res;
		}
		void AddComponent(Entity e, StaticMeshComponent D)
		{
			D.dirty = true;
			D.parent = e;
			if(!D.constantBuffer)
				D.constantBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(StmMatrixBuffer), true);
			visibilitySys->SetBBox(e, MeshMgr::GetStMeshPtr(D.stmesh)->box);
			components.add(e.index(), D);
		}
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
		
		bool SetShadow(Entity e, bool cast);
		bool GetShadow(Entity e);

		uint32_t GetMeshID(Entity e);
		string GetMeshLua(Entity e) {return MeshMgr::GetName(GetMeshID(e));}

		bool SetMaterial(Entity e, int i, string matname);
		bool SetMeshMats(Entity e, string& mesh, RArray<string>& mats);

		Material* GetMaterial(Entity e, int i);
		Material* GetMaterialObjectLua(Entity e, int i) {return GetMaterial(e, i);}
		string GetMaterialLua(Entity e, int i) {return GetMaterial(e, i)->GetName();}
		uint16_t GetMaterialsCount(Entity e);

		XMVECTOR GetCenter(Entity e);
		Vector3 GetCenterLua(Entity e) {Vector3 res; XMStoreFloat3(&res, GetCenter(e)); return res;}

	#ifdef _EDITOR
		void FixBBoxes();
	#endif

		inline bool _AddComponent(Entity e)
		{
			if(AddComponent(e))	return true;
			else return false;
		}
		
		inline bool _AddComponentMesh(Entity e, string name)
		{
			if(AddComponent(e, name))return true;
			else return false;
		}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<StaticMeshSystem>("StaticMeshSystem")
					.addFunction("AddComponent", &StaticMeshSystem::_AddComponent)
					.addFunction("AddComponentMesh", &StaticMeshSystem::_AddComponentMesh)
					.addFunction("DeleteComponent", &StaticMeshSystem::DeleteComponent)
					.addFunction("HasComponent", &StaticMeshSystem::HasComponent)
					
					.addFunction("SetShadow", &StaticMeshSystem::SetShadow)
					.addFunction("GetShadow", &StaticMeshSystem::GetShadow)
					.addFunction("SetMesh", &StaticMeshSystem::SetMesh)
					.addFunction("GetMesh", &StaticMeshSystem::GetMeshLua)
					.addFunction("SetMaterial", &StaticMeshSystem::SetMaterial)
					.addFunction("GetMaterial", &StaticMeshSystem::GetMaterialLua)
					.addFunction("GetMaterialObject", &StaticMeshSystem::GetMaterialObjectLua)
					.addFunction("GetMaterialsCount", &StaticMeshSystem::GetMaterialsCount)

					.addFunction("GetCenter", &StaticMeshSystem::GetCenterLua)
				.endClass();
		}

	private:
		inline void destroyMeshData(StaticMeshComponent& comp);

		ComponentRArray<StaticMeshComponent> components;

		TransformSystem* transformSys;
		VisibilitySystem* visibilitySys;
		EarlyVisibilitySystem* earlyVisibilitySys;

		FrustumMgr* frustumMgr;
		BaseWorld* world;
	};
}