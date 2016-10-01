#pragma once

#include "../Common.h"
#include "ECS_defines.h"
#include "Entity.h"

#define ENT_TYPE_NONE			0x000

#define ENT_TYPE_MODEL			0x001
#define ENT_TYPE_LIGHT			0x002
#define ENT_TYPE_CAMERA			0x003
#define ENT_TYPE_GLOBAL_LIGHT	0x004

#define ENT_LIGHT_MESH PATH_EDITOR_MESHES "pointlight" EXT_STATIC
#define ENT_LIGHT_MESH_MAT PATH_EDITOR_MATERIAL "pointlight" EXT_MATERIAL

namespace EngineCore
{
	struct _luaref
	{
		LuaRef ref;
		_luaref() : ref(LSTATE){}
		_luaref(LuaRef r) : ref(r){}
	};

#define MAX_TYPES_COUNT 256

	class World;

	class TypeMgr 
	{
	public:
		TypeMgr(World* wld);

		bool RegType(string& type, LuaRef* constructor = nullptr)
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
		bool UnregType(string& type)
		{
			auto it = reged_types.find(type);
			if(it == reged_types.end())
				return false;

			auto range = entitiesPerType.equal_range(type);
			for(auto it = range.first; it != range.second; it++)
				typeOfEntity[it->second.index()] = "";
			entitiesPerType.erase(range.first, range.second);

			reged_types.erase(it);
		}

		LuaRef LuaConstructor(string& type, Entity ent);

		bool SetType(Entity ent, string& type)
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
		void ClearType(Entity ent)
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

		inline string GetType(Entity ent) const {return typeOfEntity[ent.index()];}
		Entity GetFirstByType(string& type)
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
		Entity GetNextByType()
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

	#ifdef _DEV
		void UpdateLuaFuncs()
		{
			for(auto& it: reged_types)
				if(!it.second.ref.isNil())
					it.second.ref = luaTypesTable[it.first];
		}
	#endif

	private:
		SArray<string, ENTITY_COUNT> typeOfEntity;
		typedef unordered_multimap<string, Entity> mmap;
		mmap entitiesPerType;

		unordered_map<string, _luaref> reged_types;

		LuaRef luaTypesTable;
		LuaRef luaConstuctor;

		World* world;

	public:
		pair<mmap::iterator, mmap::iterator> search_range;
		string search_type;
	};
	
#define INIT_NAMES_COUNT 1024

	class NameMgr 
	{
	public:
		NameMgr()
		{
			nameOfEntity.resize(ENTITY_COUNT);
			nameOfEntity.assign("");
			namedEntities.reserve(INIT_NAMES_COUNT);
		}
		~NameMgr() {}

		bool SetName(Entity ent, string& name)
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

		void ClearName(Entity ent)
		{
			if(ent.isnull())
				return;

			auto& nameRef = nameOfEntity[ent.index()];
			if( nameRef.empty() )
				return;
			namedEntities.erase(nameRef);
			nameRef = "";
		}

		bool IsNameTaked(string& name) 
		{
			auto it = namedEntities.find(name);
			if(it == namedEntities.end())
				return false;
			else
				return true;
		}

		inline string GetName(Entity ent) const {return nameOfEntity[ent.index()];}
		Entity GetEntityByName(string& name) const 
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

	private:
		SArray<string, ENTITY_COUNT> nameOfEntity;
		unordered_map<string, Entity> namedEntities;
	};
}
