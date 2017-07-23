#include "stdafx.h"
#include "PhysicsSystem.h"
#include "World.h"

PhysicsSystem::PhysicsSystem(BaseWorld* w, rp3d::DynamicsWorld* dynamicsW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();

	dynamicsWorld = dynamicsW;

	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);

	updateAccum = 0;
	interpolationFactor = 1.0f;
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

		i.previousTransform = transformSystem->GetTransformW(e);
		i.body->setTransform(i.previousTransform);
		
		i.body->setIsSleeping(false);

		i.dirty = false;
	}
}

void PhysicsSystem::Simulate(float dt)
{
	if( components.empty() )
	{
		updateAccum = 0;
		return;
	}

	updateAccum += dt; 
	while( updateAccum >= PHYSICS_TIME_STEP_MS )
	{
		dynamicsWorld->update( PHYSICS_TIME_STEP_MS * 0.001f ); 
		updateAccum -= PHYSICS_TIME_STEP_MS; 
	}

	interpolationFactor = updateAccum / PHYSICS_TIME_STEP_MS;
}

void PhysicsSystem::UpdateSceneGraph()
{
	for(auto& i: *components.data())
	{
		if( !i.body->isActive() || i.body->isSleeping() || i.body->getType() == rp3d::STATIC )
			continue;

		const rp3d::Transform& currentTransform = i.body->getTransform();
		if( currentTransform == i.previousTransform )
			continue;

		rp3d::Transform interpolatedTransform = rp3d::Transform::interpolateTransforms(i.previousTransform, currentTransform, interpolationFactor);
		i.previousTransform = currentTransform;
		
		transformSystem->SetPhysicsTransform(i.get_entity(), XMMATRIX(interpolatedTransform));

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
	res->overwriteMass = false;
	res->overwriteCenterOfMass = false;
	
	res->body = dynamicsWorld->createRigidBody(rp3d::Transform::identity());
	res->body->setIsActive(false);
		
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
	dynamicsWorld->destroyRigidBody(comp->body);
	comp->body = nullptr;

	auto collisionComp = world->GetCollisionSystem()->GetComponent(comp->get_entity());
	if(collisionComp)
		collisionComp->body = nullptr;
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
	
	*(uint8_t*)t_data = comp.body->isActive() ? 1 : 0;
	t_data += sizeof(uint8_t);

	*(uint8_t*)t_data = comp.body->isAllowedToSleep() ? 1 : 0;
	t_data += sizeof(uint8_t);

	*(uint8_t*)t_data = comp.body->isSleeping() ? 1 : 0;
	t_data += sizeof(uint8_t);

	*(uint8_t*)t_data = comp.body->isGravityEnabled() ? 1 : 0;
	t_data += sizeof(uint8_t);

	*(uint8_t*)t_data = (uint8_t)comp.body->getType();
	t_data += sizeof(uint8_t);

	*(uint8_t*)t_data = comp.body->getNonRotatable() ? 1 : 0;
	t_data += sizeof(uint8_t);

	auto& mat = comp.body->getMaterial();

	*(float*)t_data = (float)mat.getBounciness();
	t_data += sizeof(float);

	*(float*)t_data = (float)mat.getFrictionCoefficient();
	t_data += sizeof(float);

	*(float*)t_data = (float)mat.getRollingResistance();
	t_data += sizeof(float);

	*(float*)t_data = (float)comp.body->getLinearDamping();
	t_data += sizeof(float);

	*(float*)t_data = (float)comp.body->getAngularDamping();
	t_data += sizeof(float);

	*(uint8_t*)t_data = comp.overwriteMass ? 1 : 0;
	t_data += sizeof(uint8_t);

	if(comp.overwriteMass)
	{
		*(float*)t_data = (float)comp.body->getMass();
		t_data += sizeof(float);
	}

	*(uint8_t*)t_data = comp.overwriteCenterOfMass ? 1 : 0;
	t_data += sizeof(uint8_t);

	if(comp.overwriteCenterOfMass)
	{
		*(Vector3*)t_data = (Vector3)comp.body->getCenterOfMassLocal();
		t_data += sizeof(Vector3);
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

void PhysicsSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(void());
	comp.body->setIsActive(active);
}

bool PhysicsSystem::IsSleeping(Entity e)
{
	GET_COMPONENT(false);
	return comp.body->isSleeping();
}

void PhysicsSystem::SetSleeping(Entity e, bool sleep)
{
	GET_COMPONENT(void());
	comp.body->setIsSleeping(sleep);
}

bool PhysicsSystem::IsUnsleepable(Entity e)
{
	GET_COMPONENT(false);
	return comp.body->isAllowedToSleep();
}

void PhysicsSystem::SetUnsleepable(Entity e, bool unsleepable)
{
	GET_COMPONENT(void());
	comp.body->setIsAllowedToSleep(unsleepable);
}

bool PhysicsSystem::IsGravityEnabled(Entity e)
{
	GET_COMPONENT(false);
	return comp.body->isGravityEnabled();
}

void PhysicsSystem::SetGravityEnabled(Entity e, bool enabled)
{
	GET_COMPONENT(void());
	comp.body->enableGravity(enabled);
}

void PhysicsSystem::SetType(Entity e, int32_t type)
{
	GET_COMPONENT(void());
	comp.body->setType(rp3d::BodyType(type));
}

int32_t PhysicsSystem::GetType(Entity e)
{
	GET_COMPONENT(-1);
	return (int32_t)comp.body->getType();
}

void PhysicsSystem::SetNonRotatable(Entity e, bool isNonRot)
{
	GET_COMPONENT(void());
	comp.body->setNonRotatable(isNonRot);
}

bool PhysicsSystem::GetNonRotatable(Entity e)
{
	GET_COMPONENT(false);
	return comp.body->getNonRotatable();
}

float PhysicsSystem::GetBounciness(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getMaterial().getBounciness();
}

void PhysicsSystem::SetBounciness(Entity e, float bounciness)
{
	GET_COMPONENT(void());
	comp.body->getMaterial().setBounciness(bounciness);
}

float PhysicsSystem::GetFriction(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getMaterial().getFrictionCoefficient();
}

void PhysicsSystem::SetFriction(Entity e, float friction)
{
	GET_COMPONENT(void());
	comp.body->getMaterial().setFrictionCoefficient(friction);
}

float PhysicsSystem::GetRollingResistance(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getMaterial().getRollingResistance();
}

void PhysicsSystem::SetRollingResistance(Entity e, float resistance)
{
	GET_COMPONENT(void());
	comp.body->getMaterial().setRollingResistance(resistance);
}

float PhysicsSystem::GetVelocityDamping(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getLinearDamping();
}

void PhysicsSystem::SetVelocityDamping(Entity e, float damping)
{
	GET_COMPONENT(void());
	comp.body->setLinearDamping(damping);
}

float PhysicsSystem::GetAngularDamping(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getAngularDamping();
}

void PhysicsSystem::SetAngularDamping(Entity e, float damping)
{
	GET_COMPONENT(void());
	comp.body->setAngularDamping(damping);
}

float PhysicsSystem::GetMass(Entity e)
{
	GET_COMPONENT(0);
	return comp.body->getMass();
}

void PhysicsSystem::SetMass(Entity e, float mass)
{
	GET_COMPONENT(void());
	comp.body->setMass(mass);
	comp.overwriteMass = true;
}

Vector3 PhysicsSystem::GetCenterOfMass(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	return comp.body->getCenterOfMassLocal();
}

void PhysicsSystem::SetCenterOfMass(Entity e, Vector3 local_point)
{
	GET_COMPONENT(void());
	comp.body->setCenterOfMassLocal(local_point);
	comp.overwriteCenterOfMass = true;
}

Vector3 PhysicsSystem::GetVelocity(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	return comp.body->getLinearVelocity();
}

void PhysicsSystem::SetVelocity(Entity e, Vector3 velocity)
{
	GET_COMPONENT(void());
	comp.body->setLinearVelocity(velocity);
}

Vector3 PhysicsSystem::GetAngularVelocity(Entity e)
{
	GET_COMPONENT(Vector3::Zero);
	return comp.body->getAngularVelocity();
}

void PhysicsSystem::SetAngularVelocity(Entity e, Vector3 velocity)
{
	GET_COMPONENT(void());
	comp.body->setAngularVelocity(velocity);
}

void PhysicsSystem::ApplyForce(Entity e, Vector3 point, Vector3 force)
{
	GET_COMPONENT(void());
	comp.body->applyForce(force, point);
}

void PhysicsSystem::ApplyForceToCenterOfMass(Entity e, Vector3 force)
{
	GET_COMPONENT(void());
	comp.body->applyForceToCenterOfMass(force);
}

void PhysicsSystem::ApplyTorque(Entity e, Vector3 torque)
{
	GET_COMPONENT(void());
	comp.body->applyTorque(torque);
}

void PhysicsSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<PhysicsSystem>("PhysicsSystem")
		.addFunction("IsActive", &PhysicsSystem::IsActive)
		.addFunction("SetActive", &PhysicsSystem::SetActive)
		.addFunction("IsSleeping", &PhysicsSystem::IsSleeping)
		.addFunction("SetSleeping", &PhysicsSystem::SetSleeping)
		.addFunction("IsUnsleepable", &PhysicsSystem::IsUnsleepable)
		.addFunction("SetUnsleepable", &PhysicsSystem::SetUnsleepable)
		.addFunction("IsGravityEnabled", &PhysicsSystem::IsGravityEnabled)
		.addFunction("SetGravityEnabled", &PhysicsSystem::SetGravityEnabled)

		.addFunction("SetType", &PhysicsSystem::SetType)
		.addFunction("GetType", &PhysicsSystem::GetType)
		.addFunction("GetNonRotatable", &PhysicsSystem::GetNonRotatable)
		.addFunction("SetNonRotatable", &PhysicsSystem::SetNonRotatable)

		.addFunction("GetBounciness", &PhysicsSystem::GetBounciness)
		.addFunction("SetBounciness", &PhysicsSystem::SetBounciness)

		.addFunction("GetFriction", &PhysicsSystem::GetFriction)
		.addFunction("SetFriction", &PhysicsSystem::SetFriction)

		.addFunction("GetRollingResistance", &PhysicsSystem::GetRollingResistance)
		.addFunction("SetRollingResistance", &PhysicsSystem::SetRollingResistance)

		.addFunction("GetVelocityDamping", &PhysicsSystem::GetVelocityDamping)
		.addFunction("SetVelocityDamping", &PhysicsSystem::SetVelocityDamping)

		.addFunction("GetAngularDamping", &PhysicsSystem::GetAngularDamping)
		.addFunction("SetAngularDamping", &PhysicsSystem::SetAngularDamping)

		.addFunction("GetMass", &PhysicsSystem::GetMass)
		.addFunction("SetMass", &PhysicsSystem::SetMass)
		.addFunction("SetCenterOfMass", &PhysicsSystem::SetCenterOfMass)

		.addFunction("GetVelocity", &PhysicsSystem::GetVelocity)
		.addFunction("SetVelocity", &PhysicsSystem::SetVelocity)

		.addFunction("GetAngularVelocity", &PhysicsSystem::GetAngularVelocity)
		.addFunction("SetAngularVelocity", &PhysicsSystem::SetAngularVelocity)

		.addFunction("ApplyForce", &PhysicsSystem::ApplyForce)
		.addFunction("ApplyForceToCenterOfMass", &PhysicsSystem::ApplyForceToCenterOfMass)
		.addFunction("ApplyTorque", &PhysicsSystem::ApplyTorque)
		
		.addFunction("AddComponent", &PhysicsSystem::_AddComponent)
		.addFunction("DeleteComponent", &PhysicsSystem::DeleteComponent)
		.addFunction("HasComponent", &PhysicsSystem::HasComponent)
		.endClass();
}