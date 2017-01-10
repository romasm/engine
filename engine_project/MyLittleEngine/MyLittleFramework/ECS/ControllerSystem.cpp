#include "stdafx.h"
#include "ControllerSystem.h"
#include "..\World.h"

using namespace EngineCore;

ControllerSystem::ControllerSystem(BaseWorld* w)
{
	world = w;
	scriptSys = w->GetScriptSystem();

	components.reserve(1);
	keyMaps.reserve(4);
};

void ControllerSystem::Process()
{
	for(auto& i: components)
	{
		if( !world->IsEntityNeedProcess(i.second.get_entity()) )
			continue;

		if(!i.second.active)
			continue;

		// TODO: call funcs from func map
	}
}

void ControllerSystem::RawInput(RawInputData& data)
{
	//LOG("Raw Input");
}
		
void ControllerSystem::AddComponent(Entity e, string keyMapName)
{
	auto scriptComp = scriptSys->GetComponent(e);
	if(!scriptComp)
	{
		ERR("Can\'t add controller component %s, script component needed!", keyMapName.c_str());
		return;
	}

	Controller cntr;
	auto keyMap = GetKeyMap(keyMapName);
	if(!keyMap)
	{
		ERR("Can\'t add controller component %s, can\'t get key map!", keyMapName.c_str());
		return;
	}

	cntr.funcMap = new FuncMap;
	// TODO: fill func map
	// itarate throw keymap
	/*LuaRef func = scriptSys->GetLuaFunction(scriptComp, on[EventName]);
	if(!func.isNil())
		cntr.funcMap[keyID] = new LuaRef(func);
	*/
	components.insert(make_pair(e.index(), cntr));
}

#define GET_COMPONENT(res) auto& it = components.find(e.index());\
	if(it == components.end())	return res;\
	auto comp = it->second;

bool ControllerSystem::IsActive(Entity e)
{
	GET_COMPONENT(false)
	return comp.active;
}

bool ControllerSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(false)
	comp.active = active;
	return true;
}

#ifdef _DEV
void ControllerSystem::UpdateLuaFuncs()
{
	for(auto& i: components)
	{
		if(!i.second.funcMap)
			return;
		
		// TODO: update funcs
		// itarate throw funcmap

	}
}
#endif

KeyMap* ControllerSystem::GetKeyMap(string& keyMapName)
{
	auto& it = keyMaps.find(keyMapName);
	if(it != keyMaps.end())
	{
		return it->second;
	}

	string path(PATH_KEYMAPS);
	path += keyMapName;
	path += ".cfg";

	FileIO file(path);
	auto root = file.Root();
	if(!root)
		return nullptr;

	KeyMap* res = new KeyMap;

	// TODO: fill key map

	keyMaps.insert(make_pair(keyMapName, res));
	return res;
}
