#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"

#define PHYSICS_TIME_STEP_MS 1000.0f / 60.0f

namespace EngineCore
{
	struct CollisionHandle
	{
		rp3d::CollisionShape* shape;
		rp3d::ProxyShape* proxy;

		CollisionHandle() : shape(nullptr), proxy(nullptr) {}
		CollisionHandle(rp3d::CollisionShape* s, rp3d::ProxyShape* p) 
			: shape(s), proxy(p) {}
	};

	struct PhysicsComponent
	{
		ENTITY_IN_COMPONENT
		
		bool dirty;

		rp3d::RigidBody* body;
		rp3d::Transform previousTransform;
		
		// TODO: use resource IDs
		DArray<CollisionHandle> shapes;

		PhysicsComponent()
		{
			body = nullptr;
			dirty = false;
		}
	};

	class BaseWorld;

	class PhysicsSystem
	{
	public:
		PhysicsSystem(BaseWorld* w, uint32_t maxCount);
		~PhysicsSystem();

		PhysicsComponent* AddComponent(Entity e);

		void CopyComponent(Entity src, Entity dest);
		void DeleteComponent(Entity e);
		bool HasComponent(Entity e) const { return components.has(e.index()); }
		size_t ComponentsCount() {return components.dataSize();}

		inline PhysicsComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		void Simulate(float dt);
		void UpdateTransformations();
		void UpdateSceneGraph();

		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
				
		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool IsActive(Entity e);
		void SetActive(Entity e, bool active);

		bool IsSleeping(Entity e);
		void SetSleeping(Entity e, bool sleep);

		bool IsUnsleepable(Entity e);
		void SetUnsleepable(Entity e, bool unsleepable);

		void SetType(Entity e, int32_t type);
		int32_t GetType(Entity e);

		bool GetNonRotatable(Entity e);
		void SetNonRotatable(Entity e, bool isNonRot);

		int32_t AddBoxShape(Entity e, Vector3 pos, Quaternion rot, float mass, Vector3 halfSize);
		int32_t AddSphereShape(Entity e, Vector3 pos, Quaternion rot, float mass, float radius);
		int32_t AddConeShape(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height);
		int32_t AddCylinderShape(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height);
		int32_t AddCapsuleShape(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height);

		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<PhysicsSystem>("PhysicsSystem")
					.addFunction("SetType", &PhysicsSystem::SetType)
					.addFunction("GetType", &PhysicsSystem::GetType)
					.addFunction("GetNonRotatable", &PhysicsSystem::GetNonRotatable)
					.addFunction("SetNonRotatable", &PhysicsSystem::SetNonRotatable)
					.addFunction("IsActive", &PhysicsSystem::IsActive)
					.addFunction("SetActive", &PhysicsSystem::SetActive)
					.addFunction("IsSleeping", &PhysicsSystem::IsSleeping)
					.addFunction("SetSleeping", &PhysicsSystem::SetSleeping)
					.addFunction("IsUnsleepable", &PhysicsSystem::IsUnsleepable)
					.addFunction("SetUnsleepable", &PhysicsSystem::SetUnsleepable)

					.addFunction("AddComponent", &PhysicsSystem::_AddComponent)
					.addFunction("DeleteComponent", &PhysicsSystem::DeleteComponent)
					.addFunction("HasComponent", &PhysicsSystem::HasComponent)

					.addFunction("AddBoxShape", &PhysicsSystem::AddBoxShape)
					.addFunction("AddSphereShape", &PhysicsSystem::AddSphereShape)
					.addFunction("AddConeShape", &PhysicsSystem::AddConeShape)
					.addFunction("AddCylinderShape", &PhysicsSystem::AddCylinderShape)
					.addFunction("AddCapsuleShape", &PhysicsSystem::AddCapsuleShape)
				.endClass();
		}

	private:
		inline int32_t AddShape(PhysicsComponent& comp, Vector3& pos, Quaternion& rot, 
			float& mass, rp3d::CollisionShape* shape)
		{
			rp3d::Transform shapeTransform(pos, rot);
			auto proxy = comp.body->addCollisionShape(shape, shapeTransform, mass);
			comp.shapes.push_back(CollisionHandle(shape, proxy));
			return comp.shapes.size() - 1;
		}

	private:
		ComponentRArray<PhysicsComponent> components;
				
		BaseWorld* world;
		TransformSystem* transformSystem;

		rp3d::DynamicsWorld* physWorld;
		float updateAccum;
		float interpolationFactor;

		// temp
		rp3d::RigidBody* bodyFloor;
		rp3d::BoxShape* shapeFloor;
	};
}