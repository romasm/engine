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
		
		rp3d::BoxShape* testBox;

		PhysicsComponent()
		{
			body = nullptr;
			dirty = false;

			testBox = nullptr;
		}
	};

	class BaseWorld;

	class PhysicsSystem
	{
	public:
		PhysicsSystem(BaseWorld* w, uint32_t maxCount);
		~PhysicsSystem();

		PhysicsComponent* AddComponent(Entity e);

		void CopyComponent(Entity src, Entity dest)
		{
			auto comp = GetComponent(src);
			if(!comp || HasComponent(dest)) 
				return;

			PhysicsComponent* res = AddComponent(dest);
			if(!res)
				return;
			
			// TODO
		}
		void DeleteComponent(Entity e)
		{
			auto& comp = components.getDataById(e.index());
			physWorld->destroyRigidBody(comp.body);
			comp.body = nullptr;

			_DELETE(comp.testBox);

			components.remove(e.index());
		}
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

		void SetType(Entity e, int32_t type);
		int32_t GetType(Entity e);

		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<PhysicsSystem>("PhysicsSystem")
					.addFunction("SetType", &PhysicsSystem::SetType)
					.addFunction("GetType", &PhysicsSystem::GetType)

					.addFunction("AddComponent", &PhysicsSystem::_AddComponent)
					.addFunction("DeleteComponent", &PhysicsSystem::DeleteComponent)
					.addFunction("HasComponent", &PhysicsSystem::HasComponent)
				.endClass();
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