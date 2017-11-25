#include "stdafx.h"
#include "PhysicsSystem.h"
#include "WorldMgr.h"
#include "World.h"

PhysicsSystem::PhysicsSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();
	collisionSystem = world->GetCollisionSystem();

	dynamicsWorld = dynamicsW;
	
	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
}

PhysicsSystem::~PhysicsSystem()
{
	for(auto& i: *components.data())
	{
		_DeleteComponent(&i);
	}
}

void PhysicsSystem::updateSingleTransformation(PhysicsComponent& comp)
{
	Entity e = comp.get_entity();

	// TODO: parented entities?
	btTransform transform = ToBtTransform( transformSystem->GetTransform_LInternal(e) );
	//transform.setOrigin( transform.getOrigin() + btVector3(i.centerOfMassOffset) );
	comp.body->proceedToTransform( transform );

	if( comp.body->wantsSleeping() )
		comp.body->activate( true );

	comp.dirty = false;
}

void PhysicsSystem::UpdateTransformations()
{
	for(auto& i: *components.data())
	{
		if(!i.dirty)
			continue;
		updateSingleTransformation(i);		
	}
}

void PhysicsSystem::SimulateAndUpdate(float dt)
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
	
	int32_t collisionGroup = CollisionGroups::Default;
	int32_t collisionMask = CollisionGroups::Physics;

	btCollisionShape* collision = CollisionMgr::GetResourcePtr(CollisionMgr::nullres);
	auto collisionComp = collisionSystem->GetComponent(e);
	if(collisionComp)
	{
		if(collisionComp->collisionData)
			collision = collisionSystem->GetCollision(e);

		if( collisionComp->collisionGroup != 0 )
			collisionGroup = collisionComp->collisionGroup;
		else
			collisionComp->collisionGroup = collisionGroup;

		if( collisionComp->collisionMask != 0 )
			collisionMask = collisionComp->collisionMask;
		else
			collisionComp->collisionMask = collisionMask;
	}

	btRigidBody::btRigidBodyConstructionInfo info(0, nullptr, collision);
	info.m_linearSleepingThreshold = SLEEP_THRESHOLD_LINEAR;
	info.m_angularSleepingThreshold = SLEEP_THRESHOLD_ANGULAR;

	res->body = new btRigidBody(info);
	res->body->setUserIndex(e);
	res->body->setCollisionFlags( res->body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
	
	dynamicsWorld->addRigidBody(res->body, collisionGroup, collisionMask);

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
	
	int32_t collisionGroup = CollisionGroups::Default;
	int32_t collisionMask = CollisionGroups::Physics;

	auto collision = collisionSystem->GetCollision(e);
	if(!collision)
		WRN("Collision component must be set before Physics component");
	else
	{
		comp.body->setCollisionShape(collision);
		auto collisionComp = collisionSystem->GetComponent(e);
		collisionGroup = collisionComp->collisionGroup;
		collisionMask = collisionComp->collisionMask;
	}

	dynamicsWorld->addRigidBody(comp.body, collisionGroup, collisionMask);
}

bool PhysicsSystem::IsActive(Entity e)
{
	GET_COMPONENT(false);
	return comp.body->isActive();
}

bool PhysicsSystem::IsEnableSimulation(Entity e)
{
	GET_COMPONENT(false);
	return comp.body->getActivationState() != DISABLE_SIMULATION;
}

void PhysicsSystem::SetEnableSimulation(Entity e, bool enable, bool nonSleeping)
{
	GET_COMPONENT(void());
	int32_t state = enable ? (nonSleeping ? DISABLE_DEACTIVATION : ACTIVE_TAG) : DISABLE_SIMULATION;
	comp.body->forceActivationState(state);
}

void PhysicsSystem::SetEnable(Entity e, bool enable)
{
	GET_COMPONENT(void());

	if(enable)
	{
		auto clsComp = collisionSystem->GetComponent(e);
		if(!clsComp)
			return;
		dynamicsWorld->addRigidBody(comp.body, clsComp->collisionGroup, clsComp->collisionMask);
	}
	else
	{
		dynamicsWorld->removeRigidBody(comp.body);
	}
}

// Overwrite collision group and mask
void PhysicsSystem::SetType(Entity e, int32_t type)
{
	GET_COMPONENT(void());
	auto flags = comp.body->getCollisionFlags();

	auto collisionComp = collisionSystem->GetComponent(e);
	if(!collisionComp)
	{
		WRN("Collision component must be set before Physics component");
		return;
	}
	
	switch (type)
	{
	case 0:
		flags |= btCollisionObject::CF_STATIC_OBJECT;
		flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
		collisionComp->collisionMask &= ~CollisionGroups::Static;
		collisionComp->collisionMask &= ~CollisionGroups::Kinematic;
		collisionComp->collisionMask &= ~CollisionGroups::Trigger;
		if( collisionComp->collisionGroup == CollisionGroups::Kinematic || collisionComp->collisionGroup == CollisionGroups::Default )
			collisionComp->collisionGroup = CollisionGroups::Static;
		break;
	case 1:
		flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
		flags &= ~btCollisionObject::CF_STATIC_OBJECT;
		collisionComp->collisionMask &= ~CollisionGroups::Static;
		collisionComp->collisionMask &= ~CollisionGroups::Kinematic;
		if( collisionComp->collisionGroup == CollisionGroups::Static )
			collisionComp->collisionMask |= CollisionGroups::Trigger;
		if( collisionComp->collisionGroup == CollisionGroups::Static || collisionComp->collisionGroup == CollisionGroups::Default )
			collisionComp->collisionGroup = CollisionGroups::Kinematic;
		break;
	case 2:
		flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
		flags &= ~btCollisionObject::CF_STATIC_OBJECT;
		collisionComp->collisionMask |= CollisionGroups::Static;
		collisionComp->collisionMask |= CollisionGroups::Kinematic;
		if( collisionComp->collisionGroup == CollisionGroups::Static )
			collisionComp->collisionMask |= CollisionGroups::Trigger;
		if( collisionComp->collisionGroup == CollisionGroups::Static || collisionComp->collisionGroup == CollisionGroups::Kinematic )
			collisionComp->collisionGroup = CollisionGroups::Default;
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
	comp.body->applyCentralImpulse(impulse);
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

void PhysicsSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<PhysicsSystem>("PhysicsSystem")
		.addFunction("UpdateState", &PhysicsSystem::UpdateState)

		.addFunction("IsActive", &PhysicsSystem::IsActive)
		.addFunction("IsEnable", &PhysicsSystem::IsEnableSimulation)
		.addFunction("SetEnable", &PhysicsSystem::SetEnableSimulation)

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
				
		.addFunction("AddComponent", &PhysicsSystem::_AddComponent)
		.addFunction("DeleteComponent", &PhysicsSystem::DeleteComponent)
		.addFunction("HasComponent", &PhysicsSystem::HasComponent)
		.endClass();
}