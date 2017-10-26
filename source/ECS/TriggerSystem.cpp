#include "stdafx.h"
#include "TriggerSystem.h"
#include "World.h"

TriggerSystem::TriggerSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();
	collisionSystem = world->GetCollisionSystem();
	typeMgr = world->GetTypeMgr();
	nameMgr = world->GetNameMgr();

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

void TriggerSystem::CheckOverlaps(float dt, uint32_t frameID)
{
	for(auto& i: *components.data())
	{
		if(!i.active)
			continue;

		Entity trigEnt = i.get_entity();

		int32_t overlapsCount = i.object->getNumOverlappingObjects();
		if( overlapsCount == 0 )
		{
			if( i.overlappingMap.empty() )
				continue;

			bool endTouchAll = false;
			for(auto& it: i.overlappingMap)
			{
				if( i.reactionDelay <= it.second.time )
				{
					if( i.endTouch )
						LUA_CALL((*i.endTouch)(trigEnt, Entity(it.first), it.second.time),);
					endTouchAll = true;
				}
			}

			if( i.endTouchAll && endTouchAll )
				LUA_CALL((*i.endTouchAll)(trigEnt),);

			i.overlappingMap.clear();
		}
		else
		{
			for(int32_t j = 0; j < overlapsCount; j++)
			{
				auto overlappedObj = i.object->getOverlappingObject(j);
				Entity ent = overlappedObj->getUserIndex();
				
				if(!FilterEntity(i, ent))
					continue;

				auto it = i.overlappingMap.find(ent);				
				if( it == i.overlappingMap.end() )
				{
					i.overlappingMap.insert(make_pair(ent, OverlappedEntity(dt, frameID)));
					if( i.startTouch && i.reactionDelay <= dt )
						LUA_CALL((*i.startTouch)(trigEnt, ent, dt),);
				}
				else
				{
					const float newTime = it->second.time + dt;
					if( i.startTouch && i.reactionDelay > it->second.time && i.reactionDelay <= newTime )
						LUA_CALL((*i.startTouch)(trigEnt, ent, newTime),);

					it->second.time = newTime;
					it->second.frameID = frameID;
				}
			}

			auto overlapIt = i.overlappingMap.begin();
			while(overlapIt != i.overlappingMap.end())
			{
				if( overlapIt->second.frameID != frameID )
				{
					if( i.endTouch && i.reactionDelay <= overlapIt->second.time )
						LUA_CALL((*i.endTouch)(trigEnt, Entity(overlapIt->first), overlapIt->second.time),);
					overlapIt = i.overlappingMap.erase(overlapIt);
				}
				else
				{
					++overlapIt;
				}
			}
		}
	}
}

bool TriggerSystem::FilterEntity(TriggerComponent& comp, Entity ent)
{
	switch(comp.filter)
	{
	case TriggerFilterType::FilterNone:
		return true;

	case TriggerFilterType::FilterByType:
		return (typeMgr->GetType(ent) == comp.filterString);
	case TriggerFilterType::FilterByTypeInv:
		return (typeMgr->GetType(ent) != comp.filterString);

	case TriggerFilterType::FilterByName:
		return (nameMgr->GetName(ent) == comp.filterString);
	case TriggerFilterType::FilterByNameInv:
		return (nameMgr->GetName(ent) != comp.filterString);

	case TriggerFilterType::FilterByNamePart:
		return (nameMgr->GetName(ent).find(comp.filterString) != string::npos);
	case TriggerFilterType::FilterByNamePartInv:
		return (nameMgr->GetName(ent).find(comp.filterString) == string::npos);
	}
	return false;
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
	res->reactionDelay = 0;

	res->overlappingMap.reserve(10);

	int32_t collisionGroup = CollisionGroups::Trigger;
	int32_t collisionMask = CollisionGroups::All & ~CollisionGroups::Static;
	collisionMask &= ~CollisionGroups::Debris;

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
	res->object->setUserIndex(e);
	res->object->setCollisionShape(collision);
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
	_DELETE(comp->startTouch);
	_DELETE(comp->endTouch);
	_DELETE(comp->endTouchAll);
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

void TriggerSystem::UpdateState(Entity e)
{
	GET_COMPONENT(void());
	// TODO: optimize? 
	// dynamicsWorld->resetRigidBody(comp.body)
	dynamicsWorld->removeCollisionObject(comp.object);
	
	int32_t collisionGroup = CollisionGroups::Trigger;
	int32_t collisionMask = CollisionGroups::All & ~CollisionGroups::Static;
	collisionMask &= ~CollisionGroups::Debris;

	auto collision = collisionSystem->GetCollision(e);
	if(!collision)
		WRN("Collision component must be set before Trigger component");
	else
	{
		comp.object->setCollisionShape(collision);
		auto collisionComp = collisionSystem->GetComponent(e);
		collisionGroup = collisionComp->collisionGroup;
		collisionMask = collisionComp->collisionMask;
	}

	dynamicsWorld->addCollisionObject(comp.object, collisionGroup, collisionMask);
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

void TriggerSystem::SetFuncStartTouch(Entity e, LuaRef func)
{
	GET_COMPONENT(void());
	_DELETE(comp.startTouch);
	if(func.isFunction())
		comp.startTouch = new LuaRef(func);
}

void TriggerSystem::SetFuncEndTouch(Entity e, LuaRef func)
{
	GET_COMPONENT(void());
	_DELETE(comp.endTouch);
	if(func.isFunction())
		comp.endTouch = new LuaRef(func);
}

void TriggerSystem::SetFuncEndTouchAll(Entity e, LuaRef func)
{
	GET_COMPONENT(void());
	_DELETE(comp.endTouchAll);
	if(func.isFunction())
		comp.endTouchAll = new LuaRef(func);
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

float TriggerSystem::GetTouchingTime(Entity e, Entity touching)
{
	GET_COMPONENT(0.0f);
	auto it = comp.overlappingMap.find(touching);
	if( it == comp.overlappingMap.end() )
		return 0.0f;
	return it->second.time;
}

float TriggerSystem::GetDelay(Entity e)
{
	GET_COMPONENT(0.0f);
	return comp.reactionDelay;
}

void TriggerSystem::SetDelay(Entity e, float d)
{
	GET_COMPONENT(void());
	comp.reactionDelay = d;
}

void TriggerSystem::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<TriggerSystem>("TriggerSystem")
		.addFunction("IsActive", &TriggerSystem::IsActive)
		.addFunction("SetActive", &TriggerSystem::SetActive)

		.addFunction("UpdateState", &TriggerSystem::UpdateState)

		.addFunction("GetDelay", &TriggerSystem::GetDelay)
		.addFunction("SetDelay", &TriggerSystem::SetDelay)

		.addFunction("GetFilterType", &TriggerSystem::GetFilterType)
		.addFunction("SetFilterType", &TriggerSystem::SetFilterType)
		.addFunction("GetFilterString", &TriggerSystem::GetFilterString)
		.addFunction("SetFilterString", &TriggerSystem::SetFilterString)

		.addFunction("SetFuncStartTouch", &TriggerSystem::SetFuncStartTouch)
		.addFunction("SetFuncEndTouch", &TriggerSystem::SetFuncEndTouch)
		.addFunction("SetFuncEndTouchAll", &TriggerSystem::SetFuncEndTouchAll)

		.addFunction("GetTouchingTime", &TriggerSystem::GetTouchingTime)
		.addFunction("IsTouching", &TriggerSystem::IsTouching)

		.addFunction("AddComponent", &TriggerSystem::_AddComponent)
		.addFunction("DeleteComponent", &TriggerSystem::DeleteComponent)
		.addFunction("HasComponent", &TriggerSystem::HasComponent)
		.endClass();
}