#include "stdafx.h"
#include "TriggerSystem.h"
#include "World.h"

TriggerSystem::TriggerSystem(BaseWorld* w, btDiscreteDynamicsWorld* dynamicsW, uint32_t maxCount)
{	
	world = w;
	transformSystem = world->GetTransformSystem();
	collisionSystem = world->GetCollisionSystem();
	scriptSystem = world->GetScriptSystem();
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
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.active)
			continue;

		const Entity e = i.get_entity();
		auto scriptComp = scriptSystem->GetComponent(e);
		if(!scriptComp)
			continue;

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
					if( i.endTouch.isFunction() )
						LUA_CALL(i.endTouch(scriptComp->classInstanceRef, Entity(it.first), it.second.time),);
					endTouchAll = true;
				}
			}

			if( i.endTouchAll.isFunction() && endTouchAll )
				LUA_CALL(i.endTouchAll(scriptComp->classInstanceRef),);

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
					if( i.startTouch.isFunction() && i.reactionDelay <= dt )
						LUA_CALL(i.startTouch(scriptComp->classInstanceRef, ent, dt),);
				}
				else
				{
					const float newTime = it->second.time + dt;
					if( i.startTouch.isFunction() && i.reactionDelay > it->second.time && i.reactionDelay <= newTime )
						LUA_CALL(i.startTouch(scriptComp->classInstanceRef, ent, newTime),);

					it->second.time = newTime;
					it->second.frameID = frameID;
				}
			}

			auto overlapIt = i.overlappingMap.begin();
			while(overlapIt != i.overlappingMap.end())
			{
				if( overlapIt->second.frameID != frameID )
				{
					if( i.endTouch.isFunction() && i.reactionDelay <= overlapIt->second.time )
						LUA_CALL(i.endTouch(scriptComp->classInstanceRef, Entity(overlapIt->first), overlapIt->second.time),);
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
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

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
	int32_t collisionMask = CollisionGroups::AllNoSpecial & ~CollisionGroups::Static;
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

	auto scriptComp = scriptSystem->GetComponent(e);
	if(!scriptComp)
		ERR("Can\'t update trigger callbacks, script component needed!");
	else
		_UpdateCallbacks(res, scriptComp);

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
	comp->startTouch = LuaRef(LSTATE);
	comp->endTouch = LuaRef(LSTATE);
	comp->endTouchAll = LuaRef(LSTATE);
}

void TriggerSystem::CopyComponent(Entity src, Entity dest)
{
	auto copyBuffer = world->GetCopyBuffer();

	if( !Serialize(src, copyBuffer) )
		return;

	Deserialize(dest, copyBuffer);
}

#ifdef _DEV
void TriggerSystem::UpdateLuaFuncs()
{
	for(auto& comp: *components.data())
	{
		Entity e = comp.get_entity();

		auto scriptComp = scriptSystem->GetComponent(e);
		if(!scriptComp)
		{
			ERR("Can\'t update trigger callbacks, script component needed!");
			continue;
		}

		_UpdateCallbacks(&comp, scriptComp);
	}
}
#endif

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
	int32_t collisionMask = CollisionGroups::AllNoSpecial & ~CollisionGroups::Static;
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

void TriggerSystem::UpdateCallbacks(Entity e)
{
	GET_COMPONENT(void());

	auto scriptComp = scriptSystem->GetComponent(e);
	if(!scriptComp)
	{
		ERR("Can\'t update trigger callbacks, script component needed!");
		return;
	}

	_UpdateCallbacks(&comp, scriptComp);
}

void TriggerSystem::_UpdateCallbacks(TriggerComponent* comp, ScriptComponent* script)
{
	LuaRef func0 = scriptSystem->GetLuaFunction(*script, TRIGGER_FUNC_START);
	if(func0.isFunction())
		comp->startTouch = func0;

	LuaRef func1 = scriptSystem->GetLuaFunction(*script, TRIGGER_FUNC_END);
	if(func1.isFunction())
		comp->endTouch = func1;

	LuaRef func2 = scriptSystem->GetLuaFunction(*script, TRIGGER_FUNC_ENDALL);
	if(func2.isFunction())
		comp->endTouchAll = func2;
}

uint32_t TriggerSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)
	
	uint8_t* t_data = data;

	*(bool*)t_data = comp.active;
	t_data += sizeof(bool);

	*(int32_t*)t_data = (int32_t)comp.filter;
	t_data += sizeof(int32_t);

	uint32_t dummySize = 0;
	StringSerialize(comp.filterString, &t_data, &dummySize);

	*(float*)t_data = (float)comp.filter;
	t_data += sizeof(float);
	
	return (uint32_t)(t_data - data);
}

uint32_t TriggerSystem::Deserialize(Entity e, uint8_t* data)
{
	auto comp = AddComponent(e);
	if(!comp)
		return 0; // TODO must return size
		
	uint8_t* t_data = data;

	SetActive(comp->get_entity(), *(bool*)t_data);
	t_data += sizeof(bool);

	comp->filter = TriggerFilterType(*(int32_t*)t_data);
	t_data += sizeof(int32_t);

	comp->filterString = StringDeserialize(&t_data);

	comp->reactionDelay = *(float*)t_data;
	t_data += sizeof(float);

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
		
		.addFunction("GetTouchingTime", &TriggerSystem::GetTouchingTime)
		.addFunction("IsTouching", &TriggerSystem::IsTouching)

		.addFunction("AddComponent", &TriggerSystem::_AddComponent)
		.addFunction("DeleteComponent", &TriggerSystem::DeleteComponent)
		.addFunction("HasComponent", &TriggerSystem::HasComponent)
		.endClass();
}