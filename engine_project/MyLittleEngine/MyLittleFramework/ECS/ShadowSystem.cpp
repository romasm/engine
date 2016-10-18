#include "stdafx.h"
#include "ShadowSystem.h"
#include "LightSystem.h"
#include "../World.h"

using namespace EngineCore;

ShadowSystem::ShadowSystem(World* world)
{
	shadow_casters = 0;

	earlyVisibilitySys = world->GetEarlyVisibilitySystem();
	auto frustum_mgr = world->GetFrustumMgr();
	frustums = &frustum_mgr->camDataArray;
}

void ShadowSystem::SetLightSys(LightSystem* ls)
{
	lightSys = ls;
}

void ShadowSystem::initShadowmap(ShadowComponent* comp)
{
	comp->dirty = true;
	comp->render_mgr = new ShadowRenderMgr();
	comp->vp_buf = Buffer::CreateConstantBuffer(Render::Device(), sizeof(XMMATRIX), false);
}

void ShadowSystem::Update()
{
	for(auto& i: *components.data())
	{
		i.render_mgr->ZeroMeshgroups();

		if(!i.dirty)
			continue;
		
		XMMATRIX view;
		XMVECTOR pos;
		view = lightSys->GetView(i.get_entity(), i.num, &pos);
		i.view = XMMatrixTranspose(view);
		i.view_proj = i.proj * i.view;

		Render::UpdateSubresource(i.vp_buf, 0, NULL, &i.view_proj, 0, 0);

		i.localFrustum.Transform(i.worldFrustum, TransformationFromViewPos(view, pos));

		i.render_mgr->UpdateCamera(pos);

		i.dirty = false;
	}
}

void ShadowSystem::ClearShadowsQueue()
{
	for(auto& i: *components.data())
	{
		i.render_mgr->ClearAll();
	}
}

void ShadowSystem::RenderShadows()
{
	for(auto& i: *components.data())
	{
		EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());

		bitset<FRUSTUM_MAX_COUNT> bits;
		if(earlyVisComponent)
		{
			bits = earlyVisComponent->inFrust;	
			if(bits == 0)
				continue;
		}
		else
			bits = 0;
		
		if(bits == 0)
		{
			for(auto f: *frustums)
				((SceneRenderMgr*)f->rendermgr)->RenderShadow(i.get_id(), i.num, (ShadowRenderMgr*)i.render_mgr, i.vp_buf);
			continue;
		}

		for(auto f: *frustums)
		{
			if((bits & f->bit) == f->bit)
			{
				((SceneRenderMgr*)f->rendermgr)->RenderShadow(i.get_id(), i.num, (ShadowRenderMgr*)i.render_mgr, i.vp_buf);

				bits &= ~f->bit;
				if(bits == 0) break;
			}
		}
	}
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == ENTITY_COUNT)	return res;\
	auto comp = &components.getDataByArrayIdx(idx);

bool ShadowSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	bool res = true;
	do res = res && comp->dirty;
	while(comp = GetNextComponent(comp));
	return res;
}

bool ShadowSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	do comp->dirty = true;
	while(comp = GetNextComponent(comp));
	return true;
}

void ShadowSystem::UpdateShadowmapData(Entity e)
{
	GET_COMPONENT(void())

	XMMATRIX proj = lightSys->GetProj(comp->get_entity());
	BoundingFrustum frust;
	BoundingFrustum::CreateFromMatrix(frust, proj);
	do
	{
		comp->proj = XMMatrixTranspose(proj);
		comp->localFrustum = frust;
	} while(comp = GetNextComponent(comp));
}