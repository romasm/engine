#include "stdafx.h"
#include "PhysicsSystem.h"
#include "World.h"

using namespace rp3d;

PhysicsSystem::PhysicsSystem(BaseWorld* w, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();

	Vector3 defaultGravity(0, -9.81f, 0);
	physWorld = new DynamicsWorld(defaultGravity);

	// temp floor
	Transform tr = Transform::identity();
	tr.setPosition(Vector3(0, 0, 0));
	bodyFloor = physWorld->createRigidBody(tr);
	bodyFloor->setType(STATIC);
	shapeFloor = new BoxShape(Vector3(100.0f, 0.1f, 100.0f));
	bodyFloor->addCollisionShape(shapeFloor, Transform::identity(), 10.0f);
	
	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);

	updateAccum = 0;
	interpolationFactor = 1.0f;
}

PhysicsSystem::~PhysicsSystem()
{
	for(auto& i: *components.data())
	{
		physWorld->destroyRigidBody(i.body);
		_DELETE(i.testBox);
	}

	// temp
	physWorld->destroyRigidBody(bodyFloor);
	_DELETE(shapeFloor);

	_DELETE(physWorld);
}

void PhysicsSystem::Simulate(float dt)
{
	// update physics transform
	for(auto& i: *components.data())
	{
		if(!i.dirty)
			continue;

		Entity e = i.get_entity();

		XMFLOAT3 pos = transformSystem->GetPositionL(e);
		XMVECTOR rot = transformSystem->GetQuatRotationL(e);

		Vector3 position(pos.x, pos.y, pos.z);
		Quaternion rotation(XMVectorGetX(rot), XMVectorGetY(rot), XMVectorGetZ(rot), XMVectorGetW(rot));
		Transform transform(position, rotation);

		i.body->setTransform(transform);

		i.dirty = false;
	}

	// simulation
	updateAccum += dt; 
	while( updateAccum >= PHYSICS_TIME_STEP_MS )
	{
		physWorld->update( PHYSICS_TIME_STEP_MS * 0.001f ); 
		updateAccum -= PHYSICS_TIME_STEP_MS; 
	}

	interpolationFactor = updateAccum / PHYSICS_TIME_STEP_MS;
}

void PhysicsSystem::UpdateTransformations()
{
	for(auto& i: *components.data())
	{
		if( !i.body->isActive() || i.body->isSleeping() )
			continue;

		const Transform& currentTransform = i.body->getTransform();
		if( currentTransform == i.previousTransform )
			continue;

		Transform interpolatedTransform = Transform::interpolateTransforms(i.previousTransform, currentTransform, interpolationFactor);
		i.previousTransform = currentTransform;

		// update matrix: TODO
		const Vector3& pos = interpolatedTransform.getPosition();
		const Quaternion& rot = interpolatedTransform.getOrientation();

		Entity e = i.get_entity();

		transformSystem->SetPosition(e, pos.x, pos.y, pos.z);
		XMVECTOR quat = XMVectorSet(rot.x, rot.y, rot.z, rot.w);
		transformSystem->SetRotation(e, quat);

		transformSystem->SetDirty(e);
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
	
	res->body = physWorld->createRigidBody(Transform::identity());

	// test shape
	auto visSys = world->GetVisibilitySystem();
	auto bbox = visSys->GetBBoxL(e);
	Transform shapeTransform = Transform::identity();
	shapeTransform.setPosition(Vector3(bbox.Center.x, bbox.Center.y, bbox.Center.z));

	res->testBox = new BoxShape(Vector3(bbox.Extents.x, bbox.Extents.y, bbox.Extents.z));

	res->body->addCollisionShape(res->testBox, shapeTransform, 10.0f);
	
	return res;
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
		XMFLOAT4 row_data;
		XMStoreFloat4(&row_data, localMatrix->r[row]);

		*(XMFLOAT4*)t_data = row_data;
		t_data += sizeof(XMFLOAT4);
		size += sizeof(XMFLOAT4);
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
		XMFLOAT4 row_data;
		row_data = *(XMFLOAT4*)t_data;
		t_data += sizeof(XMFLOAT4);
		size += sizeof(XMFLOAT4);

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