#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "VisibilitySystem.h"
#include "EarlyVisibilitySystem.h"
#include "SkeletalMeshMgr.h"
#include "MaterialMgr.h"

namespace EngineCore
{
	struct SkeletalMeshComponent
	{
		ENTITY_IN_COMPONENT

		bool dirty;
		bool editor_only;

		bool cast_shadow;

		// static data
		uint32_t SkeletalMesh;
		RArray<Material*> materials;

		// update on transform
		XMVECTOR center;
		ID3D11Buffer* constantBuffer;

		SkeletalMeshComponent()
		{
			Entity e;
			e.setnull();
			parent = e;
			dirty = true;
			editor_only = false;
			cast_shadow = true;
			SkeletalMesh = SKELETAL_MESH_NULL;
			constantBuffer = nullptr;
			center = XMVectorZero();
		}
	};

	class BaseWorld;

	class SkeletalMeshSystem
	{
		friend BaseWorld;
	public:
		SkeletalMeshSystem(BaseWorld* w, uint32_t maxCount);
		~SkeletalMeshSystem();

		SkeletalMeshComponent* AddComponent(Entity e)
		{
			SkeletalMeshComponent* res = components.add(e.index());
			res->parent = e;
			// todo: static alloc???
			res->constantBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(StmMatrixBuffer), true);
			return res;
		}
		SkeletalMeshComponent* AddComponent(Entity e, string& mesh)
		{
			SkeletalMeshComponent* res = AddComponent(e);
			if(!SetMesh(e, mesh))
				return nullptr;
			return res;
		}
		SkeletalMeshComponent* AddComponent(Entity e, string& mesh, RArray<string>& mats)
		{
			SkeletalMeshComponent* res = AddComponent(e);
			if(!SetMeshMats(e, mesh, mats))
				return nullptr;
			return res;
		}
		void AddComponent(Entity e, SkeletalMeshComponent D)
		{
			D.dirty = true;
			D.parent = e;
			if(!D.constantBuffer)
				D.constantBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(StmMatrixBuffer), true);
			visibilitySys->SetBBox(e, StMeshMgr::GetStMeshPtr(D.stmesh)->box);
			components.add(e.index(), D);
		}
		void CopyComponent(Entity src, Entity dest);
		void DeleteComponent(Entity e);

		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		SkeletalMeshComponent* GetComponent(Entity e)
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
		
		bool IsEditorOnly(Entity e);
		bool SetEditorOnly(Entity e, bool only_editor);

		bool SetMesh(Entity e, string mesh);
		
		bool SetShadow(Entity e, bool cast);
		bool GetShadow(Entity e);

		uint32_t GetMeshID(Entity e);
		string GetMeshLua(Entity e) {return StMeshMgr::GetName(GetMeshID(e));}

		bool SetMaterial(Entity e, int i, string matname);
		bool SetMeshMats(Entity e, string& mesh, RArray<string>& mats);

		Material* GetMaterial(Entity e, int i);
		Material* GetMaterialObjectLua(Entity e, int i) {return GetMaterial(e, i);}
		string GetMaterialLua(Entity e, int i) {return GetMaterial(e, i)->GetName();}
		uint16_t GetMaterialsCount(Entity e);

		XMVECTOR GetCenter(Entity e);
		XMFLOAT3 GetCenterLua(Entity e) {XMFLOAT3 res; XMStoreFloat3(&res, GetCenter(e)); return res;}

		void PostReload();

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
				.beginClass<SkeletalMeshSystem>("SkeletalMeshSystem")
					.addFunction("AddComponent", &SkeletalMeshSystem::_AddComponent)
					.addFunction("AddComponentMesh", &SkeletalMeshSystem::_AddComponentMesh)
					.addFunction("DeleteComponent", &SkeletalMeshSystem::DeleteComponent)
					.addFunction("HasComponent", &SkeletalMeshSystem::HasComponent)
					
					.addFunction("SetShadow", &SkeletalMeshSystem::SetShadow)
					.addFunction("GetShadow", &SkeletalMeshSystem::GetShadow)
					.addFunction("SetMesh", &SkeletalMeshSystem::SetMesh)
					.addFunction("GetMesh", &SkeletalMeshSystem::GetMeshLua)
					.addFunction("SetMaterial", &SkeletalMeshSystem::SetMaterial)
					.addFunction("GetMaterial", &SkeletalMeshSystem::GetMaterialLua)
					.addFunction("GetMaterialObject", &SkeletalMeshSystem::GetMaterialObjectLua)
					.addFunction("GetMaterialsCount", &SkeletalMeshSystem::GetMaterialsCount)

					.addFunction("GetCenter", &SkeletalMeshSystem::GetCenterLua)

					.addFunction("IsEditor", &SkeletalMeshSystem::IsEditorOnly)
					.addFunction("SetEditor", &SkeletalMeshSystem::SetEditorOnly)
				.endClass();
		}

	private:
		inline void destroyMeshData(SkeletalMeshComponent& comp);

		ComponentRArray<SkeletalMeshComponent> components;

		TransformSystem* transformSys;
		VisibilitySystem* visibilitySys;
		EarlyVisibilitySystem* earlyVisibilitySys;

		FrustumMgr* frustumMgr;
		BaseWorld* world;
	};
}