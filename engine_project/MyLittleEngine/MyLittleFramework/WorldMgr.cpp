#include "stdafx.h"
#include "WorldMgr.h"
#include "Common.h"
#include "Render.h"

using namespace EngineCore;

WorldMgr *WorldMgr::m_instance = nullptr;

void WorldMgr::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginNamespace("Core")
			.addFunction("UpdateLuaFuncs", &WorldMgr::UpdateLuaFuncs)
		.endNamespace();

	getGlobalNamespace(LSTATE)
		.addFunction("GetWorldMgr", &WorldMgr::GetWorldMgr)
		.beginClass<WorldMgr>("WorldMgr")
			.addFunction("CreateWorld", &WorldMgr::CreateWorld)
			.addFunction("OpenWorld", &WorldMgr::OpenWorld)
			.addFunction("CloseWorld", &WorldMgr::CloseWorld)
			.addFunction("CloseWorldByID", &WorldMgr::CloseWorldByID)
		.endClass();
}

WorldMgr::WorldMgr()
{
	if(!m_instance)
	{
		m_instance = this;
		nextID = 0;

		RegLuaClass();
		World::RegLuaClass();
		ScenePipeline::RegLuaClass();

		TransformSystem::RegLuaClass();
		VisibilitySystem::RegLuaClass();
		EarlyVisibilitySystem::RegLuaClass();
		ScriptSystem::RegLuaClass();
		StaticMeshSystem::RegLuaClass();
		CameraSystem::RegLuaClass();
		EnvProbSystem::RegLuaClass();
		LightSystem::RegLuaClass();
		GlobalLightSystem::RegLuaClass();
		LineGeometrySystem::RegLuaClass();
		ControllerSystem::RegLuaClass();

		TransformControls::RegLuaClass();

		RegLuaClassEntity();
	}
	else
	{
		ERR("Повтороное создание WorldMgr");
	}
}

WorldMgr::~WorldMgr()
{
	Close();
	m_instance = nullptr;
}

World* WorldMgr::CreateWorld()
{
	World* world = new World();
	if(!world->Init())
		return nullptr;

	world->SetID(nextID);
	m_worldsMap.insert(make_pair(nextID, world));
	nextID++;

	return world;
}

World* WorldMgr::OpenWorld(string filename)
{
	World* world = new World();
	if(!world->Init(filename))
		return nullptr;

	world->SetID(nextID);
	m_worldsMap.insert(make_pair(nextID, world));
	//if(worldID)*worldID = nextID;
	nextID++;

	return world;
}

void WorldMgr::CloseWorldByID(UINT id)
{
	auto i = m_worldsMap.find(id);
	if(i == m_worldsMap.end())
	{
		ERR("Мира с идентификатором %i не существует", int(id));
		return;
	}
	_CLOSE(i->second);
	m_worldsMap.erase(i);
}

void WorldMgr::UpdateWorlds()
{
	for(auto& it: m_worldsMap)
	{
		it.second->Frame();
	}
}

void WorldMgr::Close()
{
	for(auto& it: m_worldsMap)
	{
		_CLOSE(it.second);
	}
	m_worldsMap.clear();
	nextID = 0;
}