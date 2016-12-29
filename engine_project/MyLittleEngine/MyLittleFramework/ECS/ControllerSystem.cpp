#include "stdafx.h"
#include "ControllerSystem.h"
#include "..\World.h"

using namespace EngineCore;

ControllerSystem::ControllerSystem(BaseWorld* w)
{
	world = w;
	transformSys = w->GetTransformSystem();
};

void ControllerSystem::Process()
{
	for(auto& i: components)
	{
		if( !world->IsEntityNeedProcess(i.second->get_entity()) )
			continue;

		if(!i.second->active) continue;

		TransformComponent* transf = transformSys->GetComponent(i.second->get_entity());
		i.second->transform = transf->localMatrix;

		i.second->Process();

		transf->localMatrix = i.second->transform;
		world->SetDirty(i.second->get_entity());
	}
}

#define GET_COMPONENT(res) auto& it = components.find(e.index());\
	if(it == components.end())	return res;\
	auto comp = it->second;

void ControllerSystem::SendInput(Entity e, ControllerComands cmd, float param1, float param2)
{
	GET_COMPONENT(void())
	if(!comp->active) return;
	comp->GetInput(cmd, param1, param2);
}

void ControllerSystem::SendInputToAll(ControllerComands cmd, float param1, float param2)
{
	for(auto& i: components)
	{
		if(!i.second->active) continue;
		i.second->GetInput(cmd, param1, param2);
	}
}

bool ControllerSystem::IsActive(Entity e)
{
	GET_COMPONENT(false)
	return comp->active;
}

bool ControllerSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(false)
	comp->active = active;
	return true;
}
