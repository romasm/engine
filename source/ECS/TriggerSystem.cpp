#include "stdafx.h"
#include "TriggerSystem.h"
#include "World.h"

TriggerSystem::TriggerSystem(BaseWorld* w, btDiscreteDynamicsWorld* collisionW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();
	collisionSystem = world->GetCollisionSystem();

	collisionWorld = collisionW;

	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);

	b_debugDraw = false;
}

TriggerSystem::~TriggerSystem()
{
	for(auto& i: *components.data())
	{
		_DeleteComponent(&i);
	}
}

void TriggerSystem::UpdateTransformations()
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
void TriggerSystem::DebugRegToDraw()
{
	if( !b_debugDraw )
		return;

	// TODO
}
#endif

TriggerComponent* TriggerSystem::AddComponent(Entity e)
{
	if(HasComponent(e))
		return &components.getDataById(e.index());

	if( !transformSystem->GetParent(e).isnull() )
		return nullptr;

	TriggerComponent* res = components.add(e.index());
	res->parent = e;
	res->dirty = true;

	//TODO

	return res;
}

void TriggerSystem::DeleteComponent(Entity e)
{
	auto comp = GetComponent(e);
	if(!comp) 
		return;

	_DeleteComponent(comp);
	components.remove(e.index());
}

void TriggerSystem::CopyComponent(Entity src, Entity dest)
{
	auto copyBuffer = world->GetCopyBuffer();

	if( !Serialize(src, copyBuffer) )
		return;

	Deserialize(dest, copyBuffer);
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool TriggerSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false);
	return comp.dirty;
}

bool TriggerSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false);
	comp.dirty = true;
	return true;
}

uint32_t TriggerSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)
	
	uint8_t* t_data = data;

	//TODO

	return (uint32_t)(t_data - data);
}

uint32_t TriggerSystem::Deserialize(Entity e, uint8_t* data)
{
	auto comp = AddComponent(e);
	if(!comp)
		return 0;
		
	uint8_t* t_data = data;

	//TODO

	return (uint32_t)(t_data - data);
}

bool TriggerSystem::IsActive(Entity e)
{
	GET_COMPONENT(false);
	// TODO
	return true;
}

void TriggerSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(void());
	// TODO
}

void TriggerSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<TriggerSystem>("TriggerSystem")
		.addFunction("IsActive", &TriggerSystem::IsActive)
		.addFunction("SetActive", &TriggerSystem::SetActive)
		
		.addFunction("SetDebugDraw", &TriggerSystem::SetDebugDraw)

		.addFunction("AddComponent", &TriggerSystem::_AddComponent)
		.addFunction("DeleteComponent", &TriggerSystem::DeleteComponent)
		.addFunction("HasComponent", &TriggerSystem::HasComponent)
		.endClass();
}