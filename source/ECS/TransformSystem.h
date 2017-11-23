#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "SceneGraph.h"

namespace EngineCore
{
	struct TransformComponent
	{
		ENTITY_IN_COMPONENT
				
		uint32_t nodeID; 

		TransformComponent()
		{
			nodeID = SCENEGRAPH_NULL_ID;
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
			if(HasComponent(e))
				return &components.getDataById(e.index());

			TransformComponent* res = components.add(e.index());
			res->nodeID = sceneGraph->AddNode(e);
			res->parent = e;
			return res;
		}
		void CopyComponent(Entity src, Entity dest)
		{
			auto comp = GetComponent(src);
			if(!comp || HasComponent(dest)) 
				return;

			TransformComponent* res = components.add(dest.index());
			res->nodeID = sceneGraph->CopyNode(comp->nodeID, dest);
			res->parent = dest;
		}
		void DeleteComponent(Entity e)
		{
			auto& comp = components.getDataById(e.index());
			sceneGraph->DeleteNode(comp.nodeID);
			components.remove(e.index());
		}
		bool HasComponent(Entity e) const { return components.has(e.index()); }
		size_t ComponentsCount() {return components.dataSize();}

		inline TransformComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
				
		void ForceUpdate(Entity e)
		{
			TransformComponent* comp = GetComponent(e);
			if(!comp)
				return;
			sceneGraph->ForceUpdate(comp->nodeID);
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

		bool SetPhysicsTransform(Entity e, XMMATRIX& transform);

		bool SetPosition(Entity e, float x, float y, float z);
		bool SetPosition(Entity e, XMVECTOR p);

		bool SetRotation(Entity e, float p, float y, float r);
		bool SetRotation(Entity e, XMVECTOR normalAxis, float angle);
		inline bool SetRotation(Entity e, Vector3& axis, float angle) 
		{return SetRotation(e, XMVector3Normalize(XMLoadFloat3(&axis)), angle);}

		bool SetRotation(Entity e, XMVECTOR quat);
		inline bool SetRotation(Entity e, Vector4& quat)
		{return SetRotation(e, XMLoadFloat4(&quat));}

		bool SetScale(Entity e, float x, float y, float z);
		bool SetScale(Entity e, XMVECTOR s);

		bool SetTransform(Entity e, CXMMATRIX mat);

		bool AddPosition(Entity e, float x, float y, float z);
		bool AddPosition(Entity e, XMVECTOR p);

		bool AddPositionLocal(Entity e, float x, float y, float z);

		bool AddRotation(Entity e, float p, float y, float r);
		bool AddRotation(Entity e, XMVECTOR normalAxis, float angle);
		inline bool AddRotation(Entity e, Vector3& axis, float angle) 
		{return AddRotation(e, XMVector3Normalize(XMLoadFloat3(&axis)), angle);}

		bool AddRotation(Entity e, XMVECTOR quat);
		inline bool AddRotation(Entity e, Vector4& quat)
		{return AddRotation(e, XMLoadFloat4(&quat));}

		bool AddScale(Entity e, float x, float y, float z);
		bool AddScale(Entity e, XMVECTOR s);

		bool AddTransform(Entity e, CXMMATRIX mat);

		
		XMVECTOR GetVectPositionL(Entity e);
		inline Vector3 GetPositionL(Entity e)
		{XMVECTOR v = GetVectPositionL(e); Vector3 res;
			XMStoreFloat3(&res, v); return res;}

		Quaternion GetQuatRotationL(Entity e);
		Vector3 GetRotationL(Entity e);
		inline XMVECTOR GetVectRotationL(Entity e)
		{Vector3 res = GetRotationL(e); return XMLoadFloat3(&res);}

		Vector3 GetDirectionL(Entity e);

		XMVECTOR GetVectScaleL(Entity e);
		inline Vector3 GetScaleL(Entity e)
		{XMVECTOR v = GetVectScaleL(e); Vector3 res;
			XMStoreFloat3(&res, v); return res;}

		XMMATRIX GetTransformL(Entity e);

		XMVECTOR GetVectPositionW(Entity e);
		inline Vector3 GetPositionW(Entity e)
		{XMVECTOR v = GetVectPositionW(e); Vector3 res;
			XMStoreFloat3(&res, v); return res;}

		Vector3 GetDirectionW(Entity e);

		Quaternion GetQuatRotationW(Entity e);
		Vector3 GetRotationW(Entity e);
		inline XMVECTOR GetVectRotationW(Entity e)
		{Vector3 res = GetRotationW(e); return XMLoadFloat3(&res);}

		XMVECTOR GetVectScaleW(Entity e);
		inline Vector3 GetScaleW(Entity e)
		{XMVECTOR v = GetVectScaleW(e); Vector3 res;
			XMStoreFloat3(&res, v); return res;}

		XMMATRIX GetTransformW(Entity e);

		ALIGNED_ALLOCATION

		inline bool _SetPosition(Entity e, float x, float y, float z){return SetPosition(e, x, y, z);}
		inline bool _SetPositionV(Entity e, Vector3& vect){return SetPosition(e, vect);}
		inline bool _SetRotation(Entity e, float p, float y, float r){return SetRotation(e, p, y, r);}
		inline bool _SetRotationAxis(Entity e, Vector3& axis, float angle){return SetRotation(e, axis, angle);}
		inline bool _SetRotationQ(Entity e, Quaternion& vect){return SetRotation(e, XMVECTOR(vect));}
		inline bool _SetScale(Entity e, float x, float y, float z){return SetScale(e, x, y, z);}
		inline bool _SetScaleV(Entity e, Vector3& vect){return SetScale(e, vect);}

		inline bool _AddPosition(Entity e, float x, float y, float z){return AddPosition(e, x, y, z);}
		inline bool _AddRotation1(Entity e, float x, float y, float z){return AddRotation(e, x, y, z);}
		inline bool _AddRotation2(Entity e, Vector3& normalAxis, float angle){return AddRotation(e, normalAxis, angle);}
		inline bool _AddScale(Entity e, float x, float y, float z){return AddScale(e, x, y, z);}

		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<TransformSystem>("TransformSystem")

					.addFunction("SetPositionL", &TransformSystem::_SetPositionL)
					.addFunction("SetRotationL", &TransformSystem::_SetRotationL)
					.addFunction("SetScaleL", &TransformSystem::_SetScaleL)

					.addFunction("SetRotationLAxis", &TransformSystem::_SetRotationLAxis)
					.addFunction("SetPositionLVect", &TransformSystem::_SetPositionLVect)
					.addFunction("SetRotationLQuat", &TransformSystem::_SetRotationLQuat)
					.addFunction("SetScaleLVect", &TransformSystem::_SetScaleLVect)

					.addFunction("SetPositionW", &TransformSystem::_SetPositionW)
					.addFunction("SetRotationW", &TransformSystem::_SetRotationW)
					.addFunction("SetScaleW", &TransformSystem::_SetScaleW)

					.addFunction("SetRotationWAxis", &TransformSystem::_SetRotationWAxis)
					.addFunction("SetPositionWVect", &TransformSystem::_SetPositionWVect)
					.addFunction("SetRotationWQuat", &TransformSystem::_SetRotationWQuat)
					.addFunction("SetScaleWVect", &TransformSystem::_SetScaleWVect)

					.addFunction("GetPositionL", &TransformSystem::GetPositionL)
					.addFunction("GetRotationL", &TransformSystem::GetRotationL)
					.addFunction("GetRotationLQuat", &TransformSystem::GetRotationLQuat)
					.addFunction("GetDirectionL", &TransformSystem::GetDirectionL)
					.addFunction("GetScaleL", &TransformSystem::GetScaleL)
					.addFunction("GetPositionW", &TransformSystem::GetPositionW)
					.addFunction("GetRotationW", &TransformSystem::GetRotationW)
					.addFunction("GetRotationWQuat", &TransformSystem::GetRotationWQuat)
					.addFunction("GetDirectionW", &TransformSystem::GetDirectionW)
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

		ComponentRArray<TransformComponent> components;
		
		// on load only
		unordered_map<uint32_t, string>* attachments_map;

		BaseWorld* world;
		SceneGraph* sceneGraph;
	};
}