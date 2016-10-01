#include "stdafx.h"
#include "TypeMgr.h"
#include "..\World.h"

using namespace EngineCore;

TypeMgr::TypeMgr(World* wld) :
	luaTypesTable(LSTATE),
	luaConstuctor(LSTATE)
{
	world = wld;

	typeOfEntity.resize(ENTITY_COUNT);
	typeOfEntity.assign("");
	entitiesPerType.reserve(ENTITY_COUNT);
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