#include "stdafx.h"
#include "ShadowSystem.h"
#include "LightSystem.h"
#include "../World.h"

using namespace EngineCore;

ShadowSystem::ShadowSystem(BaseWorld* w, uint32_t maxCount)
{
	shadow_casters = 0;

	earlyVisibilitySys = w->GetEarlyVisibilitySystem();
	frustum_mgr = w->GetFrustumMgr();
	
	maxCount = min(maxCount, ENTITY_COUNT);
	components.create(maxCount, maxCount);
}

void ShadowSystem::SetLightSys(LightSystem* ls)
{
	lightSys = ls;
}

void ShadowSystem::initShadowmap(ShadowComponent* comp)
{
	comp->dirty = true;
	comp->render_mgr = new ShadowRenderMgr();
	comp->vp_buf = Buffer::CreateConstantBuffer(Render::Device(), sizeof(XMMATRIX), true);
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

		Render::UpdateDynamicResource(i.vp_buf, (void*)&i.view_proj, sizeof(XMMATRIX));

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
			for(auto f: frustum_mgr->camDataArray)
				((SceneRenderMgr*)f->rendermgr)->shadowsRenderer->RenderShadow(i.get_id(), i.num, (ShadowRenderMgr*)i.render_mgr, i.vp_buf);
			continue;
		}

		for(auto f: frustum_mgr->camDataArray)
		{
			if((bits & f->bit) == f->bit)
			{
				((SceneRenderMgr*)f->rendermgr)->shadowsRenderer->RenderShadow(i.get_id(), i.num, (ShadowRenderMgr*)i.render_mgr, i.vp_buf);

				bits &= ~f->bit;
				if(bits == 0) break;
			}
		}
	}
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
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