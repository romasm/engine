#include "stdafx.h"
#include "CollisionSystem.h"
#include "World.h"

CollisionSystem::CollisionSystem(BaseWorld* w, btDiscreteDynamicsWorld* collisionW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();
	physicsSystem = world->GetPhysicsSystem();

	collisionWorld = collisionW;

	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);

	b_debugDraw = false;
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

		// TODO
		i.dirty = false;
	}
}

#ifdef _DEV
void CollisionSystem::DebugRegToDraw()
{
	if( !b_debugDraw )
		return;

	// TODO
}
#endif

CollisionComponent* CollisionSystem::AddComponent(Entity e)
{
	if(HasComponent(e))
		return &components.getDataById(e.index());

	if( !transformSystem->GetParent(e).isnull() )
		return nullptr;

	CollisionComponent* res = components.add(e.index());
	res->parent = e;
	res->dirty = true;

	//TODO

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
	auto copyBuffer = world->GetCopyBuffer();

	if( !Serialize(src, copyBuffer) )
		return;

	Deserialize(dest, copyBuffer);
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

	//TODO

	return (uint32_t)(t_data - data);
}

uint32_t CollisionSystem::Deserialize(Entity e, uint8_t* data)
{
	auto comp = AddComponent(e);
	if(!comp)
		return 0;
		
	uint8_t* t_data = data;

	//TODO

	return (uint32_t)(t_data - data);
}

bool CollisionSystem::IsActive(Entity e)
{
	GET_COMPONENT(false);
	// TODO
	return true;
}

void CollisionSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(void());
	// TODO
}

void CollisionSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<CollisionSystem>("CollisionSystem")
		.addFunction("IsActive", &CollisionSystem::IsActive)
		.addFunction("SetActive", &CollisionSystem::SetActive)
		
		.addFunction("SetDebugDraw", &CollisionSystem::SetDebugDraw)

		.addFunction("AddComponent", &CollisionSystem::_AddComponent)
		.addFunction("DeleteComponent", &CollisionSystem::DeleteComponent)
		.addFunction("HasComponent", &CollisionSystem::HasComponent)
		.endClass();
}