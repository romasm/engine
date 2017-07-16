#include "stdafx.h"
#include "PhysicsSystem.h"
#include "World.h"

PhysicsSystem::PhysicsSystem(BaseWorld* w, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();

	Vector3 defaultGravity(0, -9.81f, 0);
	physWorld = new rp3d::DynamicsWorld(defaultGravity);

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
	
	_DELETE(physWorld);
}

void PhysicsSystem::UpdateTransformations()
{
	for(auto& i: *components.data())
	{
		if(!i.dirty)
			continue;

		Entity e = i.get_entity();

		i.previousTransform = transformSystem->GetTransformL(e);
		i.body->setTransform(i.previousTransform);
		
		i.body->setIsSleeping(false);

		i.dirty = false;
	}
}

void PhysicsSystem::Simulate(float dt)
{
	updateAccum += dt; 
	while( updateAccum >= PHYSICS_TIME_STEP_MS )
	{
		physWorld->update( PHYSICS_TIME_STEP_MS * 0.001f ); 
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
	
	res->body = physWorld->createRigidBody(rp3d::Transform::identity());
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

void PhysicsSystem::CopyComponent(Entity src, Entity dest)
{
	auto comp = GetComponent(src);
	if(!comp || HasComponent(dest)) 
		return;

	PhysicsComponent* res = AddComponent(dest);
	if(!res)
		return;

	// TODO
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
		/*
	uint8_t* t_data = data;
	uint32_t size = 0;

	const XMMATRIX* localMatrix = sceneGraph->GetLocalTransformation(comp.nodeID);

	for(uint32_t row = 0; row < 4; row++)
	{
		Vector4 row_data;
		XMStoreFloat4(&row_data, localMatrix->r[row]);

		*(Vector4*)t_data = row_data;
		t_data += sizeof(Vector4);
		size += sizeof(Vector4);
	}

	uint32_t parentNode = sceneGraph->GetParent(comp.nodeID);

	if(parentNode == SCENEGRAPH_NULL_ID)
	{
		*(uint32_t*)t_data = 0;
		t_data += sizeof(uint32_t);
		size += sizeof(uint32_t);
	}
	else
	{
		Entity parentEntity = sceneGraph->GetEntityByNode(parentNode);
		string name = world->GetNameMgr()->GetName(parentEntity);

		uint32_t name_size = (uint32_t)name.size();
		*(uint32_t*)t_data = name_size;
		t_data += sizeof(uint32_t);
		size += sizeof(uint32_t);

		if(name_size > 0)
		{
			memcpy_s(t_data, name_size, name.data(), name_size);
			t_data += name_size * sizeof(char);
			size += name_size * sizeof(char);
		}
	}

	return size;*/
	return 0;
}

uint32_t PhysicsSystem::Deserialize(Entity e, uint8_t* data)
{
	auto comp = AddComponent(e);
	if(!comp)
		return 0;
		
	/*uint8_t* t_data = data;
	uint32_t size = 0;

	XMMATRIX localMatrix;
	for(uint32_t row = 0; row < 4; row++)
	{
		Vector4 row_data;
		row_data = *(Vector4*)t_data;
		t_data += sizeof(Vector4);
		size += sizeof(Vector4);

		localMatrix.r[row] = XMLoadFloat4(&row_data);
	}

	sceneGraph->SetTransformation(comp->nodeID, localMatrix);

	uint32_t parentName_size = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

	if(parentName_size == 0)
		return size;
	
	string parentName((char*)t_data, parentName_size);
	t_data += parentName_size * sizeof(char);
	size += parentName_size * sizeof(char);

	if(!attachments_map)
		ERR("Attachments map uninitialized, need PreLoad call first!");
	else
		attachments_map->insert(make_pair(UintFromEntity(e), parentName));

	return size;*/
	return 0;
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
}

void PhysicsSystem::SetCenterOfMass(Entity e, Vector3 local_point)
{
	GET_COMPONENT(void());
	comp.body->setCenterOfMassLocal(local_point);
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

// SHAPES

int32_t PhysicsSystem::AddBoxShape(Entity e, Vector3 pos, Quaternion rot, float mass, Vector3 halfSize)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::BoxShape(halfSize);
	return AddShape(comp, pos, rot, mass, shape);
}

int32_t PhysicsSystem::AddSphereShape(Entity e, Vector3 pos, Quaternion rot, float mass, float radius)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::SphereShape(radius);
	return AddShape(comp, pos, rot, mass, shape);
}

int32_t PhysicsSystem::AddConeShape(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::ConeShape(radius, height);
	return AddShape(comp, pos, rot, mass, shape);
}

int32_t PhysicsSystem::AddCylinderShape(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::CylinderShape(radius, height);
	return AddShape(comp, pos, rot, mass, shape);
}

int32_t PhysicsSystem::AddCapsuleShape(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::CapsuleShape(radius, height);
	return AddShape(comp, pos, rot, mass, shape);
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

		.addFunction("AddBoxShape", &PhysicsSystem::AddBoxShape)
		.addFunction("AddSphereShape", &PhysicsSystem::AddSphereShape)
		.addFunction("AddConeShape", &PhysicsSystem::AddConeShape)
		.addFunction("AddCylinderShape", &PhysicsSystem::AddCylinderShape)
		.addFunction("AddCapsuleShape", &PhysicsSystem::AddCapsuleShape)

		.addFunction("AddComponent", &PhysicsSystem::_AddComponent)
		.addFunction("DeleteComponent", &PhysicsSystem::DeleteComponent)
		.addFunction("HasComponent", &PhysicsSystem::HasComponent)
		.endClass();
}