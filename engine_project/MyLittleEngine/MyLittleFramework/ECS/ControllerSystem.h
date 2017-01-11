#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "ScriptSystem.h"
#include "InputCodes.h"

namespace EngineCore
{
	enum MouseEvents
	{
		LEFT = 0,
		RIGHT,
		MIDDLE,
		MOVE_X,
		MOVE_Y,
		WHEEL,
		COUNT
	};

	struct KeyMap
	{
		string keyboardEvents[eKeyCodes::KEY_MAX];
		string mouseEvents[MouseEvents::COUNT];
		//string gamepadEvents[];
	};

	struct FuncMap
	{
		LuaRef* keyboardEvents[eKeyCodes::KEY_MAX];
		LuaRef* mouseEvents[MouseEvents::COUNT];		

		FuncMap()
		{
			for(uint16_t i = 0; i < eKeyCodes::KEY_MAX; i++)
				keyboardEvents[i] = nullptr;
			for(uint16_t i = 0; i < MouseEvents::COUNT; i++)
				mouseEvents[i] = nullptr;
		}
	};

	class Controller
	{
		friend class ControllerSystem;
		ENTITY_IN_COMPONENT	
	public:
		bool active;
		FuncMap* funcMap;
		LuaRef classInstanceRef;
		string keyMapName;

		Controller() : active(false), funcMap(nullptr), classInstanceRef(LSTATE) {}
	};

	class BaseWorld;

	// Lua func: PlayerClass:onEvent(key, pressed, x, y, z)
	class ControllerSystem
	{
		struct KeyboardState
		{
			bool isPressed[eKeyCodes::KEY_MAX];

			KeyboardState()
			{
				for(uint16_t i = 0; i < eKeyCodes::KEY_MAX; i++)
					isPressed[i] = false;
			}
		};

	public:
		ControllerSystem(BaseWorld* w);
		~ControllerSystem()
		{
			for(auto& i: components)
			{
				for(uint16_t j = 0; j < eKeyCodes::KEY_MAX; j++)
					_DELETE(i.second.funcMap->keyboardEvents[j]);
				for(uint16_t j = 0; j < MouseEvents::COUNT; j++)
					_DELETE(i.second.funcMap->mouseEvents[j]);
				_DELETE(i.second.funcMap);
			}
			components.clear();

			for(auto& i: keyMaps)
				_DELETE(i.second);
			keyMaps.clear();
		}

		void AddComponent(Entity e, string keyMapName);

		void DeleteComponent(Entity e)
		{
			Controller& comp = GetComponent(e);
			comp.active = false;
			for(uint16_t j = 0; j < eKeyCodes::KEY_MAX; j++)
				_DELETE(comp.funcMap->keyboardEvents[j]);
			for(uint16_t j = 0; j < MouseEvents::COUNT; j++)
				_DELETE(comp.funcMap->mouseEvents[j]);
			_DELETE(comp.funcMap);
			components.erase(e.index());
		}
		bool HasComponent(Entity e) const {return components.find(e.index()) != components.end();}
		size_t ComponentsCount() {return components.size();}

		inline Controller& GetComponent(Entity e)
		{
			return components[e.index()];
		}

		void RawInput(RawInputData& data);

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
		void SendMouseEvent(MouseEvents me, bool pressed, int32_t d);
		KeyMap* GetKeyMap(string& keyMapName);
		bool AttachLuaFuncs(Entity e, Controller& comp, ScriptComponent& script);

		unordered_map<uint32_t, Controller> components;

		unordered_map<string, KeyMap*> keyMaps;

		KeyboardState lastKeyboardState;

		BaseWorld* world;
		ScriptSystem* scriptSys;

		static unordered_map<string, uint32_t> keyboardMap;
		static unordered_map<string, uint32_t> mouseMap;
		void FillControlMap();
	};
}