#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "ScriptSystem.h"

namespace EngineCore
{
#define USER_DEVICE_KEYBOARD	0
#define USER_DEVICE_MOUSE		1
#define USER_DEVICE_GAMEPAD		2

	class KeyMap
	{
		
	};

	class FuncMap
	{
		
	};

	class Controller
	{
		friend class ControllerSystem;
		ENTITY_IN_COMPONENT	
	public:
		bool active;
		FuncMap* funcMap;

		Controller() : active(false), funcMap(nullptr) {}
	};

	class BaseWorld;

	class ControllerSystem
	{
	public:
		ControllerSystem(BaseWorld* w);
		~ControllerSystem()
		{
			for(auto i: components)
				_DELETE(i.second.funcMap);
			components.clear();

			for(auto i: keyMaps)
				_DELETE(i.second);
			keyMaps.clear();
		}

		void AddComponent(Entity e, string keyMapName);

		void DeleteComponent(Entity e)
		{
			Controller& comp = GetComponent(e);
			comp.active = false;
			comp.funcMap = nullptr;
			components.erase(e.index());
		}
		bool HasComponent(Entity e) const {return components.find(e.index()) != components.end();}
		size_t ComponentsCount() {return components.size();}

		inline Controller& GetComponent(Entity e)
		{
			return components[e.index()];
		}
		
		void Process();

		bool IsActive(Entity e);
		bool SetActive(Entity e, bool active);

		#ifdef _DEV
			void UpdateLuaFuncs();
		#endif

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<ControllerSystem>("ControllerSystem")
					.addFunction("IsActive", &ControllerSystem::IsActive)
					.addFunction("SetActive", &ControllerSystem::SetActive)

					.addFunction("AddComponent", &ControllerSystem::AddComponent)
					.addFunction("DeleteComponent", &ControllerSystem::DeleteComponent)
					.addFunction("HasComponent", &ControllerSystem::HasComponent)
				.endClass();
		}

		ALIGNED_ALLOCATION

	private:
		KeyMap* GetKeyMap(string& keyMapName);

		unordered_map<UINT, Controller> components;

		unordered_map<string, KeyMap*> keyMaps;

		BaseWorld* world;
		ScriptSystem* scriptSys;
	};
}