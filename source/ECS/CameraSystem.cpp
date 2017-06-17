#include "stdafx.h"
#include "CameraSystem.h"
#include "GlobalLightSystem.h"
#include "TypeMgr.h"
#include "World.h"

using namespace EngineCore;

CameraSystem::CameraSystem(BaseWorld* w, uint32_t maxCount)
{
	world = w;
	maxCount = min(maxCount, ENTITY_COUNT);
	components.create(maxCount);
	components.reserve(CAMERAS_INIT_COUNT);

	transformSys = w->GetTransformSystem();
	frustum_mgr = w->GetFrustumMgr();

	globalLightSystem = nullptr;
}

void CameraSystem::SetGlobalLightSys(GlobalLightSystem* gls)
{
	globalLightSystem = gls;
}

CameraSystem::~CameraSystem()
{
	for(auto& it: *components.data())
	{
		_DELETE(it.render_mgr);
		if(globalLightSystem)
			globalLightSystem->DeleteCascadesForCamera(&it);
	}
}

void CameraSystem::DeleteComponent(Entity e)
{
	auto comp = GetComponent(e);
	if(!comp) return;
	_DELETE(comp->render_mgr);
	if(comp->active)
	{
		WRN("Camera component still active for deleted entity!");
		if(globalLightSystem)
			globalLightSystem->DeleteCascadesForCamera(comp);
	}
	components.remove(e.index());
}

void CameraSystem::initCamera(CameraComponent* comp)
{
	comp->dirty = true;
	comp->projMatrix = XMMatrixPerspectiveFovLH(comp->fov, comp->aspect_ratio, comp->near_clip, comp->far_clip);
	BoundingFrustum::CreateFromMatrix(comp->localFrustum, comp->projMatrix);
	comp->render_mgr = nullptr;
	comp->volume_id = FRUSTUM_MAX_COUNT;
}

void CameraSystem::RegToDraw()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		regCamera(i);
	}
}

void CameraSystem::regCamera(CameraComponent& comp)
{
	if(!comp.active)
		return;

	comp.prevViewProj = comp.viewMatrix * comp.projMatrix; // store prev frame
	
	if(comp.dirty)
	{
		XMMATRIX worldMatrix = transformSys->GetTransformW(comp.get_entity());
		comp.camPos = XMVector3TransformCoord(XMVectorZero(), worldMatrix);
		comp.camLookDir = XMVector3TransformNormal(XMVectorSet(0,0,1,0), worldMatrix);
		comp.camUp = XMVector3TransformNormal(XMVectorSet(0,1,0,0), worldMatrix);

		comp.viewMatrix = XMMatrixLookAtLH( comp.camPos, comp.camPos + comp.camLookDir, comp.camUp );
			
		////
		XMMATRIX cam_world = TransformationFromViewPos(comp.viewMatrix, comp.camPos);

		comp.localFrustum.Transform(comp.worldFrustum, cam_world);
		////comp.localFrustum.Transform(comp.worldFrustum, tranform->worldMatrix);

		if(globalLightSystem)
			globalLightSystem->UpdateCascadesForCamera(&comp);

		comp.view_proj = XMMatrixMultiply(comp.viewMatrix, comp.projMatrix); // remove

		if(comp.render_mgr->voxelRenderer) // todo
			comp.render_mgr->voxelRenderer->CalcVolumeBox(comp.camPos, comp.camLookDir);

		comp.dirty = false;
	}

	auto ent = comp.get_entity();

	auto f = frustum_mgr->AddFrustum(ent, &comp.worldFrustum, comp.render_mgr, &comp.viewMatrix, &comp.projMatrix);
	comp.frust_id = int32_t(f->get_frustum_id());

	if(comp.render_mgr->voxelRenderer)
	{
		auto v = frustum_mgr->AddFrustum(ent, &comp.render_mgr->voxelRenderer->GetBigVolumeBox(), comp.render_mgr, nullptr, nullptr, true, true);
		comp.volume_id = int32_t(v->get_frustum_id());
	}

	comp.render_mgr->UpdateCamera(&comp);
	comp.render_mgr->ZeroMeshgroups();
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

void CameraSystem::RegSingle(Entity e)
{
	GET_COMPONENT(void())
	regCamera(comp);
}

bool CameraSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

uint32_t CameraSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)

	auto t_data = data;
	uint32_t size = 0;

	*(float*)t_data = comp.far_clip;
	t_data += sizeof(float);
	size += sizeof(float);
	*(float*)t_data = comp.near_clip;
	t_data += sizeof(float);
	size += sizeof(float);
	*(float*)t_data = comp.fov;
	t_data += sizeof(float);
	size += sizeof(float);	

	return size;
}

uint32_t CameraSystem::Deserialize(Entity e, uint8_t* data)
{
	auto t_data = data;
	uint32_t size = 0;

	CameraComponent comp;
	comp.far_clip = *(float*)t_data;
	t_data += sizeof(float);
	size += sizeof(float);
	comp.near_clip = *(float*)t_data;
	t_data += sizeof(float);
	size += sizeof(float);
	comp.fov = *(float*)t_data;
	t_data += sizeof(float);
	size += sizeof(float);

	AddComponent(e, comp);
	return size;
}

bool CameraSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

bool CameraSystem::IsActive(Entity e)
{
	GET_COMPONENT(false)
	return comp.active;
}

bool CameraSystem::Activate(Entity e, ScenePipeline* scene)
{
	if(!scene)
		return false;
	GET_COMPONENT(false)
	comp.active = true;

	auto oldCam = scene->GetSceneCamera();
	CameraComponent* oldCamComp = nullptr;
	if(oldCam.sys && (oldCamComp = GetComponent(oldCam.e)))
	{
		if(oldCamComp != &comp)
		{
			oldCamComp->active = false;
			if(globalLightSystem)
				globalLightSystem->SwitchCascadesForCamera(oldCamComp, &comp);
			oldCamComp->render_mgr = nullptr;
		}
	}
	else
	{
		if(globalLightSystem)
			globalLightSystem->AddCascadesForCamera(&comp);
	}
	comp.render_mgr = scene->GetRenderMgr();
	scene->SetCamera(CameraLink(this, e));
	return true;
}

bool CameraSystem::Deactivate(Entity e, ScenePipeline* scene)
{
	GET_COMPONENT(false)

	auto sceneCam = scene->GetSceneCamera();
	if(!EntIsEq(e, sceneCam.e))
		return false;

	comp.active = false;
	if(globalLightSystem)
		globalLightSystem->DeleteCascadesForCamera(&comp);
	comp.render_mgr = nullptr;

	scene->SetCamera(CameraLink());

	return true;
}

int32_t CameraSystem::GetFrustumId(Entity e)
{
	GET_COMPONENT(-1)
	return comp.frust_id;
}

int32_t CameraSystem::GetVolumeId(Entity e)
{
	GET_COMPONENT(-1)
	return comp.volume_id;
}

XMVECTOR CameraSystem::GetVectorFromScreen(Entity e, XMVECTOR screen_point, float screen_w, float screen_h)
{
	GET_COMPONENT(XMVectorZero())
		return XMVector3Normalize(XMVector3Unproject(screen_point, 0.0f, 0.0f, screen_w, screen_h, 0.0f, 1.0f, comp.projMatrix, comp.viewMatrix, XMMatrixTranslationFromVector(comp.camPos))); 
}

XMVECTOR CameraSystem::GetLookDirV(Entity e)
{
	GET_COMPONENT(XMVectorZero())
	return comp.camLookDir;
}

XMVECTOR CameraSystem::GetLookTangentV(Entity e)
{
	GET_COMPONENT(XMVectorZero())
	return XMVector3Normalize(XMVector3Cross(comp.camLookDir, comp.camUp));
}

XMVECTOR CameraSystem::GetUpV(Entity e)
{
	GET_COMPONENT(XMVectorZero())
	return comp.camUp;
}

XMVECTOR CameraSystem::GetPosV(Entity e)
{
	GET_COMPONENT(XMVectorZero())
	return comp.camPos;
}

bool CameraSystem::SetProps(Entity e, CameraComponent D)
{
	GET_COMPONENT(false)
	comp.aspect_ratio = D.aspect_ratio;
	comp.far_clip = D.far_clip;
	comp.near_clip = D.near_clip;
	comp.fov = D.fov;
	comp.projMatrix = XMMatrixPerspectiveFovLH(comp.fov, comp.aspect_ratio, comp.near_clip, comp.far_clip);
	BoundingFrustum::CreateFromMatrix(comp.localFrustum, comp.projMatrix);
	comp.dirty = true;
	if(comp.active && globalLightSystem)
		globalLightSystem->AddCascadesForCamera(&comp);
	return true;
}

bool CameraSystem::SetFov(Entity e, float fov)
{
	GET_COMPONENT(false)
	comp.fov = fov;
	comp.projMatrix = XMMatrixPerspectiveFovLH(comp.fov, comp.aspect_ratio, comp.near_clip, comp.far_clip);
	BoundingFrustum::CreateFromMatrix(comp.localFrustum, comp.projMatrix);
	comp.dirty = true;
	if(comp.active && globalLightSystem)
		globalLightSystem->AddCascadesForCamera(&comp);
	return true;
}

bool CameraSystem::SetAspect(Entity e, float aspect)
{
	GET_COMPONENT(false)
	comp.aspect_ratio = aspect;
	comp.projMatrix = XMMatrixPerspectiveFovLH(comp.fov, comp.aspect_ratio, comp.near_clip, comp.far_clip);
	BoundingFrustum::CreateFromMatrix(comp.localFrustum, comp.projMatrix);
	comp.dirty = true;
	if(comp.active && globalLightSystem)
		globalLightSystem->AddCascadesForCamera(&comp);
	return true;
}

bool CameraSystem::SetFar(Entity e, float farplane)
{
	GET_COMPONENT(false)
	comp.far_clip = farplane;
	comp.projMatrix = XMMatrixPerspectiveFovLH(comp.fov, comp.aspect_ratio, comp.near_clip, comp.far_clip);
	BoundingFrustum::CreateFromMatrix(comp.localFrustum, comp.projMatrix);
	comp.dirty = true;
	if(comp.active && globalLightSystem)
		globalLightSystem->AddCascadesForCamera(&comp);
	return true;
}

bool CameraSystem::SetNear(Entity e, float nearplane)
{
	GET_COMPONENT(false)
	comp.near_clip = nearplane;
	comp.projMatrix = XMMatrixPerspectiveFovLH(comp.fov, comp.aspect_ratio, comp.near_clip, comp.far_clip);
	BoundingFrustum::CreateFromMatrix(comp.localFrustum, comp.projMatrix);
	comp.dirty = true;
	if(comp.active && globalLightSystem)
		globalLightSystem->AddCascadesForCamera(&comp);
	return true;
}

float CameraSystem::GetFov(Entity e)
{
	GET_COMPONENT(0)
	return comp.fov;
}

float CameraSystem::GetAspect(Entity e)
{
	GET_COMPONENT(0)
	return comp.aspect_ratio;
}

float CameraSystem::GetFar(Entity e)
{
	GET_COMPONENT(0)
	return comp.far_clip;
}

float CameraSystem::GetNear(Entity e)
{
	GET_COMPONENT(0)
	return comp.near_clip;
}
