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
			.addFunction("CreateSmallWorld", &WorldMgr::CreateSmallWorld)
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

		BaseWorld::RegLuaClass();
		World::RegLuaClass();
		SmallWorld::RegLuaClass();

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
		PhysicsSystem::RegLuaClass();
		CollisionSystem::RegLuaClass();
		ControllerSystem::RegLuaClass();
		SkeletonSystem::RegLuaClass();

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

void WorldMgr::regWorld(BaseWorld* world)
{
	m_worldsMap.insert(make_pair(world->GetID(), world));
	nextID++;
}

World* WorldMgr::CreateWorld()
{
	World* world = new World(nextID);
	regWorld(world);

	if(!world->Init())
	{
		CloseWorldByID(world->GetID());
		return nullptr;
	}
	return world;
}

SmallWorld* WorldMgr::CreateSmallWorld()
{
	SmallWorld* world = new SmallWorld(nextID);
	regWorld(world);

	if(!world->Init())
	{
		CloseWorldByID(world->GetID());
		return nullptr;
	}

	return world;
}

World* WorldMgr::OpenWorld(string filename)
{
	World* world = new World(nextID);
	regWorld(world);

	if(!world->Init(filename))
	{
		CloseWorldByID(world->GetID());
		return nullptr;
	}
	return world;
}

void WorldMgr::CloseWorldByID(uint32_t id)
{
	auto i = m_worldsMap.find(id);
	if(i == m_worldsMap.end())
	{
		ERR("World with id %u doesn\'t exist!", id);
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