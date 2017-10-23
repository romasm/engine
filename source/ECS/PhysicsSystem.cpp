#include "stdafx.h"
#include "PhysicsSystem.h"
#include "WorldMgr.h"
#include "World.h"

PhysicsSystem::PhysicsSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();

	dynamicsWorld = dynamicsW;
	
	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
	
	debugDraw = false;
}

PhysicsSystem::~PhysicsSystem()
{
	for(auto& i: *components.data())
	{
		_DeleteComponent(&i);
	}
}

void PhysicsSystem::UpdateTransformations()
{
	for(auto& i: *components.data())
	{
		if(!i.dirty)
			continue;

		Entity e = i.get_entity();
		
		btTransform transform = ToTransform( transformSystem->GetTransformW(e) );
		//transform.setOrigin( transform.getOrigin() + btVector3(i.centerOfMassOffset) );
		i.body->proceedToTransform( transform );
		
		if( i.body->wantsSleeping() )
			i.body->activate( true );

		i.dirty = false;
	}
}

void PhysicsSystem::SimulateAndUpdateSceneGraph(float dt)
{
	dynamicsWorld->stepSimulation( dt * 0.001f, MAX_PHYSICS_STEP_PER_FRAME );

	for(auto& i: *components.data())
	{
		if( !i.body->isActive() || i.body->isStaticOrKinematicObject() )
			continue;

		btTransform transform = dynamicsWorld->getInterpolatedWorldTransform( i.body );
		//transform.setOrigin( transform.getOrigin() - btVector3(i.centerOfMassOffset) );
		transformSystem->SetPhysicsTransform(i.get_entity(), ToXMMATRIX(transform));

		i.dirty = false;
	}
}

PhysicsComponent* PhysicsSystem::AddComponent(Entity e)
{
	if(HasComponent(e))
		return &components.getDataById(e.index());

	if( !transformSystem->GetParent(e).isnull() )
		return nullptr;

	PhysicsComponent* res = components.add(e.index());
	res->parent = e;
	res->dirty = true;
	res->collisionData = 0;
	res->collisionStorage = LOCAL;
	
	btRigidBody::btRigidBodyConstructionInfo info(0, nullptr, CollisionMgr::GetResourcePtr(CollisionMgr::nullres));
	info.m_linearSleepingThreshold = SLEEP_THRESHOLD_LINEAR;
	info.m_angularSleepingThreshold = SLEEP_THRESHOLD_ANGULAR;

	res->body = new btRigidBody(info);
	res->body->setUserIndex(IntFromEntity(e));
	res->body->setCollisionFlags( res->body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);

	dynamicsWorld->addRigidBody(res->body);

	return res;
}

void PhysicsSystem::DeleteComponent(Entity e)
{
	auto comp = GetComponent(e);
	if(!comp) 
		return;

	_DeleteComponent(comp);
	components.remove(e.index());
}

void PhysicsSystem::_DeleteComponent(PhysicsComponent* comp)
{
	dynamicsWorld->removeRigidBody(comp->body);
	_DELETE(comp->body);

	_ClearCollision(comp);
}

void PhysicsSystem::_ClearCollision(PhysicsComponent* comp)
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

	if(comp->body)
		comp->body->setCollisionShape(CollisionMgr::GetResourcePtr(CollisionMgr::nullres));
}

void PhysicsSystem::CopyComponent(Entity src, Entity dest)
{
	auto copyBuffer = world->GetCopyBuffer();

	if( !Serialize(src, copyBuffer) )
		return;

	Deserialize(dest, copyBuffer);
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool PhysicsSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false);
	return comp.dirty;
}

bool PhysicsSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false);
	comp.dirty = true;
	return true;
}

uint32_t PhysicsSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)
	
	uint8_t* t_data = data;
	
	PhysicsData pdata;
	pdata.state = comp.body->getActivationState();
	if( pdata.state <= WANTS_DEACTIVATION )
		pdata.state = ACTIVE_TAG;
	pdata.flags = comp.body->getCollisionFlags();
	pdata.restitution = comp.body->getRestitution();
	pdata.friction = comp.body->getFriction();
	pdata.rollFriction = comp.body->getRollingFriction();
	pdata.spinFriction = comp.body->getSpinningFriction();
	pdata.contactStiffness = comp.body->getContactStiffness();
	pdata.contactDamp = comp.body->getContactDamping();
	pdata.linDamp = comp.body->getLinearDamping();
	pdata.angDamp = comp.body->getAngularDamping();
	pdata.linFactor = comp.body->getLinearFactor();
	pdata.angFactor = comp.body->getAngularFactor();
	float invMass = comp.body->getInvMass();
	pdata.mass = invMass == 0 ? 0 : (1.0f / invMass);
	pdata.localInertia = comp.body->getLocalInertia();

	*(PhysicsData*)t_data = pdata;
	t_data += sizeof(PhysicsData);
	
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

uint32_t PhysicsSystem::Deserialize(Entity e, uint8_t* data)
{
	if( HasComponent(e) )
		return 0;

	auto comp = AddComponent(e);
	if(!comp)
		return 0;

	uint8_t* t_data = data;
	
	PhysicsData pdata = *(PhysicsData*)t_data;
	t_data += sizeof(PhysicsData);

	comp->body->setRestitution(pdata.restitution);
	comp->body->setFriction(pdata.friction);
	comp->body->setRollingFriction(pdata.rollFriction);
	comp->body->setSpinningFriction(pdata.spinFriction);
	comp->body->setContactStiffnessAndDamping(pdata.contactStiffness, pdata.contactDamp);
	comp->body->setDamping(pdata.linDamp, pdata.angDamp);
	comp->body->setLinearFactor(pdata.linFactor);
	comp->body->setAngularFactor(pdata.angFactor);
	comp->body->setMassProps(pdata.mass, pdata.localInertia);
	comp->body->setCollisionFlags(pdata.flags);
	comp->body->forceActivationState(pdata.state);
	
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

		comp->body->setCollisionShape(shape);
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
	
	UpdateState(e);

	return (uint32_t)(t_data - data);
}

// PARAMS

void PhysicsSystem::UpdateState(Entity e)
{
	GET_COMPONENT(void());
	// TODO: optimize? 
	// dynamicsWorld->resetRigidBody(comp.body)
	dynamicsWorld->removeRigidBody(comp.body);
	dynamicsWorld->addRigidBody(comp.body);
}

bool PhysicsSystem::IsActive(Entity e)
{
	GET_COMPONENT(false);
	return comp.body->isActive();
}

bool PhysicsSystem::IsEnable(Entity e)
{
	GET_COMPONENT(false);
	return comp.body->getActivationState() != DISABLE_SIMULATION;
}

void PhysicsSystem::SetEnable(Entity e, bool enable, bool nonSleeping)
{
	GET_COMPONENT(void());
	int32_t state = enable ? (nonSleeping ? DISABLE_DEACTIVATION : ACTIVE_TAG) : DISABLE_SIMULATION;
	comp.body->forceActivationState(state);
}

void PhysicsSystem::SetType(Entity e, int32_t type)
{
	GET_COMPONENT(void());
	auto flags = comp.body->getCollisionFlags();

	switch (type)
	{
	case 0:
		flags |= btCollisionObject::CF_STATIC_OBJECT;
		flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
		break;
	case 1:
		flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
		flags &= ~btCollisionObject::CF_STATIC_OBJECT;
		break;
	case 2:
		flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
		flags &= ~btCollisionObject::CF_STATIC_OBJECT;
		break;
	default:
		WRN("Wrong physics body type");
		break;
	}

	comp.body->setCollisionFlags(flags);
}

int32_t PhysicsSystem::GetType(Entity e)
{
	GET_COMPONENT(-1);
	auto flags = comp.body->getCollisionFlags();
	if( flags & btCollisionObject::CF_STATIC_OBJECT )
		return 0;
	else if( flags & btCollisionObject::CF_KINEMATIC_OBJECT )
		return 1;
	else
		return 2;

	return -1;
}

float PhysicsSystem::GetRestitution(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getRestitution();
}

void PhysicsSystem::SetRestitution(Entity e, float restitution)
{
	GET_COMPONENT(void());
	comp.body->setRestitution(restitution);
}

float PhysicsSystem::GetFriction(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getFriction();
}

void PhysicsSystem::SetFriction(Entity e, float friction)
{
	GET_COMPONENT(void());
	comp.body->setFriction(friction);
}

float PhysicsSystem::GetRollingFriction(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getRollingFriction();
}

void PhysicsSystem::SetRollingFriction(Entity e, float friction)
{
	GET_COMPONENT(void());
	comp.body->setRollingFriction(friction);
}

float PhysicsSystem::GetSpinningFriction(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getSpinningFriction();
}

void PhysicsSystem::SetSpinningFriction(Entity e, float friction)
{
	GET_COMPONENT(void());
	comp.body->setSpinningFriction(friction);
}

float PhysicsSystem::GetContactDamping(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getContactDamping();
}

void PhysicsSystem::SetContactDamping(Entity e, float damping)
{
	GET_COMPONENT(void());
	comp.body->setContactStiffnessAndDamping(comp.body->getContactStiffness(), damping);
}

float PhysicsSystem::GetContactStiffness(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getContactStiffness();
}

void PhysicsSystem::SetContactStiffness(Entity e, float stiffness)
{
	GET_COMPONENT(void());
	comp.body->setContactStiffnessAndDamping(stiffness, comp.body->getContactDamping());
}

float PhysicsSystem::GetLinearDamping(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getLinearDamping();
}

void PhysicsSystem::SetLinearDamping(Entity e, float damping)
{
	GET_COMPONENT(void());
	comp.body->setDamping( damping, comp.body->getAngularDamping() );
}

float PhysicsSystem::GetAngularDamping(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getAngularDamping();
}

void PhysicsSystem::SetAngularDamping(Entity e, float damping)
{
	GET_COMPONENT(void());
	comp.body->setDamping( comp.body->getLinearDamping(), damping );
}

Vector3 PhysicsSystem::GetLinearFactor(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	return comp.body->getLinearFactor();
}

void PhysicsSystem::SetLinearFactor(Entity e, Vector3& factor)
{
	GET_COMPONENT(void());
	comp.body->setLinearFactor(factor);
}

Vector3 PhysicsSystem::GetAngularFactor(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	return comp.body->getAngularFactor();
}

void PhysicsSystem::SetAngularFactor(Entity e, Vector3& factor)
{
	GET_COMPONENT(void());
	comp.body->setAngularFactor(factor);
}

float PhysicsSystem::GetMass(Entity e)
{
	GET_COMPONENT(0);
	return 1.0f / comp.body->getInvMass();
}

void PhysicsSystem::SetMass(Entity e, float mass)
{
	GET_COMPONENT(void());

	btVector3 localInertia;
	comp.body->getCollisionShape()->calculateLocalInertia(mass, localInertia);
	comp.body->setMassProps(mass, localInertia);
}

Vector3 PhysicsSystem::GetLinearVelocity(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	return comp.body->getLinearVelocity();
}

void PhysicsSystem::SetLinearVelocity(Entity e, Vector3& velocity)
{
	GET_COMPONENT(void());
	comp.body->setLinearVelocity(velocity);
}

Vector3 PhysicsSystem::GetAngularVelocity(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	return comp.body->getAngularVelocity();
}

void PhysicsSystem::SetAngularVelocity(Entity e, Vector3& velocity)
{
	GET_COMPONENT(void());
	comp.body->setAngularVelocity(velocity);
}

void PhysicsSystem::ApplyForce(Entity e, Vector3& point, Vector3& force)
{
	GET_COMPONENT(void());
	comp.body->applyForce(force, point);
}

void PhysicsSystem::ApplyCentralForce(Entity e, Vector3& force)
{
	GET_COMPONENT(void());
	comp.body->applyCentralForce(force);
}

void PhysicsSystem::ApplyImpulse(Entity e, Vector3& point, Vector3& impulse)
{
	GET_COMPONENT(void());
	comp.body->applyImpulse(impulse, point);
}

void PhysicsSystem::ApplyCentralImpulse(Entity e, Vector3& impulse)
{
	GET_COMPONENT(void());
	comp.body->applyCentralForce(impulse);
}

void PhysicsSystem::ApplyTorque(Entity e, Vector3& torque)
{
	GET_COMPONENT(void());
	comp.body->applyTorque(torque);
}

void PhysicsSystem::ApplyTorqueImpulse(Entity e, Vector3& torque)
{
	GET_COMPONENT(void());
	comp.body->applyTorqueImpulse(torque);
}

Vector3 PhysicsSystem::GetTotalForce(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	return comp.body->getTotalForce();
}

Vector3 PhysicsSystem::GetTotalTorque(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	return comp.body->getTotalTorque();
}

void PhysicsSystem::ClearForces(Entity e)
{
	GET_COMPONENT(void());
	comp.body->clearForces();
}

RayCastResult PhysicsSystem::RayCast(Vector3& start, Vector3& end)
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

void PhysicsSystem::AddBoxCollider(Entity e, Vector3& pos, Quaternion& rot, Vector3& halfExtents)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btBoxShape(halfExtents));
}

void PhysicsSystem::AddSphereCollider(Entity e, Vector3& pos, Quaternion& rot, float radius)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btSphereShape(radius));
}

void PhysicsSystem::AddConeCollider(Entity e, Vector3& pos, Quaternion& rot, float radius, float height)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btConeShape(radius, height));
}

void PhysicsSystem::AddCylinderCollider(Entity e, Vector3& pos, Quaternion& rot, float radius, float height)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btCylinderShape( btVector3( radius, 0.5f * height, radius) ));
}

void PhysicsSystem::AddCapsuleCollider(Entity e, Vector3& pos, Quaternion& rot, float radius, float height)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btCapsuleShape(radius, height));
}

void PhysicsSystem::ClearCollision(Entity e)
{
	GET_COMPONENT(void());
	_ClearCollision(&comp);
}

void PhysicsSystem::_AddCollisionShape(PhysicsComponent& comp, Vector3& pos, Quaternion& rot, btCollisionShape* shape)
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

		comp.body->setCollisionShape(compound);
	}
}

void PhysicsSystem::_SetCollisionConvex(Entity e, PhysicsComponent* comp, string& name)
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

		auto compPtr = worldPtr->GetPhysicsSystem()->GetComponent(e);
		if(!compPtr)
			CollisionMgr::Get()->DeleteResource(id);
		else
			compPtr->body->setCollisionShape(collisionPtr);
	});
}

void PhysicsSystem::SetConvexHullsCollider(Entity e, string collisionName)
{
	GET_COMPONENT(void());
	_ClearCollision(&comp);
	
	comp.collisionStorage = CollisionStorageType::RESOURCE;
	_SetCollisionConvex(e, &comp, collisionName);
}

void PhysicsSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<RayCastResult>("RayCastResult")
		.addData("position", &RayCastResult::position)
		.addData("normal", &RayCastResult::normal)
		.addData("hit", &RayCastResult::hit)
		.addData("entity", &RayCastResult::entity)
		.endClass()

		.beginClass<PhysicsSystem>("PhysicsSystem")
		.addFunction("UpdateState", &PhysicsSystem::UpdateState)

		.addFunction("IsActive", &PhysicsSystem::IsActive)
		.addFunction("IsEnable", &PhysicsSystem::IsEnable)
		.addFunction("SetEnable", &PhysicsSystem::SetEnable)

		.addFunction("SetType", &PhysicsSystem::SetType)
		.addFunction("GetType", &PhysicsSystem::GetType)

		.addFunction("GetRestitution", &PhysicsSystem::GetRestitution)
		.addFunction("SetRestitution", &PhysicsSystem::SetRestitution)
		.addFunction("GetFriction", &PhysicsSystem::GetFriction)
		.addFunction("SetFriction", &PhysicsSystem::SetFriction)
		.addFunction("GetRollingFriction", &PhysicsSystem::GetRollingFriction)
		.addFunction("SetRollingFriction", &PhysicsSystem::SetRollingFriction)
		.addFunction("GetSpinningFriction", &PhysicsSystem::GetSpinningFriction)
		.addFunction("SetSpinningFriction", &PhysicsSystem::SetSpinningFriction)

		.addFunction("GetContactStiffness", &PhysicsSystem::GetContactStiffness)
		.addFunction("SetContactStiffness", &PhysicsSystem::SetContactStiffness)
		.addFunction("GetContactDamping", &PhysicsSystem::GetContactDamping)
		.addFunction("SetContactDamping", &PhysicsSystem::SetContactDamping)

		.addFunction("GetLinearDamping", &PhysicsSystem::GetLinearDamping)
		.addFunction("SetLinearDamping", &PhysicsSystem::SetLinearDamping)
		.addFunction("GetAngularDamping", &PhysicsSystem::GetAngularDamping)
		.addFunction("SetAngularDamping", &PhysicsSystem::SetAngularDamping)

		.addFunction("GetLinearFactor", &PhysicsSystem::GetLinearFactor)
		.addFunction("SetLinearFactor", &PhysicsSystem::SetLinearFactor)
		.addFunction("GetAngularFactor", &PhysicsSystem::GetAngularFactor)
		.addFunction("SetAngularFactor", &PhysicsSystem::SetAngularFactor)

		.addFunction("GetMass", &PhysicsSystem::GetMass)
		.addFunction("SetMass", &PhysicsSystem::SetMass)

		.addFunction("GetLinearVelocity", &PhysicsSystem::GetLinearVelocity)
		.addFunction("SetLinearVelocity", &PhysicsSystem::SetLinearVelocity)
		.addFunction("GetAngularVelocity", &PhysicsSystem::GetAngularVelocity)
		.addFunction("SetAngularVelocity", &PhysicsSystem::SetAngularVelocity)

		.addFunction("ApplyForce", &PhysicsSystem::ApplyForce)
		.addFunction("ApplyCentralForce", &PhysicsSystem::ApplyCentralForce)
		.addFunction("ApplyImpulse", &PhysicsSystem::ApplyImpulse)
		.addFunction("ApplyCentralImpulse", &PhysicsSystem::ApplyCentralImpulse)
		.addFunction("ApplyTorque", &PhysicsSystem::ApplyTorque)
		.addFunction("ApplyTorqueImpulse", &PhysicsSystem::ApplyTorqueImpulse)

		.addFunction("GetTotalForce", &PhysicsSystem::GetTotalForce)
		.addFunction("GetTotalTorque", &PhysicsSystem::GetTotalTorque)
		.addFunction("ClearForces", &PhysicsSystem::ClearForces)

		.addFunction("AddBoxCollider", &PhysicsSystem::AddBoxCollider)
		.addFunction("AddSphereCollider", &PhysicsSystem::AddSphereCollider)
		.addFunction("AddConeCollider", &PhysicsSystem::AddConeCollider)
		.addFunction("AddCylinderCollider", &PhysicsSystem::AddCylinderCollider)
		.addFunction("AddCapsuleCollider", &PhysicsSystem::AddCapsuleCollider)

		.addFunction("SetConvexHullsCollider", &PhysicsSystem::SetConvexHullsCollider)

		.addFunction("ClearCollision", &PhysicsSystem::ClearCollision)

		.addFunction("RayCast", &PhysicsSystem::RayCast)
		
		.addFunction("AddComponent", &PhysicsSystem::_AddComponent)
		.addFunction("DeleteComponent", &PhysicsSystem::DeleteComponent)
		.addFunction("HasComponent", &PhysicsSystem::HasComponent)

		.addFunction("SetDebugDraw", &PhysicsSystem::SetDebugDraw)
		.endClass();
}

// DEBUG DRAW

PhysicsDebugDrawer::PhysicsDebugDrawer(DebugDrawer* dbgDrawer) :
	m_debugMode(btIDebugDraw::DBG_DrawWireframe),
	m_dbgDrawer(dbgDrawer)
{
	m_colors.m_activeObject = btVector3(0.1f, 0.95f, 0.1f);
	m_colors.m_deactivatedObject = btVector3(0.1f, 0.1f, 0.95f);
}

void PhysicsDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor)
{
	Vector3 A(from);
	Vector3 B(to);
	Vector3 colorA(fromColor);
	Vector3 colorB(toColor);
	m_dbgDrawer->PushLine(A, B, colorA, colorB);
}