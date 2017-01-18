#include "stdafx.h"
#include "VisibilitySystem.h"
#include "World.h"

using namespace EngineCore;

VisibilitySystem::VisibilitySystem(BaseWorld* w, uint32_t maxCount)
{
	world = w;
	frustumMgr = w->GetFrustumMgr();
	transformSys = w->GetTransformSystem();

	maxCount = min(maxCount, ENTITY_COUNT);
	components.create(maxCount);
}

void VisibilitySystem::CheckVisibility()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(i.dirty)
		{
			XMMATRIX worldMatrix = transformSys->GetTransformW(i.get_entity());
			
			BoundingOrientedBox::CreateFromBoundingBox(i.worldBox, i.localBox);
			//i.worldBox.Transform(i.worldBox, worldMatrix); // broken DX func
			BoundingOrientedBoxTransformFixed(i.worldBox, worldMatrix);

			i.dirty = false;
		}

		i.inFrust = 0;
		for(auto& f: *frustumMgr->m_frustums.data())
			if(f.frustum.Contains(i.worldBox) != DISJOINT)
				i.inFrust |= f.bit;
	}
}

Entity VisibilitySystem::CollideRay(XMFLOAT3 origin, XMFLOAT3 ray, int frust_id)
{
	Entity ent;
	collide_ray(origin, ray, frust_id, nullptr, &ent);
	return ent;
}

XMFLOAT3 VisibilitySystem::CollideRayCoords(XMFLOAT3 origin, XMFLOAT3 ray, int frust_id)
{
	XMFLOAT3 colide_coord;
	Entity ent;
	collide_ray(origin, ray, frust_id, &colide_coord, &ent);
	return colide_coord;
}

void VisibilitySystem::collide_ray(XMFLOAT3 origin, XMFLOAT3 ray, int frust_id, XMFLOAT3* colide_coord, Entity* ent)
{
	XMVECTOR vorigin = XMLoadFloat3(&origin);
	XMVECTOR vray = XMLoadFloat3(&ray);

	Frustum& f = frustumMgr->GetFrustum(frust_id);

	float min_dist = SELECT_3D_MAX_DIST;
	ent->setnull();

	for(auto& i: *components.data())
	{
		if((i.inFrust & f.bit) == f.bit)
		{
			float dist = RayOrientedBoxIntersect(vorigin, vray, i.worldBox);
			if(dist > 0 && dist < min_dist)
			{
				min_dist = dist;
				*ent = i.get_entity();
			}
		}
	}

	if(colide_coord)
		XMStoreFloat3(colide_coord, vorigin + XMVector3Normalize(vray) * min_dist);
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool VisibilitySystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool VisibilitySystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

bool VisibilitySystem::SetBBox(Entity e, BoundingBox box)
{
	GET_COMPONENT(false)
	comp.localBox = box;
	comp.dirty = true;
	return true;
}

bool VisibilitySystem::AddToBBox(Entity e, BoundingBox box)
{
	GET_COMPONENT(false)
	BoundingBox::CreateMerged(comp.localBox, comp.localBox, box);
	comp.dirty = true;
	return true;
}

bool VisibilitySystem::AddToBBox(Entity e, BoundingSphere sphere)
{
	GET_COMPONENT(false)
	BoundingBox temp;
	BoundingBox::CreateFromSphere(temp, sphere);
	BoundingBox::CreateMerged(comp.localBox, comp.localBox, temp);
	comp.dirty = true;
	return true;
}

BoundingBox VisibilitySystem::GetBBoxL(Entity e)
{
	GET_COMPONENT(BoundingBox())
	return comp.localBox;
}

BoundingOrientedBox VisibilitySystem::GetBBoxW(Entity e)
{
	GET_COMPONENT(BoundingOrientedBox())
	return comp.worldBox;
}