#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"

#define PHYSICS_TIME_STEP_MS 1000.0f / 60.0f

namespace EngineCore
{
	struct PhysicsComponent
	{
		ENTITY_IN_COMPONENT
		
		bool dirty;

		rp3d::RigidBody* body;
		rp3d::Transform previousTransform;

		bool overwriteMass;
		bool overwriteCenterOfMass;

		PhysicsComponent()
		{
			body = nullptr;
			dirty = false;
			overwriteMass = false;
			overwriteCenterOfMass = false;
		}
	};

	class BaseWorld;

	class PhysicsSystem
	{
	public:
		PhysicsSystem(BaseWorld* w, rp3d::DynamicsWorld* dynamicsW, uint32_t maxCount);
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
		bool IsGravityEnabled(Entity e);
		void SetGravityEnabled(Entity e, bool enabled);

		void SetType(Entity e, int32_t type);
		int32_t GetType(Entity e);
		bool GetNonRotatable(Entity e);
		void SetNonRotatable(Entity e, bool isNonRot);

		float GetBounciness(Entity e);
		void SetBounciness(Entity e, float bounciness);
		float GetFriction(Entity e);
		void SetFriction(Entity e, float friction);
		float GetRollingResistance(Entity e);
		void SetRollingResistance(Entity e, float resistance);
		float GetVelocityDamping(Entity e);
		void SetVelocityDamping(Entity e, float damping);
		float GetAngularDamping(Entity e);
		void SetAngularDamping(Entity e, float damping);

		float GetMass(Entity e);
		void SetMass(Entity e, float mass);
		Vector3 GetCenterOfMass(Entity e);
		void SetCenterOfMass(Entity e, Vector3 local_point);

		Vector3 GetVelocity(Entity e);
		void SetVelocity(Entity e, Vector3 velocity);
		Vector3 GetAngularVelocity(Entity e);
		void SetAngularVelocity(Entity e, Vector3 velocity);

		void ApplyForce(Entity e, Vector3 point, Vector3 force);
		void ApplyForceToCenterOfMass(Entity e, Vector3 force);
		void ApplyTorque(Entity e, Vector3 torque);

		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass();

	private:

		inline void _DeleteComponent(PhysicsComponent* comp) // TODO: crash after deletion
		{
			dynamicsWorld->destroyRigidBody(comp->body);
			comp->body = nullptr;
		}

	private:
		ComponentRArray<PhysicsComponent> components;
				
		BaseWorld* world;
		TransformSystem* transformSystem;

		rp3d::DynamicsWorld* dynamicsWorld;
		float updateAccum;
		float interpolationFactor;
	};
}