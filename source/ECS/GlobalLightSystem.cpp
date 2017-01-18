#include "stdafx.h"
#include "GlobalLightSystem.h"
#include "World.h"

using namespace EngineCore;

GlobalLightSystem::GlobalLightSystem(BaseWorld* w, uint32_t maxCount)
{
	world = w;
	maxCount = min(maxCount, ENTITY_COUNT);

	components.create(maxCount);
	components.reserve(LIGHT_DIR_FRAME_MAX);

	cascadeNumForCamera.create(maxCount);
	cascadeNumForCamera.resize(maxCount);
	cascadeNumForCamera.assign(CAMERAS_MAX_COUNT);

	projPerCamera.resize(0);

	frustumMgr = w->GetFrustumMgr();

	transformSys = w->GetTransformSystem();
	cameraSystem = w->GetCameraSystem();

	camerasCount = 0;

	depth_cascade[0] = DEPTH_CASCADE_0;
	depth_cascade[1] = DEPTH_CASCADE_1;
	depth_cascade[2] = DEPTH_CASCADE_2;
	depth_cascade[3] = DEPTH_CASCADE_3;
	depth_cascade_half[0] = DEPTH_CASCADE_0_HALF;
	depth_cascade_half[1] = DEPTH_CASCADE_1_HALF;
	depth_cascade_half[2] = DEPTH_CASCADE_2_HALF;
	depth_cascade_half[3] = DEPTH_CASCADE_3_HALF;

	casc_dist[0] = CASCADE_DIST_0;
	casc_dist[1] = CASCADE_DIST_1;
	casc_dist[2] = CASCADE_DIST_2;
	casc_dist[3] = CASCADE_DIST_3;
}

void GlobalLightSystem::cleanCascades(CascadeShadow& cascade)
{
	for(uint8_t j=0; j<LIGHT_DIR_NUM_CASCADES; j++)
	{
		cascade.render_mgr[j] = nullptr;
		cascade.vp_buf[j] = nullptr;
		cascade.pos[j] = XMFLOAT3(0,0,0);
		cascade.view[j] = XMMatrixIdentity();
		cascade.view_proj[j] = XMMatrixIdentity();
	}
}

void GlobalLightSystem::initShadows(GlobalLightComponent& comp)
{
	comp.cascadePerCamera.resize(camerasCount);
	for(auto& i: *cameraSystem->components.data())
	{
		if(!i.active)
			continue;

		auto cascadeId = cascadeNumForCamera[i.get_id()];
		if(cascadeId >= camerasCount)
			continue;
		
		cleanCascades(comp.cascadePerCamera[cascadeId]);
		buildCascades(comp, cascadeId, &i);
	}
}
	
void GlobalLightSystem::AddCascadesForCamera(CameraComponent* camera)
{
	if(cascadeNumForCamera[camera->get_id()] >= CAMERAS_MAX_COUNT)
	{
		cascadeNumForCamera[camera->get_id()] = camerasCount;
		for(auto& i: *components.data())
		{
			i.cascadePerCamera.push_back();
			
			cleanCascades(i.cascadePerCamera[camerasCount]);
			buildCascades(i, camerasCount, camera);
		}

		projPerCamera.push_back();
		buildProj(camerasCount, camera);

		camerasCount++;
	}
	else
	{
		auto camId = cascadeNumForCamera[camera->get_id()];
		buildProj(camId, camera);
	}
}
		
void GlobalLightSystem::DeleteCascadesForCamera(CameraComponent* camera)
{
	auto camId = cascadeNumForCamera[camera->get_id()];
	if(camId >= CAMERAS_MAX_COUNT)
		return;
	for(auto& i: *components.data())
	{
		auto camEnt = (i.cascadePerCamera.end() - 1)->camera;
		cascadeNumForCamera[camEnt.index()] = camId;
		destroyCascades(i.cascadePerCamera[camId]);
		i.cascadePerCamera.erase_and_pop_back(camId);
	}
	cascadeNumForCamera[camera->get_id()] = CAMERAS_MAX_COUNT;
	projPerCamera.erase_and_pop_back(camId);
	camerasCount = camerasCount > 0 ? (camerasCount - 1) : 0;
}

void GlobalLightSystem::SwitchCascadesForCamera(CameraComponent* camOld, CameraComponent* camNew)
{
	auto camId = cascadeNumForCamera[camOld->get_id()];
	cascadeNumForCamera[camOld->get_id()] = CAMERAS_MAX_COUNT;
	cascadeNumForCamera[camNew->get_id()] = camId;
	for(auto& i: *components.data())
		buildCascades(i, camId, camNew);
	buildProj(camId, camNew);
}

void GlobalLightSystem::UpdateCascadesForCamera(CameraComponent* camera)
{
	auto camId = cascadeNumForCamera[camera->get_id()];
	for(auto& i: *components.data())
		matrixGenerate(i, i.cascadePerCamera[camId], projPerCamera[camId]);
}

#define UP_VECT XMVectorSet(1,0,0,0)
#define DIR_VECT XMVectorSet(0,-1,0,0)

void GlobalLightSystem::buildCascades(GlobalLightComponent& comp, uint16_t camId, CameraComponent* cam)
{
	auto& shadowCascade = comp.cascadePerCamera[camId];
	shadowCascade.camera = cam->get_entity();
	
	for(uint8_t i=0; i<LIGHT_DIR_NUM_CASCADES; i++)
	{
		if(!shadowCascade.render_mgr[i])
			shadowCascade.render_mgr[i] = new ShadowRenderMgr();
		if(!shadowCascade.vp_buf[i])
			shadowCascade.vp_buf[i] = Buffer::CreateConstantBuffer(Render::Device(), sizeof(XMMATRIX), true);
	}
}

void GlobalLightSystem::buildProj(uint16_t camId, CameraComponent* cam)
{
	auto& projCascade = projPerCamera[camId];
	projCascade.camera = cam->get_entity();

	float cascade_start = cam->near_clip;

	for(uint8_t i=0; i<LIGHT_DIR_NUM_CASCADES; i++)
	{
		float cascade_end = cascade_start + casc_dist[i];
		XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(cam->fov, cam->aspect_ratio, cascade_start, cascade_end);

		BoundingFrustum cam_frust_first;
		BoundingFrustum::CreateFromMatrix(cam_frust_first, projMatrix);

		XMFLOAT3 frust_corners[8];
		cam_frust_first.GetCorners(frust_corners);
		XMVECTOR diag = XMLoadFloat3(&frust_corners[0]) - XMLoadFloat3(&frust_corners[6]);
		
		projCascade.posOffset[i] = (cascade_start + cascade_end) * 0.5f;
		float size = projCascade.size[i] = XMVectorGetX(XMVector3Length(diag)) * 1.02f;

		projCascade.proj[i] = XMMatrixOrthographicLH(size, size, 0.0f, depth_cascade[i]);

		cascade_start += casc_dist[i];
	}
}

void GlobalLightSystem::destroyCascades(CascadeShadow& cascade)
{
	for(uint8_t i=0; i<LIGHT_DIR_NUM_CASCADES; i++)
	{
		_DELETE(cascade.render_mgr[i]);
		_RELEASE(cascade.vp_buf[i]);
	}
}

void GlobalLightSystem::matrixGenerate(GlobalLightComponent& comp, CascadeShadow& cascade, CascadeProj& projCascade)
{
	auto cam = cameraSystem->GetComponent(cascade.camera);
	uint16_t shadowRes = cam->render_mgr->shadowsRenderer->GetShadowCascadeRes();

	XMVECTOR lightDir = XMLoadFloat3(&comp.dir);
	XMVECTOR Up = XMLoadFloat3(&comp.dir_up);
	XMVECTOR Side = XMVector3Normalize(XMVector3Cross(Up, lightDir));
	
	for(uint8_t i = 0; i<LIGHT_DIR_NUM_CASCADES; i++)
	{
		float pix = projCascade.size[i] / float(shadowRes);

		XMVECTOR lightLookAt = cam->camPos + cam->camLookDir * projCascade.posOffset[i];

		XMVECTOR upProj = pix * XMVectorFloor(XMVector3Dot(lightLookAt, Up) / pix);
		XMVECTOR sideProj = pix * XMVectorFloor(XMVector3Dot(lightLookAt, Side) / pix);
		XMVECTOR dirProj = XMVector3Dot(lightLookAt, lightDir);

		lightLookAt = Up * upProj + Side * sideProj + lightDir * dirProj;
		XMVectorSetW(lightLookAt, 1.0f);

		float depth_offset = projCascade.size[i];
		depth_offset = sqrt(2.0f * depth_offset * depth_offset);

		XMVECTOR lightPos = lightLookAt - lightDir * (depth_cascade[i] - depth_offset);
				
		cascade.view[i] = XMMatrixLookAtLH(lightPos, lightLookAt, Up);
		cascade.view_proj[i] = XMMatrixTranspose(cascade.view[i] * projCascade.proj[i]);
		XMStoreFloat3(&cascade.pos[i], lightPos);
		
		Render::UpdateDynamicResource(cascade.vp_buf[i], (void*)&cascade.view_proj[i], sizeof(XMMATRIX));

		// Frustum
		BoundingOrientedBox& viewbox = cascade.worldFrustum[i];

		XMVECTOR viewboxPos = lightLookAt - lightDir * (depth_cascade_half[i] - depth_offset);
		XMStoreFloat3(&viewbox.Center, viewboxPos);

		viewbox.Extents.x = projCascade.size[i] * 0.5f;
		viewbox.Extents.y = viewbox.Extents.x;
		viewbox.Extents.z = depth_cascade_half[i];
		
		XMVECTOR scale, rot, translate;
		XMMatrixDecompose(&scale, &rot, &translate, XMMatrixTranspose(cascade.view[i]));
		XMStoreFloat4(&viewbox.Orientation, rot);
	}
}

void GlobalLightSystem::RegShadowMaps()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.active)
			continue;
		
		for(auto f: frustumMgr->camDataArray)
		{
			CascadeShadow* shadowMap = &i.cascadePerCamera[cascadeNumForCamera[f->get_id()]];
			for(uint8_t j=0; j < LIGHT_DIR_NUM_CASCADES; j++)
			{
				((SceneRenderMgr*)f->rendermgr)->shadowsRenderer->RegShadowMap(i.get_id(), DIR_LIGHT_SCREENSIZE);
				frustumMgr->AddFrustum(i.get_entity(), &shadowMap->worldFrustum[j], (BaseRenderMgr*)shadowMap->render_mgr[j], nullptr, nullptr, true);
			}
		}					
	}
}

void GlobalLightSystem::RegToScene()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.active)
			continue;

		for(auto f: frustumMgr->camDataArray)
		{
			auto& cascade = i.cascadePerCamera[cascadeNumForCamera[f->get_id()]];
			((SceneRenderMgr*)f->rendermgr)->RegDirLight(i.hdr_color, i.area_data, i.dir, cascade.view_proj, cascade.pos, i.get_id());
		}
	}
}

void GlobalLightSystem::Update()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.active || !i.dirty)
			continue;

		updateLightComp(i);

		for(auto& j: i.cascadePerCamera)
			matrixGenerate(i, j, projPerCamera[cascadeNumForCamera[j.camera.index()]]);

		i.dirty = false;
	}
}

void GlobalLightSystem::updateLightComp(GlobalLightComponent& comp)
{
	comp.hdr_color.w = comp.brightness;
	comp.hdr_color.x = comp.color.x * comp.brightness;
	comp.hdr_color.y = comp.color.y * comp.brightness;
	comp.hdr_color.z = comp.color.z * comp.brightness;

	comp.area_data.x = sin(comp.area);
	comp.area_data.y = cos(comp.area);
	
	XMMATRIX worldMatrix = transformSys->GetTransformW(comp.get_entity());

	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, worldMatrix);
	XMMATRIX transform = XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

	XMVECTOR lightDir = XMVector3TransformNormal(DIR_VECT, transform);
	XMVECTOR upDir = XMVector3TransformNormal(UP_VECT, transform);
	XMStoreFloat3(&comp.dir, lightDir);
	XMStoreFloat3(&comp.dir_up, upDir);
}

void GlobalLightSystem::RenderShadows()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.active)
			continue;

		for(auto f: frustumMgr->camDataArray)
		{
			auto& cascade = i.cascadePerCamera[cascadeNumForCamera[f->get_id()]];
			for(uint8_t j=0; j<LIGHT_DIR_NUM_CASCADES; j++)
				((SceneRenderMgr*)f->rendermgr)->shadowsRenderer->RenderShadow(i.get_id(), j, cascade.render_mgr[j], cascade.vp_buf[j]);
		}
	}
}

void GlobalLightSystem::ClearShadowsQueue()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.active)
			continue;
		
		for(auto f: frustumMgr->camDataArray)
		{
			CascadeShadow* shadowMap = &i.cascadePerCamera[cascadeNumForCamera[f->get_id()]];
			for(uint8_t j=0; j < LIGHT_DIR_NUM_CASCADES; j++)
				shadowMap->render_mgr[j]->ClearAll();
		}
	}
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool GlobalLightSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool GlobalLightSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

uint32_t GlobalLightSystem::Serialize(Entity e, uint8_t* data)
{
	GET_COMPONENT(0)

	auto t_data = data;
	uint32_t size = 0;

	*(bool*)t_data = comp.active;
	t_data += sizeof(bool);
	size += sizeof(bool);
	*(XMFLOAT3*)t_data = comp.color;
	t_data += sizeof(XMFLOAT3);
	size += sizeof(XMFLOAT3);
	*(float*)t_data = comp.brightness;
	t_data += sizeof(float);
	size += sizeof(float);
	*(float*)t_data = comp.area;
	t_data += sizeof(float);
	size += sizeof(float);	

	return size;
}

uint32_t GlobalLightSystem::Deserialize(Entity e, uint8_t* data)
{
	auto t_data = data;
	uint32_t size = 0;

	GlobalLightComponent comp;
	comp.active = *(bool*)t_data;
	t_data += sizeof(bool);
	size += sizeof(bool);
	comp.color = *(XMFLOAT3*)t_data;
	t_data += sizeof(XMFLOAT3);
	size += sizeof(XMFLOAT3);
	comp.brightness = *(float*)t_data;
	t_data += sizeof(float);
	size += sizeof(float);
	comp.area = *(float*)t_data;
	t_data += sizeof(float);
	size += sizeof(float);

	AddComponent(e, comp);
	return size;
}

bool GlobalLightSystem::IsActive(Entity e)
{
	GET_COMPONENT(false)
	return comp.active;
}

bool GlobalLightSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(false)
	comp.active = active;
	return true;
}

bool GlobalLightSystem::SetColor(Entity e, XMFLOAT3 color)
{
	GET_COMPONENT(false)
	comp.color = color;
	comp.dirty = true;
	return true;
}
XMFLOAT3 GlobalLightSystem::GetColor(Entity e)
{
	GET_COMPONENT(XMFLOAT3())
	return comp.color;
}

bool GlobalLightSystem::SetBrightness(Entity e, float brightness)
{
	GET_COMPONENT(false)
	comp.brightness = brightness;
	comp.dirty = true;
	return true;
}

float GlobalLightSystem::GetBrightness(Entity e)
{
	GET_COMPONENT(0.0f)
	return comp.brightness;
}

bool GlobalLightSystem::SetArea(Entity e, float area)
{
	GET_COMPONENT(false)
	comp.area = area;
	comp.dirty = true;
	return true;
}

float GlobalLightSystem::GetArea(Entity e)
{
	GET_COMPONENT(0.0f)
	return comp.area;
}	