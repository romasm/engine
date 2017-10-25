#include "stdafx.h"
#include "TriggerSystem.h"
#include "World.h"

TriggerSystem::TriggerSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();
	collisionSystem = world->GetCollisionSystem();

	dynamicsWorld = dynamicsW;

	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
}

TriggerSystem::~TriggerSystem()
{
	for(auto& i: *components.data())
	{
		_DeleteComponent(&i);
	}
}

void TriggerSystem::CheckOverlaps()
{
	for(auto& i: *components.data())
	{
		if(!i.active)
			continue;

		int32_t overlapsCount =  i.object->getNumOverlappingObjects();
		if( overlapsCount == 0 )
		{
			if( i.overlappingMap.empty() )
				continue;

			for(auto& it: i.overlappingMap)
			{
				if(i.endTouch)
					LUA_CALL((*i.endTouch)(it.first),);
			}

			if(i.endTouchAll)
				LUA_CALL((*i.endTouchAll)(it.first),);

			i.overlappingMap.clear();
		}
		else
		{
			for(uint32_t j = 0; j < overlapsCount; j++)
			{
				auto overlappedObj = i.object->getOverlappingObject(j);
				Entity ent = EntityFromInt(overlappedObj->getUserIndex());
				
				// TODO: apply filter to ent

				auto it = i.overlappingMap.find(ent);				
				if( it == i.overlappingMap.end() )
				{
					i.overlappingMap.insert(make_pair(ent, overlappedObj));
					if(i.startTouch)
						LUA_CALL((*i.startTouch)(it.first),);
				}
			}
		}
	}
}

void TriggerSystem::UpdateTransformations()
{
	for(auto& i: *components.data())
	{
		if(!i.dirty)
			continue;

		Entity e = i.get_entity();

		btTransform transform = ToBtTransform( transformSystem->GetTransformW(e) );
		i.object->setWorldTransform( transform );
		
		i.dirty = false;
	}
}

TriggerComponent* TriggerSystem::AddComponent(Entity e)
{
	if(HasComponent(e))
		return &components.getDataById(e.index());

	if( !transformSystem->GetParent(e).isnull() )
		return nullptr;

	TriggerComponent* res = components.add(e.index());
	res->parent = e;
	res->dirty = true;
	res->active = true;
	res->filter = TriggerFilterType::FilterNone;
	res->filterString = "";

	int32_t collisionGroup = CollisionGroups::Trigger;
	int32_t collisionMask = CollisionGroups::All;

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

	res->object = new btGhostObject();
	res->object->setUserIndex(IntFromEntity(e));
	res->object->setCollisionShape(CollisionMgr::GetResourcePtr(CollisionMgr::nullres));
	res->object->setCollisionFlags(res->object->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	
	dynamicsWorld->addCollisionObject(res->object, collisionGroup, collisionMask);

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

void TriggerSystem::_DeleteComponent(TriggerComponent* comp)
{
	dynamicsWorld->removeCollisionObject(comp->object);
	_DELETE(comp->object);
	comp->overlappingMap.clear();
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
	
	return comp.active;
}

void TriggerSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(void());
	if(active)
	{
		if(!comp.active)
		{
			auto collComp = collisionSystem->GetComponent(e);
			if(collComp)
				return;
			comp.active = true;
			dynamicsWorld->addCollisionObject(comp.object, collComp->collisionGroup, collComp->collisionMask);
		}
	}
	else
	{
		if(comp.active)
		{
			comp.active = false;
			dynamicsWorld->removeCollisionObject(comp.object);
		}
	}
}

int32_t TriggerSystem::GetFilterType(Entity e)
{
	GET_COMPONENT(0);
	return (int32_t)comp.filter;
}

void TriggerSystem::SetFilterType(Entity e, int32_t type)
{
	GET_COMPONENT(void());
	comp.filter = (TriggerFilterType)type;
}

string TriggerSystem::GetFilterString(Entity e)
{
	GET_COMPONENT("");
	return comp.filterString;
}

void TriggerSystem::SetFilterString(Entity e, string str)
{
	GET_COMPONENT(void());
	comp.filterString = str;
}

void TriggerSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<TriggerSystem>("TriggerSystem")
		.addFunction("IsActive", &TriggerSystem::IsActive)
		.addFunction("SetActive", &TriggerSystem::SetActive)
		
		.addFunction("AddComponent", &TriggerSystem::_AddComponent)
		.addFunction("DeleteComponent", &TriggerSystem::DeleteComponent)
		.addFunction("HasComponent", &TriggerSystem::HasComponent)
		.endClass();
}