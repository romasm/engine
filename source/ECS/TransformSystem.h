#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "SceneGraph.h"

namespace EngineCore
{
	enum Mobility
	{
		MOBILITY_STATIC = 0,
		MOBILITY_DYNAMIC = 1
	};

	struct TransformComponent
	{
		ENTITY_IN_COMPONENT
				
		uint32_t nodeID; 
		Mobility mobility;

		TransformComponent()
		{
			nodeID = SCENEGRAPH_NULL_ID;
			mobility = MOBILITY_STATIC;
		}
	};

	class BaseWorld;

	class TransformSystem
	{
		friend class GIMgr;

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

		void ForceUpdateHierarchy(Entity e)
		{
			TransformComponent* comp = GetComponent(e);
			if(!comp)
				return;		
			sceneGraph->ForceUpdateHierarchy(comp->nodeID);
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

		// TODO
		bool SetPhysicsTransform(Entity e, XMMATRIX& transform);

		bool SetMobility(Entity e, uint32_t mbl);
		uint32_t GetMobility(Entity e);

		// Set Transform
		bool SetPosition_L3F(Entity e, float x, float y, float z);
		bool SetPosition_L(Entity e, Vector3& p);
		bool SetRotationPYR_L3F(Entity e, float p, float y, float r);
		bool SetRotationPYR_L(Entity e, Vector3& r);
		bool SetRotation_L(Entity e, Quaternion& quat);
		bool SetRotationAxis_L(Entity e, Vector3& normalAxis, float angle);
		bool SetScale_L3F(Entity e, float x, float y, float z);
		bool SetScale_L(Entity e, Vector3& s);
		
		bool SetPosition_W3F(Entity e, float x, float y, float z);
		bool SetPosition_W(Entity e, Vector3& p);
		bool SetRotationPYR_W3F(Entity e, float p, float y, float r);
		bool SetRotationPYR_W(Entity e, Vector3& r);
		bool SetRotation_W(Entity e, Quaternion& quat);
		bool SetRotationAxis_W(Entity e, Vector3& normalAxis, float angle);
		bool SetScale_W3F(Entity e, float x, float y, float z);
		bool SetScale_W(Entity e, Vector3& s);

		bool SetTransform_LInternal(Entity e, XMMATRIX& mat);
		inline bool SetTransform_L(Entity e, Matrix& mat) {return SetTransform_LInternal(e, XMMATRIX(mat));};
		bool SetTransform_WInternal(Entity e, XMMATRIX& mat);
		inline bool SetTransform_W(Entity e, Matrix& mat) {return SetTransform_WInternal(e, XMMATRIX(mat));};
				
		// Get Transform		
		Vector3 GetPosition_L(Entity e);
		Quaternion GetRotation_L(Entity e);
		Vector3 GetRotationPYR_L(Entity e);
		Vector3 GetForward_L(Entity e);
		Vector3 GetUpward_L(Entity e);
		Vector3 GetRightward_L(Entity e);
		Vector3 GetScale_L(Entity e);

		Vector3 GetPosition_W(Entity e);
		Quaternion GetRotation_W(Entity e);
		Vector3 GetRotationPYR_W(Entity e);
		Vector3 GetForward_W(Entity e);
		Vector3 GetUpward_W(Entity e);
		Vector3 GetRightward_W(Entity e);
		Vector3 GetScale_W(Entity e);

		const XMMATRIX& GetTransform_LInternal(Entity e);
		inline Matrix GetTransform_L(Entity e) {return GetTransform_LInternal(e);}
		const XMMATRIX& GetTransform_WInternal(Entity e);
		inline Matrix GetTransform_W(Entity e) {return GetTransform_WInternal(e);}

		// Add Transform
		bool AddPosition_L3F(Entity e, float x, float y, float z);
		bool AddPosition_L(Entity e, Vector3& p);
		bool AddRotationPYR_L(Entity e, Vector3& r);
		bool AddRotationPYR_L3F(Entity e, float p, float y, float r);
		bool AddRotationAxis_L(Entity e, Vector3& normalAxis, float angle);
		bool AddRotation_L(Entity e, Quaternion& quat);
		bool AddScale_L3F(Entity e, float x, float y, float z);
		bool AddScale_L(Entity e, Vector3& s);

		bool AddPosition_W3F(Entity e, float x, float y, float z);
		bool AddPosition_W(Entity e, Vector3& p);
		bool AddRotationPYR_W(Entity e, Vector3& r);
		bool AddRotationPYR_W3F(Entity e, float p, float y, float r);
		bool AddRotationAxis_W(Entity e, Vector3& normalAxis, float angle);
		bool AddRotation_W(Entity e, Quaternion& quat);
		bool AddScale_W3F(Entity e, float x, float y, float z);
		bool AddScale_W(Entity e, Vector3& s);
		
		bool PostTransform_L(Entity e, Matrix& mat);
		bool PostTransform_W(Entity e, Matrix& mat);
		bool PreTransform_L(Entity e, Matrix& mat);
		bool PreTransform_W(Entity e, Matrix& mat);

		ALIGNED_ALLOCATION

		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<TransformSystem>("TransformSystem")

					.addFunction("SetPosition_L3F", &TransformSystem::SetPosition_L3F)
					.addFunction("SetRotationPYR_L3F", &TransformSystem::SetRotationPYR_L3F)
					.addFunction("SetScale_L3F", &TransformSystem::SetScale_L3F)
					.addFunction("SetRotationPYR_L", &TransformSystem::SetRotationPYR_L)
					.addFunction("SetRotationAxis_L", &TransformSystem::SetRotationAxis_L)
					.addFunction("SetPosition_L", &TransformSystem::SetPosition_L)
					.addFunction("SetRotation_L", &TransformSystem::SetRotation_L)
					.addFunction("SetScale_L", &TransformSystem::SetScale_L)
					.addFunction("SetPosition_W3F", &TransformSystem::SetPosition_W3F)
					.addFunction("SetRotationPYR_W3F", &TransformSystem::SetRotationPYR_W3F)
					.addFunction("SetScale_W3F", &TransformSystem::SetScale_W3F)
					.addFunction("SetRotationPYR_W", &TransformSystem::SetRotationPYR_W)
					.addFunction("SetRotationAxis_W", &TransformSystem::SetRotationAxis_W)
					.addFunction("SetPosition_W", &TransformSystem::SetPosition_W)
					.addFunction("SetRotation_W", &TransformSystem::SetRotation_W)
					.addFunction("SetScale_W", &TransformSystem::SetScale_W)
					.addFunction("SetTransform_L", &TransformSystem::SetTransform_L)
					.addFunction("SetTransform_W", &TransformSystem::SetTransform_W)

					.addFunction("SetMobility", &TransformSystem::SetMobility)

					.addFunction("GetPosition_L", &TransformSystem::GetPosition_L)
					.addFunction("GetRotationPYR_L", &TransformSystem::GetRotationPYR_L)
					.addFunction("GetRotation_L", &TransformSystem::GetRotation_L)
					.addFunction("GetForward_L", &TransformSystem::GetForward_L)
					.addFunction("GetUpward_L", &TransformSystem::GetUpward_L)
					.addFunction("GetRightward_L", &TransformSystem::GetRightward_L)
					.addFunction("GetScale_L", &TransformSystem::GetScale_L)
					.addFunction("GetPosition_W", &TransformSystem::GetPosition_W)
					.addFunction("GetRotationPYR_W", &TransformSystem::GetRotationPYR_W)
					.addFunction("GetRotation_W", &TransformSystem::GetRotation_W)
					.addFunction("GetForward_W", &TransformSystem::GetForward_W)
					.addFunction("GetUpward_W", &TransformSystem::GetUpward_W)
					.addFunction("GetRightward_W", &TransformSystem::GetRightward_W)
					.addFunction("GetScale_W", &TransformSystem::GetScale_W)
					.addFunction("GetTransform_L", &TransformSystem::GetTransform_L)
					.addFunction("GetTransform_W", &TransformSystem::GetTransform_W)

					.addFunction("GetMobility", &TransformSystem::GetMobility)

					.addFunction("AddPosition_L3F", &TransformSystem::AddPosition_L3F)
					.addFunction("AddRotationPYR_L3F", &TransformSystem::AddRotationPYR_L3F)
					.addFunction("AddScale_L3F", &TransformSystem::AddScale_L3F)
					.addFunction("AddRotationPYR_L", &TransformSystem::AddRotationPYR_L)
					.addFunction("AddRotationAxis_L", &TransformSystem::AddRotationAxis_L)
					.addFunction("AddPosition_L", &TransformSystem::AddPosition_L)
					.addFunction("AddRotation_L", &TransformSystem::AddRotation_L)
					.addFunction("AddScale_L", &TransformSystem::AddScale_L)
					.addFunction("AddPosition_W3F", &TransformSystem::AddPosition_W3F)
					.addFunction("AddRotationPYR_W3F", &TransformSystem::AddRotationPYR_W3F)
					.addFunction("AddScale_W3F", &TransformSystem::AddScale_W3F)
					.addFunction("AddRotationPYR_W", &TransformSystem::AddRotationPYR_W)
					.addFunction("AddRotationAxis_W", &TransformSystem::AddRotationAxis_W)
					.addFunction("AddPosition_W", &TransformSystem::AddPosition_W)
					.addFunction("AddRotation_W", &TransformSystem::AddRotation_W)
					.addFunction("AddScale_W", &TransformSystem::AddScale_W)

					.addFunction("PostTransform_L", &TransformSystem::PostTransform_L)
					.addFunction("PostTransform_W", &TransformSystem::PostTransform_W)
					.addFunction("PreTransform_L", &TransformSystem::PreTransform_L)
					.addFunction("PreTransform_W", &TransformSystem::PreTransform_W)

					.addFunction("Attach", &TransformSystem::Attach)
					.addFunction("Detach", &TransformSystem::Detach)
					.addFunction("DetachChildren", &TransformSystem::DetachChildren)
					.addFunction("GetParent", &TransformSystem::GetParent)
					.addFunction("GetChildFirst", &TransformSystem::GetChildFirst)
					.addFunction("GetChildNext", &TransformSystem::GetChildNext)

					.addFunction("ForceUpdate", &TransformSystem::ForceUpdate)
					.addFunction("ForceUpdateHierarchy", &TransformSystem::ForceUpdateHierarchy)

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