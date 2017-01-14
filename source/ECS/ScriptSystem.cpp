#include "stdafx.h"
#include "ScriptSystem.h"
#include "World.h"

using namespace EngineCore;

ScriptSystem::ScriptSystem(BaseWorld* w, uint32_t maxCount)
{	
	world = w;
	typeMgr = w->GetTypeMgr();
	
	maxCount = min(maxCount, ENTITY_COUNT);
	components.create(maxCount);
}

ScriptComponent* ScriptSystem::AddComponent(Entity e, string& className)
{
	ScriptComponent* res = components.add(e.index());
	res->parent = e;
		
	res->classInstanceRef = typeMgr->LuaConstructor(className, e);
	if(!res->classInstanceRef.isTable())
	{
		ERR("Lua entity class %s instance is wrong type!", className.c_str());
		DeleteComponent(e);
		return nullptr;
	}
	
	initLuaData(*res);

	return res;
}

ScriptComponent* ScriptSystem::AddComponent(Entity e, LuaRef& classInstanceRef)
{
	ScriptComponent* res = components.add(e.index());
	res->parent = e;
	
	if(!classInstanceRef.isTable())
	{
		string className = typeMgr->GetType(e);
		ERR("Lua entity class %s instance is wrong type!", className.c_str());
		DeleteComponent(e);
		return nullptr;
	}
	
	res->classInstanceRef = classInstanceRef;

	initLuaData(*res);

	return res;
}

void ScriptSystem::CopyComponent(Entity src, Entity dest)
{
	auto comp = GetComponent(src);
	if(!comp)
		return;

	string className = typeMgr->GetType(src);
	if(className.empty())
	{
		ERR("Wrong entity type!");
		return;
	}

	auto newComp = AddComponent(dest, className);
	if(!newComp)
		return;

	if(newComp->varArray.size() != comp->varArray.size())
	{
		ERR("Cant init lua params array for copied entity, wrong lua construction code in %s !", className.c_str());
		DeleteComponent(dest);
		return;
	}
	
	for(uint32_t i = 0; i < comp->varArray.size(); i++)
	{
		auto& varName = comp->varArray[i].name;
		switch (comp->varArray[i].type)
		{
		case LUA_TBOOLEAN:
			newComp->classInstanceRef[varName] = comp->classInstanceRef[varName].cast<bool>();
			break;
		case LUA_TNUMBER:
			newComp->classInstanceRef[varName] = comp->classInstanceRef[varName].cast<float>();
			break;
		case LUA_TSTRING:
			newComp->classInstanceRef[varName] = comp->classInstanceRef[varName].cast<string>();
			break;
		}
	}
}

void ScriptSystem::initLuaData(ScriptComponent& comp)
{
	comp.tickFunc = comp.classInstanceRef["onTick"];
	if(!comp.tickFunc.isFunction())
		comp.tickFunc = LuaRef(LSTATE);

	DArray<luaVar> vars;

	Iterator lua_data(comp.classInstanceRef);
	while(!lua_data.isNil())
	{
		if(lua_data.key().isString())
		{
			string varName = lua_data.key().cast<string>();
			if( varName.find("p_") != string::npos )
				vars.push_back(luaVar(varName, lua_data.value().type()));
		}
		++lua_data;
	}

	if(vars.empty())
		return;

	comp.varArray.create(vars.size());
	for(auto& it: vars)
		comp.varArray.push_back(it);
}

void ScriptSystem::Update(float dt)
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.tickFunc.isNil())
			i.tickFunc(i.classInstanceRef, dt);
	}
}

LuaRef ScriptSystem::GetLuaFunction(Entity e, string& funcName)
{
	auto comp = GetComponent(e);
	if(!comp)
		return LuaRef(LSTATE);

	return GetLuaFunction(*comp, funcName);
}

LuaRef ScriptSystem::GetLuaFunction(ScriptComponent& comp, string& funcName)
{
	LuaRef luaFunc = comp.classInstanceRef[funcName.c_str()];
	if(!luaFunc.isFunction())
		return LuaRef(LSTATE);

	return luaFunc;
}

#ifdef _DEV
void ScriptSystem::UpdateLuaFuncs()
{
	for(auto& i: *components.data())
	{
		if(!i.tickFunc.isNil())
			i.tickFunc = i.classInstanceRef["onTick"];
	}
}
#endif

uint32_t ScriptSystem::Serialize(Entity e, uint8_t* data)
{
	auto comp = GetComponent(e);
	if(!comp)
		return 0;

	auto t_data = data;
	uint32_t size = 0;

	uint32_t* size_slot = (uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

	*(uint32_t*)t_data = (uint32_t)comp->varArray.size();
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);

	for(auto& it: comp->varArray)
	{
		uint32_t name_size = (uint32_t)it.name.size();
		*(uint32_t*)t_data = name_size;
		t_data += sizeof(uint32_t);
		size += sizeof(uint32_t);

		memcpy_s(t_data, name_size, it.name.data(), name_size);
		t_data += name_size * sizeof(char);
		size += name_size * sizeof(char);

		*(uint8_t*)t_data = it.type;
		t_data += sizeof(uint8_t);
		size += sizeof(uint8_t);

		switch (it.type)
		{
		case LUA_TBOOLEAN:
			*(bool*)t_data = comp->classInstanceRef[it.name].cast<bool>();
			t_data += sizeof(bool);
			size += sizeof(bool);
			break;
		case LUA_TNUMBER:
			*(float*)t_data = comp->classInstanceRef[it.name].cast<float>();
			t_data += sizeof(float);
			size += sizeof(float);
			break;
		case LUA_TSTRING:
			{
				string var_str = comp->classInstanceRef[it.name].cast<string>();
				uint32_t var_str_size = (uint32_t)var_str.size();
				*(uint32_t*)t_data = var_str_size;
				t_data += sizeof(uint32_t);
				size += sizeof(uint32_t);
				
				memcpy_s(t_data, var_str_size, var_str.data(), var_str_size);
				t_data += var_str_size * sizeof(char);
				size += var_str_size * sizeof(char);
			}
			break;
		}
	}

	*size_slot = size - sizeof(uint32_t);

	return size;
}

uint32_t ScriptSystem::Deserialize(Entity e, uint8_t* data)
{
	auto t_data = data;
	uint32_t size = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);
	size += sizeof(uint32_t);
	
	uint32_t vars_count = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

	string className = typeMgr->GetType(e);
	auto comp = AddComponent(e, className);
	if(!comp)
		return 0;

	if(comp->varArray.size() != vars_count)
		WRN("Lua vars count is wrong in data struct for class %s", className.c_str());

	for(uint32_t i = 0; i < vars_count; i++)
	{
		uint32_t name_size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		string name((char*)t_data, name_size);
		t_data += name_size * sizeof(char);

		uint8_t type = *(uint8_t*)t_data;
		t_data += sizeof(uint8_t);

		auto var_type = comp->classInstanceRef[name].type();
		if( var_type == LUA_TNIL )
		{
			ERR("Lua var %s cant be found in class %s", name.c_str(), className.c_str());
			return size;
		}
		if( var_type != type )
			WRN("Lua var %s type is wrong in data struct for class %s", name.c_str(), className.c_str());

		switch (type)
		{
		case LUA_TBOOLEAN:
			comp->classInstanceRef[name] = *(bool*)t_data;
			t_data += sizeof(bool);
			break;
		case LUA_TNUMBER:
			comp->classInstanceRef[name] = *(float*)t_data;
			t_data += sizeof(float);
			break;
		case LUA_TSTRING:
			{
				uint32_t var_str_size = *(uint32_t*)t_data;
				t_data += sizeof(uint32_t);

				string var_str((char*)t_data, var_str_size);
				t_data += var_str_size * sizeof(char);

				comp->classInstanceRef[name] = var_str;
			}
			break;
		}
	}

	return size;
}