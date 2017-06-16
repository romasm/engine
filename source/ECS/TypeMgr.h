#pragma once

#include "Common.h"
#include "ECS_defines.h"
#include "Entity.h"

#define EDITOR_TYPE "_editor_"

namespace EngineCore
{
	struct _luaref
	{
		LuaRef ref;
		_luaref() : ref(LSTATE){}
		_luaref(LuaRef r) : ref(r){}
	};

#define MAX_TYPES_COUNT 256

	class BaseWorld;

	class TypeMgr 
	{
	public:
		TypeMgr(BaseWorld* wld, uint32_t maxCount);

		bool RegType(string& type, LuaRef* constructor = nullptr);
		bool UnregType(string& type);

		LuaRef LuaConstructor(string& type, Entity ent);

		bool SetType(Entity ent, string& type);
		void ClearType(Entity ent);

		inline string GetType(Entity ent) const {return typeOfEntity[ent.index()];}
		inline bool IsThisType(Entity ent, string& type) const {return typeOfEntity[ent.index()] == type;}
		Entity GetFirstByType(string& type);
		Entity GetNextByType();

	#ifdef _DEV
		inline void UpdateLuaFuncs()
		{
			for(auto& it: reged_types)
				if(!it.second.ref.isNil())
					it.second.ref = luaTypesTable[it.first];
		}
	#endif

	private:
		RArray<string> typeOfEntity;
		typedef unordered_multimap<string, Entity> mmap;
		mmap entitiesPerType;

		unordered_map<string, _luaref> reged_types;

		LuaRef luaTypesTable;
		LuaRef luaConstuctor;

		BaseWorld* world;

	public:
		pair<mmap::iterator, mmap::iterator> search_range;
		string search_type;
	};
	
#define INIT_NAMES_COUNT 1024

	class NameMgr 
	{
	public:
		NameMgr(uint32_t maxCount);
		~NameMgr() {}

		bool SetName(Entity ent, string& name);
		void ClearName(Entity ent);
		bool IsNameTaked(string& name);

		inline string GetName(Entity ent) const {return nameOfEntity[ent.index()];}
		Entity GetEntityByName(string& name) const;

	private:
		RArray<string> nameOfEntity;
		unordered_map<string, Entity> namedEntities;
	};
}
