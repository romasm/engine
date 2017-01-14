#include "stdafx.h"
#include "TypeMgr.h"
#include "World.h"

using namespace EngineCore;

TypeMgr::TypeMgr(BaseWorld* wld, uint32_t maxCount) :
	luaTypesTable(LSTATE),
	luaConstuctor(LSTATE)
{
	world = wld;

	maxCount = min(ENTITY_COUNT, maxCount);

	typeOfEntity.create(maxCount);
	typeOfEntity.resize(maxCount);
	typeOfEntity.assign("");
	entitiesPerType.reserve(maxCount);
	reged_types.reserve(MAX_TYPES_COUNT);
	search_type = "";
}

LuaRef TypeMgr::LuaConstructor(string& type, Entity ent)
{
	LuaRef classTable(LSTATE);
	auto it = reged_types.find(type);
	if(it != reged_types.end())
		classTable = it->second.ref;
	else
		RegType(type, &classTable);
	
	if(classTable.isNil())
	{
		ERR("Cant get lua constuctor for type %s", type.c_str());
		return classTable;
	}

	return luaConstuctor(world, ent, classTable);
}

bool TypeMgr::RegType(string& type, LuaRef* constructor)
{
	if(reged_types.size() >= MAX_TYPES_COUNT)
		return false;

	auto it = reged_types.find(type);
	if(it != reged_types.end())
		return true;

	if(luaTypesTable.isNil())
	{
		luaTypesTable = getGlobal(LSTATE, "EntityTypes");
		luaConstuctor = luaTypesTable["constructor"];
	}

	LuaRef classConstructor = luaTypesTable[type];
	if(!classConstructor.isTable())
	{
		classConstructor = LuaRef(LSTATE);
		WRN("No lua constuctor for type %s", type.c_str());
	}

	if(constructor)
		*constructor = classConstructor;

	reged_types.insert(make_pair(type, _luaref(classConstructor)));
	return true;
}

bool TypeMgr::UnregType(string& type)
{
	auto it = reged_types.find(type);
	if(it == reged_types.end())
		return false;

	auto range = entitiesPerType.equal_range(type);
	for(auto it = range.first; it != range.second; it++)
		typeOfEntity[it->second.index()] = "";
	entitiesPerType.erase(range.first, range.second);

	reged_types.erase(it);
	return true;
}

bool TypeMgr::SetType(Entity ent, string& type)
{
	if(ent.isnull())
		return false;

	if(reged_types.find(type) == reged_types.end())
		if(!RegType(type))
			return false;

	auto& toe = typeOfEntity[ent.index()];
	if(toe.size())
	{
		auto range = entitiesPerType.equal_range(type);
		for(auto it = range.first; it != range.second; it++)
			if(EntIsEq(it->second, ent))
			{
				entitiesPerType.erase(it);
				break;
			}
	}
	toe = type;
	entitiesPerType.insert(make_pair(type, ent));
	return true;
}

void TypeMgr::ClearType(Entity ent)
{
	if(ent.isnull())
		return;
	auto& type = typeOfEntity[ent.index()];
	if(!type.size())
		return;

	auto range = entitiesPerType.equal_range(type);
	for(auto it = range.first; it != range.second; it++)
		if(EntIsEq(it->second, ent))
		{
			entitiesPerType.erase(it);
			break;
		}

	type = "";
}

Entity TypeMgr::GetFirstByType(string& type)
{
	Entity ent;
	ent.setnull();

	if(type.empty())
		return ent;

	search_range = entitiesPerType.equal_range(type);
	if(search_range.first == entitiesPerType.end())
		return ent;
			
	search_type = type;
	return search_range.first->second;
}

Entity TypeMgr::GetNextByType()
{
	Entity ent;
	ent.setnull();

	if(search_range.first == entitiesPerType.end())
		return ent;

	search_range.first++;
	if(search_range.first != search_range.second)
		return search_range.first->second;
	else
		return ent;
}

// NameMgr -------------------------------------

NameMgr::NameMgr(uint32_t maxCount)
{
	maxCount = min(ENTITY_COUNT, maxCount);

	nameOfEntity.create(maxCount);
	nameOfEntity.resize(maxCount);
	nameOfEntity.assign("");
	namedEntities.reserve(maxCount);
}

bool NameMgr::SetName(Entity ent, string& name)
{
	if(ent.isnull())
		return false;

	auto& nameRef = nameOfEntity[ent.index()];

	if(!nameRef.empty())
		namedEntities.erase(nameRef);

	nameRef = name;
	if(!name.empty())
		namedEntities.insert(make_pair(name, ent));
	return true;
}

void NameMgr::ClearName(Entity ent)
{
	if(ent.isnull())
		return;

	auto& nameRef = nameOfEntity[ent.index()];
	if( nameRef.empty() )
		return;
	namedEntities.erase(nameRef);
	nameRef = "";
}

bool NameMgr::IsNameTaked(string& name) 
{
	auto it = namedEntities.find(name);
	if(it == namedEntities.end())
		return false;
	else
		return true;
}

Entity NameMgr::GetEntityByName(string& name) const 
{
	Entity ent;
	ent.setnull();
	if(name.empty())
		return ent;

	auto it = namedEntities.find(name);
	if(it == namedEntities.end())
		return ent;
	return it->second;
}