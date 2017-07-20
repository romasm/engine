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
		
		World* CreateWorld();
		SmallWorld* CreateSmallWorld();

		World* OpenWorld(string filename);
		void CloseWorldByID(UINT id);
		void CloseWorld(BaseWorld* wrld){CloseWorldByID(wrld->GetID());}

		inline BaseWorld* GetWorld(UINT id) 
		{
			auto it = m_worldsMap.find(id);
			if(it == m_worldsMap.end())
				return nullptr;
			return it->second;
		}

		void UpdateWorlds();

		void Close();

		static WorldMgr* GetWorldMgr(){return WorldMgr::Get();}

	#ifdef _EDITOR
		inline void PostMeshesReload() 
		{
			for(auto& it: m_worldsMap)
				it.second->PostMeshesReload();
		}
	#endif

		static void UpdateLuaFuncs() 
		{
		#ifdef _DEV
			for(auto& it: m_instance->m_worldsMap)
				it.second->UpdateLuaFuncs();
		#endif
		}

		static void RawInput(RawInputData& data) 
		{
			for(auto& it: m_instance->m_worldsMap)
				it.second->RawInput(data);
		}
	
		static void RegLuaClass();

	private:
		void regWorld(BaseWorld* world);

		static WorldMgr *m_instance;
		std::map<UINT, BaseWorld*> m_worldsMap;
		UINT nextID;
	};
}