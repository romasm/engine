#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"

namespace EngineCore
{
	enum ControllerComands
	{
		CC_NOPE						= 0x00,
		CC_DELTA_ROT				= 0x01,		
		CC_FORWARD_START			= 0x02,
		CC_FORWARD_END				= 0x03,		
		CC_BACK_START				= 0x04,
		CC_BACK_END					= 0x05,		
		CC_LEFT_START				= 0x06,
		CC_LEFT_END					= 0x07,		
		CC_RIGHT_START				= 0x08,
		CC_RIGHT_END				= 0x09,		
		CC_UP_START					= 0x0A,
		CC_UP_END					= 0x0B,
		CC_DOWN_START				= 0x0C,
		CC_DOWN_END					= 0x0D,
		CC_MOVE_SPEED_CHANGE		= 0x0E
	};

	class Controller
	{
		friend class ControllerSystem;
		ENTITY_IN_COMPONENT	
	public:
		Controller()
		{}

		bool active;
		XMMATRIX transform;
		BaseWorld* world;

		ALIGNED_ALLOCATION

	protected:
		virtual void Process() = 0;
		virtual void GetInput(ControllerComands cmd, float param1, float param2) = 0;
	};

	class BaseWorld;

	class ControllerSystem
	{
	public:
		ControllerSystem(BaseWorld* w);
		~ControllerSystem()
		{
			for(auto i: components)
			{
				_DELETE(i.second)
			}
		}

		void AddComponent(Entity e, Controller* D)
		{
			D->world = world;
			D->parent = e;
			components.insert(make_pair(e.index(), D));
		}
		void DeleteComponent(Entity e)
		{
			Controller* comp = GetComponent(e);
			_DELETE(comp)
			components.erase(e.index());
		}
		bool HasComponent(Entity e) const {return components.find(e.index()) != components.end();}
		size_t ComponentsCount() {return components.size();}

		inline Controller* GetComponent(Entity e)
		{
			return components[e.index()];
		}
		
		void Process();

		bool IsActive(Entity e);
		bool SetActive(Entity e, bool active);

		void SendInput(Entity e, ControllerComands cmd, float param1, float param2);
		void _SendInput(Entity e, UINT cmd, float param1, float param2) {SendInput(e, ControllerComands(cmd), param1, param2);}
		void SendInputToAll(ControllerComands cmd, float param1, float param2);
		void _SendInputToAll(UINT cmd, float param1, float param2) {SendInputToAll(ControllerComands(cmd), param1, param2);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<ControllerSystem>("ControllerSystem")
					.addFunction("IsActive", &ControllerSystem::IsActive)
					.addFunction("SetActive", &ControllerSystem::SetActive)
					.addFunction("SendInput", &ControllerSystem::_SendInput)
					.addFunction("SendInputToAll", &ControllerSystem::_SendInputToAll)

					//.addFunction("AddComponent", &ControllerSystem::AddComponent)
					.addFunction("DeleteComponent", &ControllerSystem::DeleteComponent)
					.addFunction("HasComponent", &ControllerSystem::HasComponent)
				.endClass();
		}

		ALIGNED_ALLOCATION

	private:
		map<UINT, Controller*> components;

		BaseWorld* world;
		TransformSystem* transformSys;
	};
}