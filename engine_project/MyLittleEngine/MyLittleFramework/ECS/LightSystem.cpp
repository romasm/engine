#include "stdafx.h"
#include "LightSystem.h"
#include "../World.h"
#include "../WorldMgr.h"

using namespace EngineCore;

LightSystem::LightSystem(World* wld)
{
	world = wld;

	frustumMgr = world->GetFrustumMgr();
	frustums = &frustumMgr->camDataArray;

	transformSys = world->GetTransformSystem();
	earlyVisibilitySys = world->GetEarlyVisibilitySystem();
	shadowSystem = world->GetShadowSystem();
}

#define UP_VECT XMVectorSet(1,0,0,0)
#define DIR_VECT XMVectorSet(0,-1,0,0)

#define ITERATE_FRUSTUMS(code) for(auto f: *frustums){ \
	if((bits & f->bit) == f->bit){ \
	code \
	bits &= ~f->bit; if(bits == 0) break;}}

void LightSystem::RegShadowMaps()
{
	for(auto& i: *components.data())
	{
		if(!i.active || !i.cast_shadows)
			continue;
		
		switch (i.type)
		{
		case LIGHT_TYPE_SPOT:
		case LIGHT_TYPE_DISK:
		case LIGHT_TYPE_RECT:
			{
				EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());
				auto bits = earlyVisComponent->inFrust;
				if(bits == 0)
					break;

				ShadowComponent* shadowComp = shadowSystem->GetComponent(i.get_entity());

				ITERATE_FRUSTUMS(
					((SceneRenderMgr*)f->rendermgr)->RegShadowMap(i.get_id(), FrustumMgr::CalcScreenSize(f->frustum, shadowComp->worldFrustum));
					frustumMgr->AddFrustum(i.get_entity(), &shadowComp->worldFrustum, (BaseRenderMgr*)shadowComp->render_mgr, nullptr, nullptr, true);
				)
			}
			break;

		case LIGHT_TYPE_POINT:
		case LIGHT_TYPE_SPHERE:
		case LIGHT_TYPE_TUBE:
			{
				EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());
				auto bits = earlyVisComponent->inFrust;
				if(bits == 0)
					break;

				ShadowComponent* shadowComp = shadowSystem->GetComponent(i.get_entity());

				ITERATE_FRUSTUMS(
					do
					{
						((SceneRenderMgr*)f->rendermgr)->RegShadowMap(i.get_id(), FrustumMgr::CalcScreenSize(f->frustum, shadowComp->worldFrustum));
						frustumMgr->AddFrustum(i.get_entity(), &shadowComp->worldFrustum, (BaseRenderMgr*)shadowComp->render_mgr, nullptr, nullptr, true);
					} while(shadowComp = shadowSystem->GetNextComponent(shadowComp));
				)
			}
			break;
		}
	}
}

void LightSystem::RegToScene()
{
	for(auto& i: *components.data())
	{
		if(!i.active)
			continue;

		switch (i.type)
		{
		case LIGHT_TYPE_SPOT:
			{
				EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());
				auto bits = earlyVisComponent->inFrust;
				if(bits == 0)
					break;

				if(i.cast_shadows)
				{
					ShadowComponent* shadowComp = shadowSystem->GetComponent(i.get_entity());
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegSpotCaster(i.hdr_color, i.rangeInvSqr, i.cone_data, i.pos, i.dir, 
							shadowComp->view_proj, shadowComp->proj, i.get_id());
					)
				}
				else
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegSpotLight(i.hdr_color, i.rangeInvSqr, i.cone_data, i.pos, i.dir);
					)
			}
			break;

		case LIGHT_TYPE_DISK:
			{
				EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());
				auto bits = earlyVisComponent->inFrust;
				if(bits == 0)
					break;

				if(i.cast_shadows)
				{
					ShadowComponent* shadowComp = shadowSystem->GetComponent(i.get_entity());
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegSpotCasterDisk(i.hdr_color, i.rangeInvSqr, i.area_data, i.cone_data, i.pos, i.dir, i.virt_pos, 
							SHADOW_NEARCLIP + i.virt_clip, shadowComp->view_proj, shadowComp->proj, i.get_id());
					)
				}
				else
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegSpotLightDisk(i.hdr_color, i.rangeInvSqr, i.area_data, i.cone_data, i.pos, i.dir, i.virt_pos);
					)
			}
			break;

		case LIGHT_TYPE_RECT:
			{
				EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());
				auto bits = earlyVisComponent->inFrust;
				if(bits == 0)
					break;

				if(i.cast_shadows)
				{
					ShadowComponent* shadowComp = shadowSystem->GetComponent(i.get_entity());
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegSpotCasterRect(i.hdr_color, i.rangeInvSqr, i.area_data, i.cone_data, i.pos, i.dir, i.dir_up, i.dir_side, i.virt_pos, 
							SHADOW_NEARCLIP + i.virt_clip, shadowComp->view_proj, shadowComp->proj, i.get_id());
					)
				}
				else
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegSpotLightRect(i.hdr_color, i.rangeInvSqr, i.area_data, i.cone_data, i.pos, i.dir, i.dir_up, i.dir_side, i.virt_pos);
					)
			}
			break;

		case LIGHT_TYPE_POINT:
			{
				EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());
				auto bits = earlyVisComponent->inFrust;	
				if(bits == 0)
					break;

				if(i.cast_shadows)
				{
					ShadowComponent* shadowComp = shadowSystem->GetComponent(i.get_entity());
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegPointCaster(i.hdr_color, i.rangeInvSqr, i.pos, shadowComp->proj, i.get_id());
					)
				}
				else
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegPointLight(i.hdr_color, i.rangeInvSqr, i.pos);
					)
			}
			break;

		case LIGHT_TYPE_SPHERE:
			{
				EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());
				auto bits = earlyVisComponent->inFrust;	
				if(bits == 0)
					break;

				if(i.cast_shadows)
				{
					ShadowComponent* shadowComp = shadowSystem->GetComponent(i.get_entity());
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegPointCasterSphere(i.hdr_color, i.rangeInvSqr, i.area_data, i.pos, shadowComp->proj, i.get_id());
					)
				}
				else
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegPointLightSphere(i.hdr_color, i.rangeInvSqr, i.area_data, i.pos);
					)
			}
			break;

		case LIGHT_TYPE_TUBE:
			{
				EarlyVisibilityComponent* earlyVisComponent = earlyVisibilitySys->GetComponent(i.get_entity());
				auto bits = earlyVisComponent->inFrust;	
				if(bits == 0)
					break;

				if(i.cast_shadows)
				{
					ShadowComponent* shadowComp = shadowSystem->GetComponent(i.get_entity());
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegPointCasterTube(i.hdr_color, i.rangeInvSqr, i.area_data, i.pos, i.dir, shadowComp->proj, shadowComp->view, i.get_id());
					)
				}
				else
					ITERATE_FRUSTUMS(
						((SceneRenderMgr*)f->rendermgr)->RegPointLightTube(i.hdr_color, i.rangeInvSqr, i.area_data, i.pos, i.dir);
					)
			}
			break;
		}
	}
}

void LightSystem::Update()
{
	for(auto& i: *components.data())
	{
		if(!i.active || !i.dirty)
			continue;

		switch (i.type)
		{
		case LIGHT_TYPE_SPOT:
		case LIGHT_TYPE_DISK:
		case LIGHT_TYPE_RECT:
			updateSpot(i);
			break;

		case LIGHT_TYPE_POINT:
		case LIGHT_TYPE_SPHERE:
		case LIGHT_TYPE_TUBE:
			updatePoint(i);
			break;
		}
		i.dirty = false;
	}
}

void LightSystem::updateSpot(LightComponent& comp)
{
	TransformComponent* transformComponent = transformSys->GetComponent(comp.get_entity());

	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, transformComponent->worldMatrix);
	XMMATRIX transform = XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

	XMVECTOR lightPos = XMVector3TransformCoord(XMVectorSet(0,0,0,1), transform);
	XMStoreFloat3(&comp.pos, lightPos);

	XMVECTOR lightDir = XMVector3TransformNormal(DIR_VECT, transform);
	XMStoreFloat3(&comp.dir, lightDir);

	XMVECTOR lightUp = XMVector3TransformNormal(UP_VECT, transform);
	XMStoreFloat3(&comp.dir_up, lightUp);

	XMVECTOR lightSide = XMVector3Cross(lightDir, lightUp);
	XMStoreFloat3(&comp.dir_side, lightSide);
			
	if(comp.type != LIGHT_TYPE_SPOT)
	{
		XMVECTOR vpos = lightPos - lightDir * comp.virt_clip;
		XMStoreFloat3(&comp.virt_pos, vpos); 
	}
	else
		comp.virt_pos = comp.pos;
}

void LightSystem::updatePoint(LightComponent& comp)
{
	TransformComponent* transformComponent = transformSys->GetComponent(comp.get_entity());

	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, transformComponent->worldMatrix);
	XMMATRIX transform = XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

	XMVECTOR lightPos = XMVector3TransformCoord(XMVectorSet(0,0,0,1), transform);
	XMStoreFloat3(&comp.pos, lightPos);

	XMVECTOR lightDir = XMVector3TransformNormal(DIR_VECT, transform);
	XMStoreFloat3(&comp.dir, lightDir);

	XMVECTOR lightUp = XMVector3TransformNormal(UP_VECT, transform);
	XMStoreFloat3(&comp.dir_up, lightUp);
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == ENTITY_COUNT)	return res;\
	auto& comp = components.getDataByArrayIdx(idx);

bool LightSystem::IsDirty(Entity e)
{
	GET_COMPONENT(false)
	return comp.dirty;
}

bool LightSystem::SetDirty(Entity e)
{
	GET_COMPONENT(false)
	comp.dirty = true;
	return true;
}

uint32_t LightSystem::Serialize(Entity e, uint8_t* data)
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
	*(XMFLOAT2*)t_data = comp.area;
	t_data += sizeof(XMFLOAT2);	
	size += sizeof(XMFLOAT2);
	*t_data = comp.type;
	t_data += sizeof(uint8_t);	
	size += sizeof(uint8_t);
	*(float*)t_data = comp.range;
	t_data += sizeof(float);	
	size += sizeof(float);
	*(XMFLOAT2*)t_data = comp.cone;
	t_data += sizeof(XMFLOAT2);	
	size += sizeof(XMFLOAT2);
	*(bool*)t_data = comp.cast_shadows;
	t_data += sizeof(bool);	
	size += sizeof(bool);
	*(bool*)t_data = comp.transparent_shadows;
	t_data += sizeof(bool);	
	size += sizeof(bool);

	return size;
}

uint32_t LightSystem::Deserialize(Entity e, uint8_t* data)
{
	auto t_data = data;
	uint32_t size = 0;

	LightComponent comp;
	comp.active = *(bool*)t_data;
	t_data += sizeof(bool);
	size += sizeof(bool);
	comp.color = *(XMFLOAT3*)t_data;
	t_data += sizeof(XMFLOAT3);
	size += sizeof(XMFLOAT3);
	comp.brightness = *(float*)t_data;
	t_data += sizeof(float);
	size += sizeof(float);
	comp.area = *(XMFLOAT2*)t_data;
	t_data += sizeof(XMFLOAT2);
	size += sizeof(XMFLOAT2);
	comp.type = *(uint8_t*)t_data;
	t_data += sizeof(uint8_t);
	size += sizeof(uint8_t);
	comp.range = *(float*)t_data;
	t_data += sizeof(float);
	size += sizeof(float);
	comp.cone = *(XMFLOAT2*)t_data;
	t_data += sizeof(XMFLOAT2);
	size += sizeof(XMFLOAT2);
	comp.cast_shadows = *(bool*)t_data;
	t_data += sizeof(bool);
	size += sizeof(bool);
	comp.transparent_shadows = *(bool*)t_data;
	t_data += sizeof(bool);
	size += sizeof(bool);

	AddComponent(e, comp);
	return size;
}

bool LightSystem::IsActive(Entity e)
{
	GET_COMPONENT(false)
	return comp.active;
}

bool LightSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(false)
	comp.active = active;
	return true;
}

bool LightSystem::SetType(Entity e, uint8_t type)
{
	GET_COMPONENT(false)
	comp.type = type;
	comp.dirty = true;
	return true;
}
uint8_t LightSystem::GetType(Entity e)
{
	GET_COMPONENT(0)
	return comp.type;
}

bool LightSystem::SetColor(Entity e, XMFLOAT3 color)
{
	GET_COMPONENT(false)
	comp.color = color;
	comp.dirty = true;
	return true;
}
XMFLOAT3 LightSystem::GetColor(Entity e)
{
	GET_COMPONENT(XMFLOAT3())
	return comp.color;
}

bool LightSystem::SetBrightness(Entity e, float brightness)
{
	GET_COMPONENT(false)
	comp.brightness = brightness;
	comp.dirty = true;
	return true;
}

float LightSystem::GetBrightness(Entity e)
{
	GET_COMPONENT(0.0f)
	return comp.brightness;
}
		
bool LightSystem::SetRange(Entity e, float range)
{
	GET_COMPONENT(false)
	comp.range = range;
	comp.dirty = true;
	return true;
}
float LightSystem::GetRange(Entity e)
{
	GET_COMPONENT(0)
	return comp.range;
}
		
bool LightSystem::SetConeIn(Entity e, float conein)
{
	GET_COMPONENT(false)
	comp.cone.x = conein;
	comp.dirty = true;
	return true;
}
float LightSystem::GetConeIn(Entity e)
{
	GET_COMPONENT(0.0f)
	return comp.cone.x;
}
bool LightSystem::SetConeOut(Entity e, float coneout)
{
	GET_COMPONENT(false)
	comp.cone.y = coneout;
	comp.dirty = true;
	return true;
}
float LightSystem::GetConeOut(Entity e)
{
	GET_COMPONENT(0.0f)
	return comp.cone.y;
}
		
bool LightSystem::SetAreaX(Entity e, float areax)
{
	GET_COMPONENT(false)
	comp.area.x = areax;
	comp.dirty = true;
	return true;
}
float LightSystem::GetAreaX(Entity e)
{
	GET_COMPONENT(0.0f)
	return comp.area.x;
}		
bool LightSystem::SetAreaY(Entity e, float areay)
{
	GET_COMPONENT(false)
	comp.area.y = areay;
	comp.dirty = true;
	return true;
}
float LightSystem::GetAreaY(Entity e)
{
	GET_COMPONENT(0.0f)
	return comp.area.y;
}
		
bool LightSystem::SetCastShadows(Entity e, bool cast)
{
	GET_COMPONENT(false)
	comp.cast_shadows = cast;
	initShadows(e, &comp);
	comp.dirty = true;
	return true;
}
bool LightSystem::GetCastShadows(Entity e)
{
	GET_COMPONENT(false)
	return comp.cast_shadows;
}
		
bool LightSystem::SetTransparentShadows(Entity e, bool cast)
{
	GET_COMPONENT(false)
	comp.transparent_shadows = cast;
	comp.dirty = true;
	return true;
}
bool LightSystem::GetTransparentShadows(Entity e)
{
	GET_COMPONENT(false)
	return comp.transparent_shadows;
}

void LightSystem::UpdateShadows(Entity e)
{
	GET_COMPONENT(void())
	if(comp.cast_shadows)
		initShadows(e, &comp);
}

void LightSystem::initShadows(Entity e, LightComponent* comp)
{
	shadowSystem->DeleteComponents(e);

	if(!comp->cast_shadows)
	{
		shadowSystem->DeleteComponents(e);
		return;
	}

	switch (comp->type)
	{
	case LIGHT_TYPE_SPOT:
	case LIGHT_TYPE_DISK:
	case LIGHT_TYPE_RECT:
		{
			shadowSystem->AddComponent(e);
			shadowSystem->UpdateShadowmapData(e);
		}
		break;
	case LIGHT_TYPE_POINT:
	case LIGHT_TYPE_SPHERE:
	case LIGHT_TYPE_TUBE:
		{
			for(uint i=0; i<6; i++)
				shadowSystem->AddComponent(e);
			shadowSystem->UpdateShadowmapData(e);
		}
		break;
	}
}

void LightSystem::UpdateLightProps(Entity e)
{
	GET_COMPONENT(void())

	comp.hdr_color.w = comp.brightness;
	comp.hdr_color.x = comp.color.x * comp.brightness;
	comp.hdr_color.y = comp.color.y * comp.brightness;
	comp.hdr_color.z = comp.color.z * comp.brightness;

	comp.range = max(comp.range, 0.001f);
	comp.rangeInvSqr = 1.0f / (comp.range * comp.range);

	if(comp.type >= 3)
	{
		float angleOffset = -cos(comp.cone.y);
		comp.cone_data.x = 1.0f / max(0.005f, cos(comp.cone.x) + angleOffset);
		comp.cone_data.y = angleOffset * comp.cone_data.x;

		switch (comp.type)
		{
		case LIGHT_TYPE_SPOT:
			comp.virt_clip = 0;
			break;
		case LIGHT_TYPE_DISK:
			comp.area.x = max(comp.area.x, 0.05f);
			comp.area_data.x = comp.area.x;
			comp.area_data.y = comp.area.x * comp.area.x;
			comp.area_data.z = 0;
			comp.virt_clip = (max(comp.area_data.x, comp.area_data.y * 2) / tan(comp.cone.y));
			break;
		case LIGHT_TYPE_RECT:
			comp.area.x = max(comp.area.x, 0.05f);
			comp.area.y = max(comp.area.y, 0.05f);
			comp.area_data.x = sqrt(comp.area.x * comp.area.x + comp.area.y * comp.area.y);
			comp.area_data.y = comp.area.y * 0.5f;
			comp.area_data.z = comp.area.x * 0.5f;
			comp.virt_clip = (max(comp.area_data.x, comp.area_data.y * 2) / tan(comp.cone.y));
			break;
		}

		updateSpot(comp);

		earlyVisibilitySys->SetType(comp.get_entity(), BoundingType::BT_FRUSTUM_SPHERE);
		XMMATRIX proj = XMMatrixPerspectiveFovLH(comp.cone.y * 2.0f, 1.0f, SHADOW_NEARCLIP + comp.virt_clip, comp.range + comp.virt_clip);

		BoundingFrustum frust = BoundingFrustum(proj);
		// because Y-up
		frust.Transform(frust, XMMatrixTranslation(0, 0, -comp.virt_clip) * XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0, 0));

		earlyVisibilitySys->SetBFrustum(comp.get_entity(), frust);
		earlyVisibilitySys->SetBSphere(comp.get_entity(), BoundingSphere(XMFLOAT3(0,0,0), comp.range));
	}
	else
	{
		switch (comp.type)
		{
		case LIGHT_TYPE_POINT:
			break;
		case LIGHT_TYPE_SPHERE:
			comp.area.x = max(comp.area.x, 0.05f);
			comp.area_data.x = comp.area.x;
			comp.area_data.y = comp.area.x * comp.area.x;
			comp.area_data.z = 0;
			break;
		case LIGHT_TYPE_TUBE:
			comp.area.x = max(comp.area.x, 0.05f);
			comp.area.y = max(comp.area.y, 0.05f);
			comp.area_data.x = comp.area.x;
			comp.area_data.y = comp.area.y;
			comp.area_data.z = comp.area.x * comp.area.x;
			break;
		}

		comp.virt_clip = 0;
		updatePoint(comp);
	
		earlyVisibilitySys->SetType(comp.get_entity(), BoundingType::BT_SPHERE);
		earlyVisibilitySys->SetBSphere(comp.get_entity(), BoundingSphere(XMFLOAT3(0,0,0), comp.range));
	}
}

XMMATRIX LightSystem::GetProj(Entity e)
{
	GET_COMPONENT(XMMatrixIdentity())

	switch (comp.type)
	{
	case LIGHT_TYPE_SPOT:
	case LIGHT_TYPE_DISK:
	case LIGHT_TYPE_RECT:
		return XMMatrixPerspectiveFovLH(comp.cone.y * 2.0f, 1.0f, SHADOW_NEARCLIP + comp.virt_clip, comp.range + comp.virt_clip);

	case LIGHT_TYPE_POINT:
	case LIGHT_TYPE_SPHERE:
	case LIGHT_TYPE_TUBE:
		return XMMatrixPerspectiveFovLH(XM_PIDIV2 + SHADOW_POINT_FOVADD, 1.0f, SHADOW_NEARCLIP, comp.range);
	}
	return XMMatrixIdentity();
}

XMMATRIX LightSystem::GetView(Entity e, uchar num, XMVECTOR* pos)
{
	GET_COMPONENT(XMMatrixIdentity())

	switch (comp.type)
	{
	case LIGHT_TYPE_SPOT:
	case LIGHT_TYPE_DISK:
	case LIGHT_TYPE_RECT:
		*pos = XMLoadFloat3(&comp.virt_pos);
		return XMMatrixLookAtLH( *pos, *pos + XMLoadFloat3(&comp.dir), -XMLoadFloat3(&comp.dir_up) );

	case LIGHT_TYPE_POINT:
	case LIGHT_TYPE_SPHERE:
		{
			XMVECTOR dir, up;
			switch (num)
			{
			case 0:
				dir = XMVectorSet(1,0,0,0);
				up = XMVectorSet(0,1,0,0);
				break;
			case 1:
				dir = XMVectorSet(-1,0,0,0);
				up = XMVectorSet(0,1,0,0);
				break;
			case 2:
				dir = XMVectorSet(0,0,1,0);
				up = XMVectorSet(0,1,0,0);
				break;
			case 3:
				dir = XMVectorSet(0,0,-1,0);
				up = XMVectorSet(0,1,0,0);
				break;
			case 4:
				dir = XMVectorSet(0,1,0,0);
				up = XMVectorSet(1,0,0,0);
				break;
			case 5:
				dir = XMVectorSet(0,-1,0,0);
				up = XMVectorSet(-1,0,0,0);
				break;
			}

			*pos = XMLoadFloat3(&comp.pos);
			return XMMatrixLookAtLH( *pos, *pos + dir, up );
		}
	case LIGHT_TYPE_TUBE:
		{
			XMVECTOR dir = XMLoadFloat3(&comp.dir);
			XMVECTOR up = XMLoadFloat3(&comp.dir_up);
			switch (num)
			{
			case 0:
				break;
			case 1:
				dir = -dir;
				break;
			case 2:
				{
					XMMATRIX transform = XMMatrixRotationNormal(up, XM_PIDIV2);
					dir = XMVector3TransformNormal(dir, transform);
				}
				break;
			case 3:
				{
					XMMATRIX transform = XMMatrixRotationNormal(up, -XM_PIDIV2);
					dir = XMVector3TransformNormal(dir, transform);
				}
				break;
			case 4:
				{
					XMVECTOR temp = dir;
					dir = up;
					up = temp;
				}
				break;
			case 5:
				{
					XMVECTOR temp = dir;
					dir = -up;
					up = -temp;
				}
				break;
			}

			*pos = XMLoadFloat3(&comp.pos);
			return XMMatrixLookAtLH( *pos, *pos + dir, up );
		}
	}
	return XMMatrixIdentity();
}