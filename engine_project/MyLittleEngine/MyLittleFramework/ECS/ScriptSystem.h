#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TypeMgr.h"

namespace EngineCore
{
	struct luaVar
	{
		uint8_t type;
		string name;
		luaVar(){type = LUA_TNIL;}
		luaVar(string& n, uint8_t t)
		{name = n; type = t;}
	};

	struct ScriptComponent
	{
		ENTITY_IN_COMPONENT
		
		LuaRef classInstanceRef;
		LuaRef tickFunc;

		RArray<luaVar> varArray;

		ScriptComponent() : 
			classInstanceRef(LSTATE),
			tickFunc(LSTATE)
		{
			parent.setnull();
		}
	};

	class World;

	class ScriptSystem
	{
	public:
		ScriptSystem(World* w);

		// from c++
		ScriptComponent* AddComponent(Entity e, string& className);
		// from lua
		ScriptComponent* AddComponent(Entity e, LuaRef& classInstanceRef);

		void CopyComponent(Entity src, Entity dest);

		void DeleteComponent(Entity e)
		{
			auto comp = GetComponent(e);
			if(!comp)
				return;
			comp->classInstanceRef = LuaRef(LSTATE);
			comp->tickFunc = LuaRef(LSTATE);
			comp->varArray.destroy();
			components.remove(e.index());
		}
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		inline ScriptComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == ENTITY_COUNT) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
		
		inline LuaRef GetLuaClassInstance(Entity e)
		{
			auto comp = GetComponent(e);
			if(!comp)
				return LuaRef(LSTATE);
			return comp->classInstanceRef;
		}

		void Update(float dt);

	#ifdef _DEV
		void UpdateLuaFuncs();
	#endif

		inline void _AddComponent(Entity e, LuaRef ref) {AddComponent(e, ref);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<ScriptSystem>("ScriptSystem")
					.addFunction("AddComponent", &ScriptSystem::_AddComponent)
					.addFunction("DeleteComponent", &ScriptSystem::DeleteComponent)
					.addFunction("HasComponent", &ScriptSystem::HasComponent)
					.addFunction("GetLuaEntity", &ScriptSystem::GetLuaClassInstance)
				.endClass();
		}
	private:
		void initLuaData(ScriptComponent& comp);

		ComponentSArray<ScriptComponent, ENTITY_COUNT> components;
		World* world;
		TypeMgr* typeMgr;
	};
}