#include "stdafx.h"
#include "PhysicsSystem.h"
#include "World.h"

PhysicsSystem::PhysicsSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();

	dynamicsWorld = dynamicsW;

	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);

	defaultCollision = new btBoxShape(Vector3(0.5f, 0.5f, 0.5f));
	defaultMass = 1.0f;
	defaultCollision->calculateLocalInertia(1.0f, defaultInertia);
}

PhysicsSystem::~PhysicsSystem()
{
	for(auto& i: *components.data())
	{
		_DeleteComponent(&i);
	}

	_DELETE(defaultCollision);
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
	
	btRigidBody::btRigidBodyConstructionInfo info(defaultMass, nullptr, defaultCollision, defaultInertia);
	res->body = new btRigidBody(info);
	res->body->setCollisionFlags( res->body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);

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
			btCompoundShape* shape = (btCompoundShape*)comp->collisionData;

			// multilayer compound collisions is NOT supported
			auto childrenCount = shape->getNumChildShapes();
			auto childrenPtrs = shape->getChildList();
			while( childrenCount > 0 )
			{
				delete childrenPtrs;
				childrenPtrs++;
				childrenCount--;
			}

			_DELETE(shape);
			comp->collisionData = 0;
		}
	}
	else
	{
		CollisionMgr::Get()->DeleteResource((uint32_t)comp->collisionData);
		comp->collisionData = (uint64_t)CollisionMgr::nullres;
	}

	comp->body->setCollisionShape(defaultCollision);
}

void PhysicsSystem::CopyComponent(Entity src, Entity dest)
{
	auto comp = GetComponent(src);
	if(!comp || HasComponent(dest)) 
		return;

	PhysicsComponent* res = AddComponent(dest);
	if(!res)
		return;

	res->body->setIsActive(comp->body->isActive());
	res->body->setIsAllowedToSleep(comp->body->isAllowedToSleep());
	res->body->setIsSleeping(comp->body->isSleeping());
	res->body->enableGravity(comp->body->isGravityEnabled());
	res->body->setType(comp->body->getType());
	res->body->setNonRotatable(comp->body->getNonRotatable());

	auto& matSrc = comp->body->getMaterial();
	auto& matDest = res->body->getMaterial();

	matDest.setBounciness(matSrc.getBounciness());
	matDest.setFrictionCoefficient(matSrc.getFrictionCoefficient());
	matDest.setRollingResistance(matSrc.getRollingResistance());

	res->body->setLinearDamping(comp->body->getLinearDamping());
	res->body->setAngularDamping(comp->body->getAngularDamping());

	res->overwriteMass = comp->overwriteMass;
	if(comp->overwriteMass)
	{
		res->body->setMass(comp->body->getMass()); // TODO: to post copy, or will be overwritten in collision system
	}

	res->overwriteCenterOfMass = comp->overwriteCenterOfMass;
	if(comp->overwriteCenterOfMass)
	{
		res->body->setCenterOfMassLocal(comp->body->getCenterOfMassLocal()); // TODO: to post copy
	}
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
	pdata.type = GetType(e);
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
	pdata.mass = 1.0f / comp.body->getInvMass();

	*(PhysicsData*)t_data = pdata;
	t_data += sizeof(PhysicsData);
	
	*(uint8_t*)t_data = comp.collisionStorage;
	t_data += sizeof(uint8_t);

	if( comp.collisionStorage == CollisionStorageType::LOCAL )
	{
		auto childrenCount = ((btCompoundShape*)comp.collisionData)->getNumChildShapes();
		auto childrenPtrs = ((btCompoundShape*)comp.collisionData)->getChildList();

		*(uint32_t*)t_data = childrenCount;
		t_data += sizeof(uint32_t);

		while( childrenCount > 0 )
		{
			

			childrenPtrs++;
			childrenCount--;
		}

		for(auto& handle: comp.shapes)
		{
			*(uint8_t*)t_data = (uint8_t)handle.stoarge;
			t_data += sizeof(uint8_t);

			if( handle.stoarge == CollisionStorageType::RESOURCE )
				continue;

			*(float*)t_data = (float)handle.proxy->getMass();
			t_data += sizeof(float);

			auto& transform = handle.proxy->getLocalToBodyTransform();
			*(Vector3*)t_data = transform.getPosition();
			t_data += sizeof(Vector3);

			*(Quaternion*)t_data = transform.getOrientation();
			t_data += sizeof(Quaternion);

			const rp3d::CollisionShapeType shapeType = handle.shape->getType();

			*(uint8_t*)t_data = (uint8_t)shapeType;
			t_data += sizeof(uint8_t);

			switch (shapeType)
			{
			case rp3d::CollisionShapeType::BOX:
				{
					rp3d::BoxShape* shape = (rp3d::BoxShape*)handle.shape;

					*(Vector3*)t_data = (Vector3)shape->getExtent();
					t_data += sizeof(Vector3);

					*(float*)t_data = (float)shape->getMargin();
					t_data += sizeof(float);
				}
				break;
			case rp3d::CollisionShapeType::SPHERE:
				{
					rp3d::SphereShape* shape = (rp3d::SphereShape*)handle.shape;

					*(float*)t_data = (float)shape->getRadius();
					t_data += sizeof(float);
				}
				break;
			case rp3d::CollisionShapeType::CONE:
				{
					rp3d::ConeShape* shape = (rp3d::ConeShape*)handle.shape;

					*(float*)t_data = (float)shape->getRadius();
					t_data += sizeof(float);

					*(float*)t_data = (float)shape->getHeight();
					t_data += sizeof(float);

					*(float*)t_data = (float)shape->getMargin();
					t_data += sizeof(float);
				}
				break;
			case rp3d::CollisionShapeType::CYLINDER:
				{
					rp3d::CylinderShape* shape = (rp3d::CylinderShape*)handle.shape;

					*(float*)t_data = (float)shape->getRadius();
					t_data += sizeof(float);

					*(float*)t_data = (float)shape->getHeight();
					t_data += sizeof(float);

					*(float*)t_data = (float)shape->getMargin();
					t_data += sizeof(float);
				}
				break;
			case rp3d::CollisionShapeType::CAPSULE:
				{
					rp3d::CapsuleShape* shape = (rp3d::CapsuleShape*)handle.shape;

					*(float*)t_data = (float)shape->getRadius();
					t_data += sizeof(float);

					*(float*)t_data = (float)shape->getHeight();
					t_data += sizeof(float);
				}
				break;
			default:
				ERR("Wrong type of collision shape: %i", (int32_t)shapeType);
				break;
			}
		}
	}
	else
	{
		string collision_name = CollisionMgr::GetName(comp.collisionData);
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
	auto comp = AddComponent(e);
	if(!comp)
		return 0;

	uint8_t* t_data = data;
	
	comp->body->setIsActive(*(uint8_t*)t_data > 0);
	t_data += sizeof(uint8_t);

	comp->body->setIsAllowedToSleep(*(uint8_t*)t_data > 0);
	t_data += sizeof(uint8_t);

	comp->body->setIsSleeping(*(uint8_t*)t_data > 0);
	t_data += sizeof(uint8_t);

	comp->body->enableGravity(*(uint8_t*)t_data > 0);
	t_data += sizeof(uint8_t);

	comp->body->setType((rp3d::BodyType)(*(uint8_t*)t_data));
	t_data += sizeof(uint8_t);

	comp->body->setNonRotatable(*(uint8_t*)t_data > 0);
	t_data += sizeof(uint8_t);

	auto& mat = comp->body->getMaterial();

	mat.setBounciness(*(float*)t_data);
	t_data += sizeof(float);

	mat.setFrictionCoefficient(*(float*)t_data);
	t_data += sizeof(float);

	mat.setRollingResistance(*(float*)t_data);
	t_data += sizeof(float);

	comp->body->setLinearDamping(*(float*)t_data);
	t_data += sizeof(float);

	comp->body->setAngularDamping(*(float*)t_data);
	t_data += sizeof(float);

	comp->overwriteMass = *(uint8_t*)t_data > 0;
	t_data += sizeof(uint8_t);

	if(comp->overwriteMass)
	{
		comp->body->setMass(*(float*)t_data); // to post load, or will be overwritten in collision system
		t_data += sizeof(float);
	}

	comp->overwriteCenterOfMass = *(uint8_t*)t_data > 0;
	t_data += sizeof(uint8_t);

	if(comp->overwriteCenterOfMass)
	{
		comp->body->setCenterOfMassLocal(*(Vector3*)t_data); // to post load
		t_data += sizeof(Vector3);
	}

	return (uint32_t)(t_data - data);
}

// PARAMS

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
	comp.body->getTotalForce();
}

Vector3 PhysicsSystem::GetTotalTorque(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	comp.body->getTotalTorque();
}

void PhysicsSystem::ClearForces(Entity e)
{
	GET_COMPONENT(void());
	comp.body->clearForces();
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

void PhysicsSystem::AddCylinderCollider(Entity e, Vector3& pos, Quaternion& rot, Vector3& halfExtents)
{
	GET_COMPONENT(void());
	_AddCollisionShape(comp, pos, rot, new btCylinderShape(halfExtents));
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

		btCompoundShape* shape = (btCompoundShape*)comp.collisionData;
		shape->addChildShape(btTransform(rot, pos), shape);

		comp.body->setCollisionShape(shape);
	}
}

void PhysicsSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<PhysicsSystem>("PhysicsSystem")
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
		.addFunction("ApplyForceToCenterOfMass", &PhysicsSystem::ApplyCentralForce)
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

		.addFunction("ClearCollision", &PhysicsSystem::ClearCollision)
		
		.addFunction("AddComponent", &PhysicsSystem::_AddComponent)
		.addFunction("DeleteComponent", &PhysicsSystem::DeleteComponent)
		.addFunction("HasComponent", &PhysicsSystem::HasComponent)
		.endClass();
}