#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "CollisionMgr.h"

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
	
	enum CollisionGroups
	{
		None = 0,
		Default = 1,
		Static = 2,
		Kinematic = 4,
		Debris = 8,
		Trigger = 16,
		Character = 32,
		Gamelogic = 64,

		Special0 = 128,
		Special1 = 256,
		Special2 = 512,
		Special3 = 1024,
		Special4 = 2048,
		Special5 = 4096,

		Physics = Default | Static | Kinematic | Debris | Character | Trigger,
		All = -1, //all bits sets
		AllNoSpecial = All & ~(Special0 | Special1 | Special2 | Special3 | Special4 | Special5),
	};

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
			collisionGroup = CollisionGroups::None;
			collisionMask = CollisionGroups::None;
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

		void UpdateState(Entity e);

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

		//bool IsActive(Entity e);
		//void SetActive(Entity e, bool active);

		int32_t GetCollisionGroup(Entity e);
		void SetCollisionGroup(Entity e, int32_t group);
		bool HasCollisionMask(Entity e, int32_t group);
		void AddCollisionMask(Entity e, int32_t group);
		void RemoveCollisionMask(Entity e, int32_t group);

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