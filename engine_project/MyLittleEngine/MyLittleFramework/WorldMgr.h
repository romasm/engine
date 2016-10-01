#pragma once
#include "stdafx.h"
#include "DataTypes.h"
#include "World.h"

namespace EngineCore
{
	class WorldMgr
	{
	public:
		WorldMgr();
		~WorldMgr();
		
		inline static WorldMgr* Get(){return m_instance;}

		//World* GetWorld(UINT id);

		World* CreateWorld();
		World* OpenWorld(string filename);
		void CloseWorldByID(UINT id);
		void CloseWorld(World* wrld){CloseWorldByID(wrld->GetID());}

		inline World* GetWorld(UINT id) 
		{
			auto it = m_worldsMap.find(id);
			if(it == m_worldsMap.end())
				return nullptr;
			return it->second;
		}

		void UpdateWorlds();

		void Close();

		static WorldMgr* GetWorldMgr(){return WorldMgr::Get();}

		inline void PostStMeshesReload() 
		{
			for(auto& it: m_worldsMap)
				it.second->PostStMeshesReload();
		}

		static void UpdateLuaFuncs() 
		{
		#ifdef _DEV
			for(auto& it: m_instance->m_worldsMap)
				it.second->UpdateLuaFuncs();
		#endif
		}
	
		static void RegLuaClass();

	private:
		static WorldMgr *m_instance;
		std::map<UINT, World*> m_worldsMap;
		UINT nextID;
	};
}