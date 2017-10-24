#include "stdafx.h"
#include "CollisionSystem.h"
#include "WorldMgr.h"
#include "World.h"

CollisionSystem::CollisionSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();

	dynamicsWorld = dynamicsW;
	
	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
	
	debugDraw = false;
}

CollisionSystem::~CollisionSystem()
{
	for(auto& i: *components.data())
	{
		_DeleteComponent(&i);
	}
}

void CollisionSystem::UpdateTransformations()
{
	for(auto& i: *components.data())
	{
		if(!i.dirty)
			continue;

		if(i.object)
		{
			Entity e = i.get_entity();
		
			btTransform transform = ToBtTransform( transformSystem->GetTransformW(e) );
			i.object->setWorldTransform( transform );
		}

		i.dirty = false;
	}
}

CollisionComponent* CollisionSystem::AddComponent(Entity e, bool dummy)
{
	if(HasComponent(e))
		return &components.getDataById(e.index());
	
	CollisionComponent* res = components.add(e.index());
	res->parent = e;
	res->dirty = true;
	res->collisionData = 0;
	res->collisionStorage = LOCAL;
	
	if(dummy)
	{
		res->object = new btCollisionObject();
		res->object->setUserIndex(IntFromEntity(e));
		res->object->setCollisionShape(CollisionMgr::GetResourcePtr(CollisionMgr::nullres));
		res->object->setCollisionFlags(res->object->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

		dynamicsWorld->addCollisionObject(res->object);
	}	

	return res;
}

void CollisionSystem::DeleteComponent(Entity e)
{
	auto comp = GetComponent(e);
	if(!comp) 
		return;

	_DeleteComponent(comp);
	components.remove(e.index());
}

void CollisionSystem::_DeleteComponent(CollisionComponent* comp)
{
	if(comp->object)
		dynamicsWorld->removeCollisionObject(comp->object);
	_DELETE(comp->object);

	_ClearCollision(comp);
}

void CollisionSystem::_ClearCollision(CollisionComponent* comp)
{
	if( comp->collisionStorage == CollisionStorageType::LOCAL )
	{
		if( comp->collisionData != 0 )
		{
			auto shape = (btCollisionShape*)comp->collisionData;
			CollisionMgr::Get()->ResourceDeallocate( shape );
			comp->collisionData = 0;
		}
	}
	else
	{
		CollisionMgr::Get()->DeleteResource((uint32_t)comp->collisionData);
		comp->collisionData = (uint64_t)CollisionMgr::nullres;
	}

	if(comp->object)
		comp->object->setCollisionShape(CollisionMgr::GetResourcePtr(CollisionMgr::nullres));
}

void CollisionSystem::CopyComponent(Entity src, Entity dest)
{
	auto copyBuffer = world->GetCopyBuffer();

	if( !Serialize(src, copyBuffer) )
		return;

	Deserialize(dest, copyBuffer);
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool CollisionSystem::IsDummy(Entity e)
{
	GET_COMPONENT(false);
	return comp.object != nullptr;
}

bool CollisionSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false);
	return comp.dirty;
}

bool CollisionSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false);
	comp.dirty = true;
	return true;
}

uint32_t CollisionSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)
	
	uint8_t* t_data = data;
	
	*(bool*)t_data = (comp.object != nullptr);
	t_data += sizeof(bool);
	
	*(CollisionStorageType*)t_data = comp.collisionStorage;
	t_data += sizeof(CollisionStorageType);

	if( comp.collisionStorage == CollisionStorageType::LOCAL )
	{
		uint32_t childrenCount = 0;
		btCompoundShapeChild* childrenPtrs = nullptr;

		if( comp.collisionData != 0 )
		{
			childrenCount = ((btCompoundShape*)comp.collisionData)->getNumChildShapes();
			childrenPtrs = ((btCompoundShape*)comp.collisionData)->getChildList();
		}

		*(uint32_t*)t_data = childrenCount;
		t_data += sizeof(uint32_t);

		while( childrenCount > 0 )
		{
			*(int32_t*)t_data = childrenPtrs->m_childShapeType;
			t_data += sizeof(int32_t);

			*(Vector3*)t_data = childrenPtrs->m_transform.getOrigin();
			t_data += sizeof(Vector3);

			*(Quaternion*)t_data = childrenPtrs->m_transform.getRotation();
			t_data += sizeof(Quaternion);

			switch (childrenPtrs->m_childShapeType)
			{
			case BroadphaseNativeTypes::BOX_SHAPE_PROXYTYPE:
				{
					btBoxShape* shape = (btBoxShape*)childrenPtrs->m_childShape;

					*(Vector3*)t_data = shape->getHalfExtentsWithMargin();
					t_data += sizeof(Vector3);
				}
				break;
			case BroadphaseNativeTypes::SPHERE_SHAPE_PROXYTYPE:
				{
					btSphereShape* shape = (btSphereShape*)childrenPtrs->m_childShape;

					*(float*)t_data = (float)shape->getRadius();
					t_data += sizeof(float);
				}
				break;
			case BroadphaseNativeTypes::CONE_SHAPE_PROXYTYPE:
				{
					btConeShape* shape = (btConeShape*)childrenPtrs->m_childShape;

					*(float*)t_data = (float)shape->getRadius();
					t_data += sizeof(float);
					*(float*)t_data = (float)shape->getHeight();
					t_data += sizeof(float);
				}
				break;
			case BroadphaseNativeTypes::CYLINDER_SHAPE_PROXYTYPE:
				{
					btCylinderShape* shape = (btCylinderShape*)childrenPtrs->m_childShape;

					*(Vector3*)t_data = shape->getHalfExtentsWithMargin();
					t_data += sizeof(Vector3);
				}
				break;
			case BroadphaseNativeTypes::CAPSULE_SHAPE_PROXYTYPE:
				{
					btCapsuleShape* shape = (btCapsuleShape*)childrenPtrs->m_childShape;

					*(float*)t_data = (float)shape->getRadius();
					t_data += sizeof(float);
					*(float*)t_data = (float)shape->getHalfHeight() * 2.0f;
					t_data += sizeof(float);
				}
				break;
			default:
				ERR("Wrong type of collision shape: %i", childrenPtrs->m_childShapeType);
				break;
			}

			childrenPtrs++;
			childrenCount--;
		}
	}
	else
	{
		string collision_name = CollisionMgr::GetName((uint32_t)comp.collisionData);
		uint32_t collision_name_size = (uint32_t)collision_name.size();

		*(uint32_t*)t_data = collision_name_size;
		t_data += sizeof(uint32_t);

		if( collision_name_size != 0 )
		{
			memcpy_s(t_data, collision_name_size, collision_name.data(), collision_name_size);
			t_data += collision_name_size * sizeof(char);
		}
	}

	return (uint32_t)(t_data - data);
}

uint32_t CollisionSystem::Deserialize(Entity e, uint8_t* data)
{
	if( HasComponent(e) )
		return 0;

	uint8_t* t_data = data;

	bool isDummy = *(bool*)t_data;
	t_data += sizeof(bool);

	auto comp = AddComponent(e, isDummy);
	if(!comp)
		return 0;
	
	comp->collisionStorage = *(CollisionStorageType*)t_data;
	t_data += sizeof(CollisionStorageType);

	if( comp->collisionStorage == CollisionStorageType::LOCAL )
	{
		auto childrenCount = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		btCompoundShape* shape = new btCompoundShape(true, childrenCount);
		comp->collisionData = (uint64_t)shape;

		while( childrenCount > 0 )
		{
			int32_t type = *(int32_t*)t_data;
			t_data += sizeof(int32_t);

			Vector3 pos = *(Vector3*)t_data;
			t_data += sizeof(Vector3);

			Quaternion rot = *(Quaternion*)t_data;
			t_data += sizeof(Quaternion);

			btTransform transform(rot, pos);

			switch (type)
			{
			case BroadphaseNativeTypes::BOX_SHAPE_PROXYTYPE:
				{
					shape->addChildShape(transform, new btBoxShape(*(Vector3*)t_data));
					t_data += sizeof(Vector3);
				}
				break;
			case BroadphaseNativeTypes::SPHERE_SHAPE_PROXYTYPE:
				{
					shape->addChildShape(transform, new btSphereShape(*(float*)t_data));
					t_data += sizeof(float);
				}
				break;
			case BroadphaseNativeTypes::CONE_SHAPE_PROXYTYPE:
				{
					auto radius = *(float*)t_data;
					t_data += sizeof(float);
					auto height = *(float*)t_data;
					t_data += sizeof(float);

					shape->addChildShape(transform, new btConeShape(radius, height));
				}
				break;
			case BroadphaseNativeTypes::CYLINDER_SHAPE_PROXYTYPE:
				{
					shape->addChildShape(transform, new btCylinderShape(*(Vector3*)t_data));
					t_data += sizeof(Vector3);
				}
				break;
			case BroadphaseNativeTypes::CAPSULE_SHAPE_PROXYTYPE:
				{
					auto radius = *(float*)t_data;
					t_data += sizeof(float);
					auto height = *(float*)t_data;
					t_data += sizeof(float);

					shape->addChildShape(transform, new btCapsuleShape(radius, height));
				}
				break;
			default:
				ERR("Wrong type of collision shape: %i", type);
				break;
			}
			
			childrenCount--;
		}

		if(comp->object)
			comp->object->setCollisionShape(shape);
	}
	else
	{
		uint32_t collision_name_size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		string collision_name((char*)t_data, collision_name_size);
		t_data += collision_name_size * sizeof(char);

		if( !collision_name.empty() )
			_SetCollisionConvex(e, comp, collision_name);
	}

	return (uint32_t)(t_data - data);
}

// PARAMS

bool CollisionSystem::IsEnable(Entity e)
{
	GET_COMPONENT(false);
	// TODO
	return true;
}

void CollisionSystem::SetEnable(Entity e, bool enable)
{
	GET_COMPONENT(void());
	// TODO
}

RayCastResult CollisionSystem::RayCast(Vector3& start, Vector3& end)
{
	btCollisionWorld::ClosestRayResultCallback rayCallback(start, end);
	dynamicsWorld->rayTest(start, end, rayCallback);

	RayCastResult result;
	result.hit = rayCallback.hasHit();
	if(result.hit)
	{
		result.position = rayCallback.m_hitPointWorld;
		result.normal = rayCallback.m_hitNormalWorld;
		result.entity = EntityFromInt(rayCallback.m_collisionObject->getUserIndex());
	}
	return result;
}

// COLLIDERS

void CollisionSystem::AddBoxCollider(Entity e, Vector3& pos, Quaternion& rot, Vector3& halfExtents)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btBoxShape(halfExtents));
}

void CollisionSystem::AddSphereCollider(Entity e, Vector3& pos, Quaternion& rot, float radius)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btSphereShape(radius));
}

void CollisionSystem::AddConeCollider(Entity e, Vector3& pos, Quaternion& rot, float radius, float height)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btConeShape(radius, height));
}

void CollisionSystem::AddCylinderCollider(Entity e, Vector3& pos, Quaternion& rot, float radius, float height)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btCylinderShape( btVector3( radius, 0.5f * height, radius) ));
}

void CollisionSystem::AddCapsuleCollider(Entity e, Vector3& pos, Quaternion& rot, float radius, float height)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btCapsuleShape(radius, height));
}

void CollisionSystem::ClearCollision(Entity e)
{
	GET_COMPONENT(void());
	_ClearCollision(&comp);
}

btCollisionShape* CollisionSystem::GetCollision(Entity e)
{
	GET_COMPONENT(nullptr);
	if( comp.collisionStorage == LOCAL )
		return (btCollisionShape*)comp.collisionData;
	else
		return CollisionMgr::GetResourcePtr((uint32_t)comp.collisionData);
}

void CollisionSystem::_AddCollisionShape(CollisionComponent& comp, Vector3& pos, Quaternion& rot, btCollisionShape* shape)
{
	if( comp.collisionStorage != LOCAL )
	{
		WRN("Shape collision can\'t be added to mesh collision");
	}
	else
	{
		if(!comp.collisionData)
			comp.collisionData = (uint64_t)(new btCompoundShape());

		btCompoundShape* compound = (btCompoundShape*)comp.collisionData;
		compound->addChildShape(btTransform(rot, pos), shape);

		if(comp.object)
			comp.object->setCollisionShape(compound);
	}
}

void CollisionSystem::_SetCollisionConvex(Entity e, CollisionComponent* comp, string& name)
{
	auto worldID = world->GetID();
	comp->collisionData = (uint64_t)CollisionMgr::Get()->GetResource(name, false, 
		[e, worldID](uint32_t id, bool status) -> void
	{
		auto collisionPtr = CollisionMgr::GetResourcePtr(id);
		if(!collisionPtr)
			return;

		auto worldPtr = WorldMgr::Get()->GetWorld(worldID);
		if(!worldPtr || !worldPtr->IsEntityAlive(e))
		{
			CollisionMgr::Get()->DeleteResource(id);
			return;
		}

		auto compPtr = worldPtr->GetCollisionSystem()->GetComponent(e);
		auto compTrigPtr = worldPtr->GetTriggerSystem()->GetComponent(e);
		auto compPhysPtr = worldPtr->GetPhysicsSystem()->GetComponent(e);
		if(!compPtr)
			CollisionMgr::Get()->DeleteResource(id);
		else if(compPtr->object)
			compPtr->object->setCollisionShape(collisionPtr);
		else if(compTrigPtr)
			compTrigPtr->object->setCollisionShape(collisionPtr);
		else if(compPhysPtr)
			compPhysPtr->body->setCollisionShape(collisionPtr);
	});
}

void CollisionSystem::SetConvexHullsCollider(Entity e, string collisionName)
{
	GET_COMPONENT(void());
	_ClearCollision(&comp);
	
	comp.collisionStorage = CollisionStorageType::RESOURCE;
	_SetCollisionConvex(e, &comp, collisionName);
}

void CollisionSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<RayCastResult>("RayCastResult")
		.addData("position", &RayCastResult::position)
		.addData("normal", &RayCastResult::normal)
		.addData("hit", &RayCastResult::hit)
		.addData("entity", &RayCastResult::entity)
		.endClass()

		.beginClass<CollisionSystem>("CollisionSystem")

		.addFunction("IsDummy", &CollisionSystem::IsDummy)
		.addFunction("IsEnable", &CollisionSystem::IsEnable)
		.addFunction("SetEnable", &CollisionSystem::SetEnable)

		.addFunction("AddBoxCollider", &CollisionSystem::AddBoxCollider)
		.addFunction("AddSphereCollider", &CollisionSystem::AddSphereCollider)
		.addFunction("AddConeCollider", &CollisionSystem::AddConeCollider)
		.addFunction("AddCylinderCollider", &CollisionSystem::AddCylinderCollider)
		.addFunction("AddCapsuleCollider", &CollisionSystem::AddCapsuleCollider)

		.addFunction("SetConvexHullsCollider", &CollisionSystem::SetConvexHullsCollider)

		.addFunction("ClearCollision", &CollisionSystem::ClearCollision)

		.addFunction("RayCast", &CollisionSystem::RayCast)
		
		.addFunction("AddComponent", &CollisionSystem::_AddComponent)
		.addFunction("DeleteComponent", &CollisionSystem::DeleteComponent)
		.addFunction("HasComponent", &CollisionSystem::HasComponent)

		.addFunction("SetDebugDraw", &CollisionSystem::SetDebugDraw)
		.endClass();
}

// DEBUG DRAW

CollisionDebugDrawer::CollisionDebugDrawer(DebugDrawer* dbgDrawer) :
	m_debugMode(btIDebugDraw::DBG_DrawWireframe),
	m_dbgDrawer(dbgDrawer)
{
	m_colors.m_activeObject = btVector3(0.1f, 0.95f, 0.1f);
	m_colors.m_deactivatedObject = btVector3(0.1f, 0.1f, 0.95f);
}

void CollisionDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor)
{
	Vector3 A(from);
	Vector3 B(to);
	Vector3 colorA(fromColor);
	Vector3 colorB(toColor);
	m_dbgDrawer->PushLine(A, B, colorA, colorB);
}