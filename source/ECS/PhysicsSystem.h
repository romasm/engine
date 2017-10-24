#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "CollisionMgr.h"
#include "CollisionSystem.h"

#define MAX_PHYSICS_STEP_PER_FRAME 10

#define SLEEP_THRESHOLD_LINEAR 0.6f
#define SLEEP_THRESHOLD_ANGULAR 0.5f

namespace EngineCore
{
	struct PhysicsComponent
	{
		ENTITY_IN_COMPONENT
		
		bool dirty;

		btRigidBody* body;
		
		PhysicsComponent()
		{
			dirty = false;
			body = nullptr;
		}
	};

	class BaseWorld;

	class PhysicsSystem
	{
	public:
		PhysicsSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount);
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
		
		void SimulateAndUpdateSceneGraph(float dt);
		void UpdateTransformations();

		void UpdateState(Entity e);

		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
		
		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool IsActive(Entity e);
		bool IsEnable(Entity e);
		void SetEnable(Entity e, bool enable, bool nonSleeping);
		
		void SetType(Entity e, int32_t type);
		int32_t GetType(Entity e);

		float GetRestitution(Entity e);
		void SetRestitution(Entity e, float restitution);
		float GetFriction(Entity e);
		void SetFriction(Entity e, float friction);
		float GetRollingFriction(Entity e);
		void SetRollingFriction(Entity e, float friction);
		float GetSpinningFriction(Entity e);
		void SetSpinningFriction(Entity e, float friction);

		float GetContactStiffness(Entity e);
		void SetContactStiffness(Entity e, float stiffness);
		float GetContactDamping(Entity e);
		void SetContactDamping(Entity e, float damping);

		float GetLinearDamping(Entity e);
		void SetLinearDamping(Entity e, float damping);
		float GetAngularDamping(Entity e);
		void SetAngularDamping(Entity e, float damping);

		Vector3 GetLinearFactor(Entity e);
		void SetLinearFactor(Entity e, Vector3& factor);
		Vector3 GetAngularFactor(Entity e);
		void SetAngularFactor(Entity e, Vector3& factor);

		float GetMass(Entity e);
		void SetMass(Entity e, float mass);

		Vector3 GetLinearVelocity(Entity e);
		void SetLinearVelocity(Entity e, Vector3& velocity);
		Vector3 GetAngularVelocity(Entity e);
		void SetAngularVelocity(Entity e, Vector3& velocity);

		void ApplyForce(Entity e, Vector3& point, Vector3& force);
		void ApplyCentralForce(Entity e, Vector3& force);
		void ApplyImpulse(Entity e, Vector3& point, Vector3& impulse);
		void ApplyCentralImpulse(Entity e, Vector3& impulse);
		void ApplyTorque(Entity e, Vector3& torque);
		void ApplyTorqueImpulse(Entity e, Vector3& torque);

		Vector3 GetTotalForce(Entity e);
		Vector3 GetTotalTorque(Entity e);
		void ClearForces(Entity e);

		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass();

	private:
		struct PhysicsData 
		{
			int32_t state;
			int32_t flags;
			float restitution;
			float friction;
			float rollFriction;
			float spinFriction;
			float contactStiffness;
			float contactDamp;
			float linDamp;
			float angDamp;
			Vector3 linFactor;
			Vector3 angFactor;
			float mass;
			Vector3 localInertia;
		};
		
		void _DeleteComponent(PhysicsComponent* comp);

		ComponentRArray<PhysicsComponent> components;
		
		BaseWorld* world;
		TransformSystem* transformSystem;
		CollisionSystem* collisionSystem;

		btDiscreteDynamicsWorld* dynamicsWorld;
	};
}