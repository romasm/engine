#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "PhysicsSystem.h"
#include "CollisionMgr.h"
#include "MaterialMgr.h"

#define COLLISION_RESOURCE_NULL 0

namespace EngineCore
{
	enum CollisionStorageType
	{
		LOCAL = 0,
		RESOURCE,
	};

	struct CollisionHandle
	{
		CollisionStorageType stoarge;
		rp3d::CollisionShape* shape;
		rp3d::ProxyShape* proxy;

		CollisionHandle() : shape(nullptr), proxy(nullptr), stoarge(CollisionStorageType::LOCAL) {}
		CollisionHandle(rp3d::CollisionShape* s, rp3d::ProxyShape* p, CollisionStorageType strg) 
			: shape(s), proxy(p), stoarge(strg) {}
	};

	struct CollisionComponent
	{
		ENTITY_IN_COMPONENT
		
		bool dirty;
		
		rp3d::CollisionBody* body;
		bool physicsBody;
		
		uint32_t resourceId;
		DArray<CollisionHandle> shapes;

	#ifdef _DEV
		bool dirty_vis;
		ID3D11Buffer* constantBuffer;
		Material* material;
	#endif

		CollisionComponent()
		{
			body = nullptr;
			dirty = false;
			physicsBody = false;
			resourceId = CollisionMgr::nullres;

		#ifdef _DEV
			dirty_vis = false;
			constantBuffer = nullptr;
			material = nullptr;
		#endif
		}
	};

	class BaseWorld;

	class CollisionSystem
	{
	public:
		CollisionSystem(BaseWorld* w, rp3d::CollisionWorld* collisionWorld, uint32_t maxCount);
		~CollisionSystem();

		CollisionComponent* AddComponent(Entity e);

		void CopyComponent(Entity src, Entity dest);
		void DeleteComponent(Entity e);
		bool HasComponent(Entity e) const { return components.has(e.index()); }
		size_t ComponentsCount() {return components.dataSize();}

		inline CollisionComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		void UpdateTransformations();

	#ifdef _DEV
		void DebugRegToDraw();
	#endif

		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
				
		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool IsActive(Entity e);
		void SetActive(Entity e, bool active);
		
		int32_t AddBoxCollider(Entity e, Vector3 pos, Quaternion rot, float mass, Vector3 halfSize, float margin);
		int32_t AddSphereCollider(Entity e, Vector3 pos, Quaternion rot, float mass, float radius);
		int32_t AddConeCollider(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height, float margin);
		int32_t AddCylinderCollider(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height, float margin);
		int32_t AddCapsuleCollider(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height);
		void AddConvexCollision(Entity e, string filename);

		inline void _AddComponent(Entity e) {AddComponent(e);}

		void SetDebugDraw(bool draw)
		{
			b_debugDraw = draw;
		}

		static void RegLuaClass();

	private:
		inline int32_t AddShape(CollisionComponent& comp, Vector3& pos, Quaternion& rot, 
			float mass, rp3d::CollisionShape* shape, CollisionStorageType strg)
		{
			rp3d::Transform shapeTransform(pos, rot);
			rp3d::ProxyShape* proxy;
			if(comp.physicsBody)
				proxy = ((rp3d::RigidBody*)comp.body)->addCollisionShape(shape, shapeTransform, mass);
			else
				proxy = comp.body->addCollisionShape(shape, shapeTransform);
			comp.shapes.push_back(CollisionHandle(shape, proxy, strg));
			return (int32_t)comp.shapes.size() - 1;
		}

		void _addConvexCollision(CollisionComponent* comp, string& filename);

		inline void _DeleteComponent(CollisionComponent* comp)
		{
			if(comp->physicsBody)
			{
				if(comp->body != nullptr)
					ERR("Delete PhysicsComponent before deletion of CollisionComponent");
			}
			else
			{
				collisionWorld->destroyCollisionBody(comp->body);
			}
			comp->body = nullptr;

			if( comp->resourceId != CollisionMgr::nullres )
			{
				CollisionMgr::Get()->DeleteResource(comp->resourceId);
				comp->resourceId = CollisionMgr::nullres;
			}

			for(auto& handle: comp->shapes)
			{
				if( handle.stoarge == CollisionStorageType::RESOURCE )
					handle.shape = nullptr;
				else
					_DELETE(handle.shape);

				handle.proxy = nullptr;
			}
			comp->shapes.destroy();
			
		#ifdef _DEV
			_RELEASE(comp->constantBuffer);
			MATERIAL_PTR_DROP(comp->material);
		#endif
		}

		ComponentRArray<CollisionComponent> components;
				
		BaseWorld* world;
		TransformSystem* transformSystem;
		PhysicsSystem* physicsSystem;

		rp3d::CollisionWorld* collisionWorld;

		bool b_debugDraw;
	};
}