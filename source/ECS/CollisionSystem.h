#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "CollisionMgr.h"

#define MAX_PHYSICS_STEP_PER_FRAME 10

#define SLEEP_THRESHOLD_LINEAR 0.6f
#define SLEEP_THRESHOLD_ANGULAR 0.5f

namespace EngineCore
{
	struct RayCastResult
	{
		Vector3 position;
		Vector3 normal;
		bool hit;
		Entity entity;

		RayCastResult() : hit(false) { entity.setnull(); }
	};
	
	/*enum CollisionFilterGroups
	{
		DefaultFilter = 1,
		StaticFilter = 2,
		KinematicFilter = 4,
		DebrisFilter = 8,
		SensorTrigger = 16,
		CharacterFilter = 32,
		AllFilter = -1 //all bits sets: DefaultFilter | StaticFilter | KinematicFilter | DebrisFilter | SensorTrigger
	};*/

	struct CollisionComponent
	{
		ENTITY_IN_COMPONENT
		
		bool dirty;

		int32_t collisionGroup;
		int32_t collisionMask;

		btCollisionObject* object;

		uint64_t collisionData;		
		CollisionStorageType collisionStorage;

		CollisionComponent()
		{
			dirty = false;
			object = nullptr;
			collisionData = 0;
			collisionStorage = LOCAL;
			collisionGroup = 1;
			collisionMask = -1;
		}
	};

	class BaseWorld;

	class CollisionSystem
	{
	public:
		CollisionSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount);
		~CollisionSystem();

		CollisionComponent* AddComponent(Entity e, bool dummy);

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
		
		btCollisionShape* GetCollision(Entity e);

		void UpdateTransformations();

		void DebugDraw()
		{
			if(debugDraw)
				dynamicsWorld->debugDrawWorld();
		}
		
		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);

		bool IsDummy(Entity e);

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool IsEnable(Entity e);
		void SetEnable(Entity e, bool enable);
		
		void AddBoxCollider(Entity e, Vector3& pos, Quaternion& rot, Vector3& halfExtents);
		void AddSphereCollider(Entity e, Vector3& pos, Quaternion& rot, float radius);
		void AddConeCollider(Entity e, Vector3& pos, Quaternion& rot, float radius, float height);
		void AddCylinderCollider(Entity e, Vector3& pos, Quaternion& rot, float radius, float height);
		void AddCapsuleCollider(Entity e, Vector3& pos, Quaternion& rot, float radius, float height);
		
		void SetConvexHullsCollider(Entity e, string collisionName);

		void ClearCollision(Entity e);

		RayCastResult RayCast(Vector3& start, Vector3& end);

		void SetDebugDraw(bool draw)
		{
			debugDraw = draw;
		}

		inline void _AddComponent(Entity e, bool dummy) {AddComponent(e, dummy);}

		static void RegLuaClass();

	private:		
		void _DeleteComponent(CollisionComponent* comp);
		void _AddCollisionShape(CollisionComponent& comp, Vector3& pos, Quaternion& rot, btCollisionShape* shape);
		void _SetCollisionConvex(Entity e, CollisionComponent* comp, string& name);
		void _ClearCollision(CollisionComponent* comp);

		ComponentRArray<CollisionComponent> components;
		
		BaseWorld* world;
		TransformSystem* transformSystem;

		btDiscreteDynamicsWorld* dynamicsWorld;

		bool debugDraw;
	};

	class DebugDrawer;

	class CollisionDebugDrawer : public btIDebugDraw
	{
	public:
		CollisionDebugDrawer(DebugDrawer* dbgDrawer);
		
		virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor);

		virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
		{
			drawLine(from, to, color, color);
		}

		virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
		{
			drawLine(PointOnB, PointOnB + normalOnB * distance, color);
			btVector3 ncolor(0, 0, 0);
			drawLine(PointOnB, PointOnB + normalOnB * 0.01f, ncolor);
		}

		virtual void reportErrorWarning(const char* warningString)
		{
			WRN("[BulletPhysics] %s", warningString);
		}

		virtual void draw3dText(const btVector3& location,const char* textString)
		{
		}

		virtual DefaultColors getDefaultColors() const {return m_colors;}

		virtual int	getDebugMode() const {return m_debugMode;}
		virtual void setDebugMode(int debugMode) {m_debugMode = debugMode;}

	private:
		int m_debugMode;
		DebugDrawer* m_dbgDrawer;
		DefaultColors m_colors;
	};
}