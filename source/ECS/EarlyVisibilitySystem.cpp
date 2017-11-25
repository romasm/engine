#include "stdafx.h"
#include "EarlyVisibilitySystem.h"
#include "World.h"

using namespace EngineCore;

EarlyVisibilitySystem::EarlyVisibilitySystem(BaseWorld* w, uint32_t maxCount)
{
	world = w;
	frustumMgr = w->GetFrustumMgr();
	transformSys = w->GetTransformSystem();
	
	maxCount = std::min<uint32_t>(maxCount, ENTITY_COUNT);

	components.create(maxCount);
}

void EarlyVisibilitySystem::CheckEarlyVisibility()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;
				
		if(i.dirty)
		{
			XMMATRIX worldMatrix = transformSys->GetTransform_WInternal(i.get_entity());

			switch (i.type)
			{
			case BT_BOX:
				{
					BoundingOrientedBox::CreateFromBoundingBox(i.worldBox, i.localBox);
					//i.worldBox.Transform(i.worldBox, tranform->worldMatrix); // broken DX func
					BoundingOrientedBoxTransformFixed(i.worldBox, worldMatrix);
				}
				break;
			case BT_SPHERE:
				i.localSphere.Transform(i.worldSphere, worldMatrix);
				break;
			case BT_FRUSTUM:
				i.localFrustum.Transform(i.worldFrustum, worldMatrix);
				break;
			case BT_FRUSTUM_SPHERE:
				i.localSphere.Transform(i.worldSphere, worldMatrix);
				i.localFrustum.Transform(i.worldFrustum, worldMatrix);
				break;
			}

			i.dirty = false;
		}

		i.inFrust = 0;
		for(auto& f: *(frustumMgr->m_frustums.data()))
			switch (i.type)
			{
			case BT_BOX:
				if(f.frustum.Contains(i.worldBox) != DISJOINT)
					i.inFrust |= f.bit;
				break;
			case BT_SPHERE:
				if(f.frustum.Contains(i.worldSphere) != DISJOINT)
					i.inFrust |= f.bit;
				break;
			case BT_FRUSTUM:
				if(f.frustum.Contains(i.worldFrustum) != DISJOINT)
					i.inFrust |= f.bit;
				break;
			case BT_FRUSTUM_SPHERE:
				if(f.frustum.Contains(i.worldSphere) != DISJOINT && f.frustum.Contains(i.worldFrustum) != DISJOINT)
					i.inFrust |= f.bit;
				break;
			}
			
	}
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool EarlyVisibilitySystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool EarlyVisibilitySystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

void EarlyVisibilitySystem::SetType(Entity e, BoundingType type)
{
	GET_COMPONENT(void())
	comp.type = type;
}

bool EarlyVisibilitySystem::SetBBox(Entity e, BoundingBox box)
{
	GET_COMPONENT(false)
	comp.localBox = box;
	comp.dirty = true;
	return true;
}

bool EarlyVisibilitySystem::AddToBBox(Entity e, BoundingBox box)
{
	GET_COMPONENT(false)
	BoundingBox::CreateMerged(comp.localBox, comp.localBox, box);
	comp.dirty = true;
	return true;
}

bool EarlyVisibilitySystem::AddToBBox(Entity e, BoundingSphere sphere)
{
	GET_COMPONENT(false)
	BoundingBox temp;
	BoundingBox::CreateFromSphere(temp, sphere);
	BoundingBox::CreateMerged(comp.localBox, comp.localBox, temp);
	comp.dirty = true;
	return true;
}

BoundingBox EarlyVisibilitySystem::GetBBoxL(Entity e)
{
	GET_COMPONENT(BoundingBox())
	return comp.localBox;
}

BoundingOrientedBox EarlyVisibilitySystem::GetBBoxW(Entity e)
{
	GET_COMPONENT(BoundingOrientedBox())
	return comp.worldBox;
}

bool EarlyVisibilitySystem::SetBSphere(Entity e, BoundingSphere shpere)
{
	GET_COMPONENT(false)
	comp.localSphere = shpere;
	comp.dirty = true;
	return true;
}

bool EarlyVisibilitySystem::AddToBSphere(Entity e, BoundingSphere sphere)
{
	GET_COMPONENT(false)
	BoundingSphere::CreateMerged(comp.localSphere, comp.localSphere, sphere);
	comp.dirty = true;
	return true;
}

bool EarlyVisibilitySystem::AddToBSphere(Entity e, BoundingBox box)
{
	GET_COMPONENT(false)
	BoundingSphere temp;
	BoundingSphere::CreateFromBoundingBox(temp, box);
	BoundingSphere::CreateMerged(comp.localSphere, comp.localSphere, temp);
	comp.dirty = true;
	return true;
}

BoundingSphere EarlyVisibilitySystem::GetBSphereL(Entity e)
{
	GET_COMPONENT(BoundingSphere())
	return comp.localSphere;
}

BoundingSphere EarlyVisibilitySystem::GetBSphereW(Entity e)
{
	GET_COMPONENT(BoundingSphere())
	return comp.worldSphere;
}

bool EarlyVisibilitySystem::SetBFrustum(Entity e, BoundingFrustum frust)
{
	GET_COMPONENT(false)
	comp.localFrustum = frust;
	comp.dirty = true;
	return true;
}

BoundingFrustum EarlyVisibilitySystem::GetBFrustumL(Entity e)
{
	GET_COMPONENT(BoundingFrustum())
	return comp.localFrustum;
}

BoundingFrustum EarlyVisibilitySystem::GetBFrustumW(Entity e)
{
	GET_COMPONENT(BoundingFrustum())
	return comp.worldFrustum;
}