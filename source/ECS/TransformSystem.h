#pragma once

#include "ECS_defines.h"
#include "Entity.h"

#define MAX_HIERARCHY_DEPTH 128

namespace EngineCore
{
	struct TransformComponent
	{
		ENTITY_IN_COMPONENT
			
		XMMATRIX localMatrix;
		XMMATRIX worldMatrix;
		
		uint32_t parentID; 
		uint32_t firstChildID;
		uint32_t nextID;
		uint32_t prevID;
		
		ALIGNED_ALLOCATION

		TransformComponent()
		{
			localMatrix = XMMatrixIdentity();
			worldMatrix = XMMatrixIdentity();
			parentID = ENTITY_COUNT;
			firstChildID = ENTITY_COUNT;
			nextID = ENTITY_COUNT;
			prevID = ENTITY_COUNT;
		}
	};

	class BaseWorld;

	class TransformSystem
	{
	public:
		TransformSystem(BaseWorld* w, uint32_t maxCount);
		~TransformSystem()
		{
			_DELETE(attachments_map);
		}

		TransformComponent* AddComponent(Entity e)
		{
			TransformComponent* res = nullptr;
			if(e.index() >= capacity)
				return res;

			uint32_t& id = lookup[e.index()];
			if(id >= capacity)
			{
				id = (uint32_t)components.size();
				res = components.push_back();
				dirty.push_back(true);
			}
			else
			{
				res = &components[id];
				dirty[id] = true;
			}			

			*res = TransformComponent();
			res->parent = e;
			return res;
		}
		void CopyComponent(Entity src, Entity dest)
		{
			auto comp = GetComponent(src);
			if(!comp) return;
			auto res = AddComponent(dest);
			res->localMatrix = comp->localMatrix;
			res->worldMatrix = comp->worldMatrix;
		}
		void DeleteComponent(Entity e);
		bool HasComponent(Entity e) const
		{ return e.index() < capacity && lookup[e.index()] < capacity; }
		size_t ComponentsCount() {return components.size();}

		inline TransformComponent* GetComponent(Entity e)
		{
			size_t idx = lookup[e.index()];
			if(idx >= capacity) return nullptr;
			return &components[idx];
		}
		
		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);

		void Update();

		// TODO: prevent non-uniform scaling for parents
		// 1 - dont inherit scaling
		// 2 - inherit uniformed scaling
		inline void UpdateComponent(uint32_t& id)
		{
			TransformComponent& comp = components[id];
			if(comp.parentID < capacity)
				comp.worldMatrix = XMMatrixMultiply(comp.localMatrix, components[comp.parentID].worldMatrix);
			else
				comp.worldMatrix = comp.localMatrix;
			dirty[id] = false;
		}

		void ForceUpdate(Entity e)
		{
			uint32_t idx = lookup[e.index()];
			if(idx >= capacity)
				return;
			UpdateComponent(idx);
		}

		void PreLoad();
		bool PostLoadParentsResolve();

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool Attach(Entity child, Entity parent);
		bool Detach(Entity child);
		bool DetachChildren(Entity parent);

		Entity GetParent(Entity child);
		Entity GetChildFirst(Entity parent);
		Entity GetChildNext(Entity child);

		bool SetPosition(Entity e, float x, float y, float z);
		bool SetPosition(Entity e, XMVECTOR p);

		bool SetRotation(Entity e, float p, float y, float r);
		bool SetRotation(Entity e, XMVECTOR normalAxis, float angle);
		inline bool SetRotation(Entity e, XMFLOAT3 axis, float angle) 
		{return SetRotation(e, XMVector3Normalize(XMLoadFloat3(&axis)), angle);}

		bool SetRotation(Entity e, XMVECTOR quat);
		inline bool SetRotation(Entity e, XMFLOAT4 quat)
		{return SetRotation(e, XMLoadFloat4(&quat));}

		bool SetScale(Entity e, float x, float y, float z);
		bool SetScale(Entity e, XMVECTOR s);

		bool SetTransform(Entity e, CXMMATRIX mat);

		bool AddPosition(Entity e, float x, float y, float z);
		bool AddPosition(Entity e, XMVECTOR p);

		bool AddPositionLocal(Entity e, float x, float y, float z);

		bool AddRotation(Entity e, float p, float y, float r);
		bool AddRotation(Entity e, XMVECTOR normalAxis, float angle);
		inline bool AddRotation(Entity e, XMFLOAT3 axis, float angle) 
		{return AddRotation(e, XMVector3Normalize(XMLoadFloat3(&axis)), angle);}

		bool AddRotation(Entity e, XMVECTOR quat);
		inline bool AddRotation(Entity e, XMFLOAT4 quat)
		{return AddRotation(e, XMLoadFloat4(&quat));}

		bool AddScale(Entity e, float x, float y, float z);
		bool AddScale(Entity e, XMVECTOR s);

		bool AddTransform(Entity e, CXMMATRIX mat);

		
		XMVECTOR GetVectPositionL(Entity e);
		inline XMFLOAT3 GetPositionL(Entity e)
		{XMVECTOR v = GetVectPositionL(e); XMFLOAT3 res;
			XMStoreFloat3(&res, v); return res;}

		XMFLOAT3 GetRotationL(Entity e);
		inline XMVECTOR GetVectRotationL(Entity e)
		{XMFLOAT3 res = GetRotationL(e); return XMLoadFloat3(&res);}

		XMVECTOR GetVectScaleL(Entity e);
		inline XMFLOAT3 GetScaleL(Entity e)
		{XMVECTOR v = GetVectScaleL(e); XMFLOAT3 res;
			XMStoreFloat3(&res, v); return res;}

		XMMATRIX GetTransformL(Entity e);

		XMVECTOR GetVectPositionW(Entity e);
		inline XMFLOAT3 GetPositionW(Entity e)
		{XMVECTOR v = GetVectPositionW(e); XMFLOAT3 res;
			XMStoreFloat3(&res, v); return res;}

		XMFLOAT3 GetRotationW(Entity e);
		inline XMVECTOR GetVectRotationW(Entity e)
		{XMFLOAT3 res = GetRotationW(e); return XMLoadFloat3(&res);}

		XMVECTOR GetVectScaleW(Entity e);
		inline XMFLOAT3 GetScaleW(Entity e)
		{XMVECTOR v = GetVectScaleW(e); XMFLOAT3 res;
			XMStoreFloat3(&res, v); return res;}

		XMMATRIX GetTransformW(Entity e);

		ALIGNED_ALLOCATION

		inline bool _SetPosition(Entity e, float x, float y, float z){return SetPosition(e, x, y, z);}
		inline bool _SetRotation1(Entity e, float p, float y, float r){return SetRotation(e, p, y, r);}
		inline bool _SetRotation2(Entity e, XMFLOAT3 axis, float angle){return SetRotation(e, axis, angle);}
		inline bool _SetScale(Entity e, float x, float y, float z){return SetScale(e, x, y, z);}
		inline bool _AddPosition(Entity e, float x, float y, float z){return AddPosition(e, x, y, z);}
		inline bool _AddRotation1(Entity e, float x, float y, float z){return AddRotation(e, x, y, z);}
		inline bool _AddRotation2(Entity e, XMFLOAT3 normalAxis, float angle){return AddRotation(e, normalAxis, angle);}
		inline bool _AddScale(Entity e, float x, float y, float z){return AddScale(e, x, y, z);}

		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<TransformSystem>("TransformSystem")
					.addFunction("SetPosition", &TransformSystem::_SetPosition)
					.addFunction("SetRotation", &TransformSystem::_SetRotation1)
					.addFunction("SetRotationAxis", &TransformSystem::_SetRotation2)
					.addFunction("SetScale", &TransformSystem::_SetScale)
					.addFunction("GetPositionL", &TransformSystem::GetPositionL)
					.addFunction("GetRotationL", &TransformSystem::GetRotationL)
					.addFunction("GetScaleL", &TransformSystem::GetScaleL)
					.addFunction("GetPositionW", &TransformSystem::GetPositionW)
					.addFunction("GetRotationW", &TransformSystem::GetRotationW)
					.addFunction("GetScaleW", &TransformSystem::GetScaleW)
					.addFunction("AddPosition", &TransformSystem::_AddPosition)
					.addFunction("AddRotation", &TransformSystem::_AddRotation1)
					.addFunction("AddRotationAxis", &TransformSystem::_AddRotation2)
					.addFunction("AddScale", &TransformSystem::_AddScale)

					.addFunction("AddPositionLocal", &TransformSystem::AddPositionLocal)

					.addFunction("Attach", &TransformSystem::Attach)
					.addFunction("Detach", &TransformSystem::Detach)
					.addFunction("DetachChildren", &TransformSystem::DetachChildren)
					.addFunction("GetParent", &TransformSystem::GetParent)
					.addFunction("GetChildFirst", &TransformSystem::GetChildFirst)
					.addFunction("GetChildNext", &TransformSystem::GetChildNext)

					.addFunction("ForceUpdate", &TransformSystem::ForceUpdate)

					.addFunction("AddComponent", &TransformSystem::_AddComponent)
					.addFunction("DeleteComponent", &TransformSystem::DeleteComponent)
					.addFunction("HasComponent", &TransformSystem::HasComponent)
				.endClass();
		}
	private:
		inline void _detach(TransformComponent& childComp, uint32_t childID)
		{
			if(childComp.parentID >= capacity)
				return;
			
			auto& oldParentComp = components[childComp.parentID];
			if(oldParentComp.firstChildID == childID)
				oldParentComp.firstChildID = childComp.nextID;
			if(childComp.prevID < capacity)
			{
				auto& prevComp = components[childComp.prevID];
				prevComp.nextID = childComp.nextID;
			}
			if(childComp.nextID < capacity)
			{
				auto& nextComp = components[childComp.nextID];
				nextComp.prevID = childComp.prevID;
			}
			childComp.parentID = capacity;
			childComp.prevID = capacity;
			childComp.nextID = capacity;	
		}

		inline void _detachChildren(TransformComponent& parentComp)
		{			
			uint32_t child = parentComp.firstChildID;
			while( child < capacity )
			{
				auto& childComp = components[child];
				uint32_t next = childComp.nextID;
				childComp.parentID = capacity;
				childComp.prevID = capacity;
				childComp.nextID = capacity;	
				child = next;
			}

			parentComp.firstChildID = capacity;
		}

		inline void _moveComponentPrepare(uint32_t whatID, uint32_t whereID)
		{
			auto& whatComp = components[whatID];
			if(whatComp.prevID < capacity)
			{
				auto& prevComp = components[whatComp.prevID];
				prevComp.nextID = whereID;
			}
			if(whatComp.nextID < capacity)
			{
				auto& nextComp = components[whatComp.nextID];
				nextComp.prevID = whereID;
			}
			if(whatComp.parentID < capacity)
			{
				auto& parentComp = components[whatComp.parentID];
				if(parentComp.firstChildID == whatID)
					parentComp.firstChildID = whereID;
			}

			uint32_t childID = whatComp.firstChildID;
			while( childID < capacity )
			{
				auto& childComp = components[childID];
				childComp.parentID = whereID;
				childID = childComp.nextID;
			}
			
		}

		// ~ 12 MB total size
		uint32_t capacity;

		RArray<TransformComponent> components;
		
		RArray<bool> dirty;
		RArray<uint32_t> lookup;

		struct sort_data { int16_t hierarchy; uint32_t new_id; };
		RArray<sort_data> hierarchy_sort;
		RArray<uint32_t> links_fix;

		SArray<uint32_t, MAX_HIERARCHY_DEPTH> hi_buffer;

		// on load only
		unordered_map<uint32_t, string>* attachments_map;

		BaseWorld* world;

		bool structureChanged;
	};
}