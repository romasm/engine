#include "stdafx.h"
#include "CollisionSystem.h"
#include "World.h"

CollisionSystem::CollisionSystem(BaseWorld* w, rp3d::CollisionWorld* collisionW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();
	physicsSystem = world->GetPhysicsSystem();

	collisionWorld = collisionW;

	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
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

		if(i.physicsBody)
		{
			i.dirty = false;
			continue;
		}

		if(!i.dirty || i.physicsBody)
			continue;

		i.body->setTransform(transformSystem->GetTransformW(i.get_entity()));
		i.dirty = false;
	}
}

CollisionComponent* CollisionSystem::AddComponent(Entity e)
{
	if(HasComponent(e))
		return &components.getDataById(e.index());

	if( !transformSystem->GetParent(e).isnull() )
		return nullptr;

	CollisionComponent* res = components.add(e.index());
	res->parent = e;
	res->dirty = true;

	auto physicsComp = physicsSystem->GetComponent(e);
		
	if(physicsComp)
	{
		res->physicsBody = true;
		res->body = physicsComp->body;
	}
	else
	{
		res->physicsBody = false;
		res->body = collisionWorld->createCollisionBody(rp3d::Transform::identity());
	}

	res->body->setIsActive(false);

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

void CollisionSystem::CopyComponent(Entity src, Entity dest)
{
	auto comp = GetComponent(src);
	if(!comp || HasComponent(dest)) 
		return;

	CollisionComponent* res = AddComponent(dest);
	if(!res)
		return;

	res->body->setIsActive(comp->body->isActive());

	for(auto& handle: comp->shapes)
	{
		auto& newHandle = res->shapes.push_back();
		newHandle.stoarge = handle.stoarge;

		if( newHandle.stoarge == CollisionStorageType::RESOURCE )
		{
			newHandle.shape = nullptr;//CollisionMgr::GetResource(handle.shape);

			if(comp->physicsBody)
				newHandle.proxy = ((rp3d::RigidBody*)comp->body)->addCollisionShape(newHandle.shape, 
					handle.proxy->getLocalToBodyTransform(), handle.proxy->getMass());
			else
				newHandle.proxy = comp->body->addCollisionShape(newHandle.shape, 
					handle.proxy->getLocalToBodyTransform());
		}
		else
		{
			auto type = handle.shape->getType();
			switch( type )
			{
			case rp3d::CollisionShapeType::BOX:
				{
					rp3d::BoxShape* shape = (rp3d::BoxShape*)handle.shape;
					newHandle.shape = new rp3d::BoxShape(shape->getExtent(), shape->getMargin());
				}
				break;
			case rp3d::CollisionShapeType::SPHERE:
				{
					rp3d::SphereShape* shape = (rp3d::SphereShape*)handle.shape;
					newHandle.shape = new rp3d::SphereShape(shape->getRadius());
				}
				break;
			case rp3d::CollisionShapeType::CONE:
				{
					rp3d::ConeShape* shape = (rp3d::ConeShape*)handle.shape;
					newHandle.shape = new rp3d::ConeShape(shape->getRadius(), shape->getHeight(), shape->getMargin());
				}
				break;
			case rp3d::CollisionShapeType::CYLINDER:
				{
					rp3d::CylinderShape* shape = (rp3d::CylinderShape*)handle.shape;
					newHandle.shape = new rp3d::CylinderShape(shape->getRadius(), shape->getHeight(), shape->getMargin());
				}
				break;
			case rp3d::CollisionShapeType::CAPSULE:
				{
					rp3d::CapsuleShape* shape = (rp3d::CapsuleShape*)handle.shape;
					newHandle.shape = new rp3d::CapsuleShape(shape->getRadius(), shape->getHeight());
				}
				break;
			default:
				ERR("Wrong type of collision shape: %i", (int32_t)type);
				break;
			}			 
			
			if(comp->physicsBody)
				newHandle.proxy = ((rp3d::RigidBody*)comp->body)->addCollisionShape(newHandle.shape, 
					handle.proxy->getLocalToBodyTransform(), handle.proxy->getMass());
			else
				newHandle.proxy = comp->body->addCollisionShape(newHandle.shape, 
					handle.proxy->getLocalToBodyTransform());
		}
	}
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

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

	// BODY
	*(uint8_t*)t_data = comp.physicsBody ? 1 : 0;
	t_data += sizeof(uint8_t);

	if(!comp.physicsBody)
	{
		*(uint8_t*)t_data = comp.body->isActive() ? 1 : 0;
		t_data += sizeof(uint8_t);
	}

	// SHAPES
	uint32_t shapesCount = (uint32_t)comp.shapes.size();
	*(uint32_t*)t_data = shapesCount;
	t_data += sizeof(uint32_t);

	for(auto& handle: comp.shapes)
	{
		*(uint8_t*)t_data = (uint8_t)handle.stoarge;
		t_data += sizeof(uint8_t);

		*(float*)t_data = (float)handle.proxy->getMass();
		t_data += sizeof(float);

		auto& transform = handle.proxy->getLocalToBodyTransform();
		*(Vector3*)t_data = transform.getPosition();
		t_data += sizeof(Vector3);
		
		*(Quaternion*)t_data = transform.getOrientation();
		t_data += sizeof(Quaternion);

		if( handle.stoarge == CollisionStorageType::RESOURCE )
		{
			string collision_name = "";//CollisionMgr::GetName(handle.shape);
			uint32_t collision_name_size = (uint32_t)collision_name.size();

			*(uint32_t*)t_data = collision_name_size;
			t_data += sizeof(uint32_t);

			memcpy_s(t_data, collision_name_size, collision_name.data(), collision_name_size);
			t_data += collision_name_size * sizeof(char);
		}
		else
		{
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

	return (uint32_t)(t_data - data);
}

uint32_t CollisionSystem::Deserialize(Entity e, uint8_t* data)
{
	auto comp = AddComponent(e);
	if(!comp)
		return 0;
		
	uint8_t* t_data = data;

	// BODY
	bool physicsBody = *(uint8_t*)t_data > 0;
	t_data += sizeof(uint8_t);

	if(comp->physicsBody != physicsBody)
		ERR("Physics component needed to be initialize first");

	if(!physicsBody)
	{
		bool isActive = *(uint8_t*)t_data > 0;
		t_data += sizeof(uint8_t);

		comp->body->setIsActive(isActive);
	}

	// SHAPES
	uint32_t shapesCount = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

	for(uint32_t i = 0; i < shapesCount; i++)
	{
		CollisionStorageType stoarge = (CollisionStorageType)(*(uint8_t*)t_data);
		t_data += sizeof(uint8_t);

		float mass = *(float*)t_data;
		t_data += sizeof(float);

		Vector3 pos = *(Vector3*)t_data;
		t_data += sizeof(Vector3);

		Quaternion rot = *(Quaternion*)t_data;
		t_data += sizeof(Quaternion);

		if( stoarge == CollisionStorageType::RESOURCE )
		{
			uint32_t collision_name_size = *(uint32_t*)t_data;
			t_data += sizeof(uint32_t);

			string collision_name((char*)t_data, collision_name_size);
			t_data += collision_name_size * sizeof(char);

			rp3d::CollisionShape* shape = nullptr;//CollisionMgr::GetResource(collision_name);

			AddShape(*comp, pos, rot, mass, shape, CollisionStorageType::RESOURCE);
		}
		else
		{
			rp3d::CollisionShapeType shapeType = (rp3d::CollisionShapeType)(*(uint8_t*)t_data);
			t_data += sizeof(uint8_t);

			switch (shapeType)
			{
			case rp3d::CollisionShapeType::BOX:
				{
					Vector3 extents = *(Vector3*)t_data;
					t_data += sizeof(Vector3);

					float margin = *(float*)t_data;
					t_data += sizeof(float);

					rp3d::BoxShape* shape = new rp3d::BoxShape(extents, margin);
					AddShape(*comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
				}
				break;
			case rp3d::CollisionShapeType::SPHERE:
				{
					float radius = *(float*)t_data;
					t_data += sizeof(float);

					rp3d::SphereShape* shape = new rp3d::SphereShape(radius);
					AddShape(*comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
				}
				break;
			case rp3d::CollisionShapeType::CONE:
				{
					float radius = *(float*)t_data;
					t_data += sizeof(float);

					float height = *(float*)t_data;
					t_data += sizeof(float);

					float margin = *(float*)t_data;
					t_data += sizeof(float);

					rp3d::ConeShape* shape = new rp3d::ConeShape(radius, height, margin);
					AddShape(*comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
				}
				break;
			case rp3d::CollisionShapeType::CYLINDER:
				{
					float radius = *(float*)t_data;
					t_data += sizeof(float);

					float height = *(float*)t_data;
					t_data += sizeof(float);

					float margin = *(float*)t_data;
					t_data += sizeof(float);

					rp3d::CylinderShape* shape = new rp3d::CylinderShape(radius, height, margin);
					AddShape(*comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
				}
				break;
			case rp3d::CollisionShapeType::CAPSULE:
				{
					float radius = *(float*)t_data;
					t_data += sizeof(float);

					float height = *(float*)t_data;
					t_data += sizeof(float);

					rp3d::CapsuleShape* shape = new rp3d::CapsuleShape(radius, height);
					AddShape(*comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
				}
				break;
			default:
				ERR("Wrong type of collision shape: %i", (int32_t)shapeType);
				break;
			}
		}
	}

	return (uint32_t)(t_data - data);
}

bool CollisionSystem::IsActive(Entity e)
{
	GET_COMPONENT(false);
	return comp.body->isActive();
}

void CollisionSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(void());
	comp.body->setIsActive(active);
}

int32_t CollisionSystem::AddBoxCollider(Entity e, Vector3 pos, Quaternion rot, float mass, Vector3 halfSize, float margin)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::BoxShape(halfSize, margin == 0 ? rp3d::OBJECT_MARGIN : margin);
	return AddShape(comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
}

int32_t CollisionSystem::AddSphereCollider(Entity e, Vector3 pos, Quaternion rot, float mass, float radius)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::SphereShape(radius);
	return AddShape(comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
}

int32_t CollisionSystem::AddConeCollider(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height, float margin)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::ConeShape(radius, height, margin == 0 ? rp3d::OBJECT_MARGIN : margin);
	return AddShape(comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
}

int32_t CollisionSystem::AddCylinderCollider(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height, float margin)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::CylinderShape(radius, height, margin == 0 ? rp3d::OBJECT_MARGIN : margin);
	return AddShape(comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
}

int32_t CollisionSystem::AddCapsuleCollider(Entity e, Vector3 pos, Quaternion rot, float mass, float radius, float height)
{
	GET_COMPONENT(-1);
	auto shape = new rp3d::CapsuleShape(radius, height);
	return AddShape(comp, pos, rot, mass, shape, CollisionStorageType::LOCAL);
}

void CollisionSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<CollisionSystem>("CollisionSystem")
		.addFunction("IsActive", &CollisionSystem::IsActive)
		.addFunction("SetActive", &CollisionSystem::SetActive)

		.addFunction("AddBoxCollider", &CollisionSystem::AddBoxCollider)
		.addFunction("AddSphereCollider", &CollisionSystem::AddSphereCollider)
		.addFunction("AddConeCollider", &CollisionSystem::AddConeCollider)
		.addFunction("AddCylinderCollider", &CollisionSystem::AddCylinderCollider)
		.addFunction("AddCapsuleCollider", &CollisionSystem::AddCapsuleCollider)

		.addFunction("AddComponent", &CollisionSystem::_AddComponent)
		.addFunction("DeleteComponent", &CollisionSystem::DeleteComponent)
		.addFunction("HasComponent", &CollisionSystem::HasComponent)
		.endClass();
}