#include "stdafx.h"
#include "ScriptSystem.h"
#include "World.h"

using namespace EngineCore;

ScriptSystem::ScriptSystem(BaseWorld* w, uint32_t maxCount)
{	
	world = w;
	typeMgr = w->GetTypeMgr();
	
	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
}

ScriptComponent* ScriptSystem::AddComponent(Entity e, string& className)
{
	ScriptComponent* res = components.add(e.index());
	res->parent = e;
	res->frameID = 0;
		
	res->classInstanceRef = typeMgr->LuaConstructor(className, e);
	if(!res->classInstanceRef.isTable())
	{
		ERR("Lua entity class %s instance is wrong type!", className.c_str());
		DeleteComponent(e);
		return nullptr;
	}
	
	_UpdateScript(res);

	return res;
}

ScriptComponent* ScriptSystem::AddComponent(Entity e, LuaRef& classInstanceRef)
{
	ScriptComponent* res = components.add(e.index());
	res->parent = e;
	res->frameID = 0;

	if(!classInstanceRef.isTable())
	{
		string className = typeMgr->GetType(e);
		ERR("Lua entity class %s instance is wrong type!", className.c_str());
		DeleteComponent(e);
		return nullptr;
	}
	
	res->classInstanceRef = classInstanceRef;

	_UpdateScript(res);

	return res;
}

void ScriptSystem::CopyComponent(Entity src, Entity dest) // TODO unify
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
		case LUA_TFUNCTION:
			// link on same function, correct?
			newComp->classInstanceRef[varName] = comp->classInstanceRef[varName];
			break;
		}
	}

	_UpdateScript(comp);
}

void ScriptSystem::SendKill(Entity e)
{
	auto comp = GetComponent(e);
	if(!comp)
		return;

	if( comp->frameID > 0 && !comp->killFunc.isNil() )
		LUA_CALL(comp->killFunc(comp->classInstanceRef),);
}

void ScriptSystem::Update(float dt, uint32_t frameID)
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(i.frameID == 0)
		{
			if(!i.spawnFunc.isNil())
				LUA_CALL(i.spawnFunc(i.classInstanceRef),);
		}

		if(!i.tickFunc.isNil())
			LUA_CALL(i.tickFunc(i.classInstanceRef, dt),);

		i.frameID = frameID;
	}
}

void ScriptSystem::UpdateScript(Entity e)
{
	auto comp = GetComponent(e);
	if(!comp)
		return;

	_UpdateScript(comp);
}

void ScriptSystem::_UpdateScript(ScriptComponent* comp)
{
	DArray<luaVar> vars;

	Iterator lua_data(comp->classInstanceRef);
	while(!lua_data.isNil())
	{
		if(lua_data.key().isString())
		{
			string varName = lua_data.key().cast<string>();
			if( varName.find(SCRIPT_USER_DATA_PREFIX) != string::npos )
				vars.push_back(luaVar(varName, lua_data.value().type()));
		}
		++lua_data;
	}

	if(vars.empty())
		return;

	comp->varArray.destroy();
	comp->varArray.create(vars.size());
	for(auto& it: vars)
		comp->varArray.push_back(it);

	comp->tickFunc = GetLuaFunction(*comp, SCRIPT_FUNC_TICK);
	comp->spawnFunc = GetLuaFunction(*comp, SCRIPT_FUNC_SPAWN);
	comp->killFunc = GetLuaFunction(*comp, SCRIPT_FUNC_KILL);
}

LuaRef ScriptSystem::GetLuaFunction(Entity e, string& funcName)
{
	auto comp = GetComponent(e);
	if(!comp)
		return LuaRef(LSTATE);

	return GetLuaFunction(*comp, funcName);
}

LuaRef ScriptSystem::GetLuaFunction(ScriptComponent& comp, const char* funcName)
{
	LuaRef luaFunc = comp.classInstanceRef[funcName];
	if(!luaFunc.isFunction())
		return LuaRef(LSTATE);

	return luaFunc;
}

int32_t ScriptSystem::GetLuaVarsCount(Entity e)
{
	auto comp = GetComponent(e);
	if(!comp)
		return 0;
	return (int32_t)comp->varArray.size();
}

string ScriptSystem::GetLuaVarName(Entity e, int32_t i)
{
	auto comp = GetComponent(e);
	if(!comp)
		return "";
	if( i >= comp->varArray.size() )
		return "";
	return comp->varArray[i].name;
}

#ifdef _DEV
void ScriptSystem::UpdateLuaFuncs()
{
	for(auto& i: *components.data())
	{
		if(!i.tickFunc.isNil())
			i.tickFunc = i.classInstanceRef[SCRIPT_FUNC_TICK];
		if(!i.spawnFunc.isNil())
			i.spawnFunc = i.classInstanceRef[SCRIPT_FUNC_SPAWN];
		if(!i.killFunc.isNil())
			i.killFunc = i.classInstanceRef[SCRIPT_FUNC_KILL];
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
				StringSerialize(var_str, &t_data, &size);
			}
			break;
		case LUA_TFUNCTION:
			{
				string funcBytecode = LuaVM::FunctionSerialize(comp->classInstanceRef[it.name]);
				StringSerialize(funcBytecode, &t_data, &size);
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

	for(uint32_t i = 0; i < vars_count; i++)
	{
		uint32_t name_size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		string name((char*)t_data, name_size);
		t_data += name_size * sizeof(char);

		uint8_t type = *(uint8_t*)t_data;
		t_data += sizeof(uint8_t);

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
			comp->classInstanceRef[name] = StringDeserialize(&t_data);
			break;
		case LUA_TFUNCTION:
			comp->classInstanceRef[name] = LuaVM::FunctionDeserialize(StringDeserialize(&t_data));
			break;
		}
	}

	_UpdateScript(comp);

	return size;
}