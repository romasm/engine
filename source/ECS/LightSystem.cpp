#include "stdafx.h"
#include "LightSystem.h"
#include "World.h"
#include "WorldMgr.h"

using namespace EngineCore;

LightSystem::LightSystem(BaseWorld* w, uint32_t maxCount)
{
	world = w;
	frustumMgr = w->GetFrustumMgr();

	transformSys = w->GetTransformSystem();
	earlyVisibilitySys = w->GetEarlyVisibilitySystem();
	shadowSystem = w->GetShadowSystem();
	
	maxCount = min<uint32_t>(maxCount, ENTITY_COUNT);
	components.create(maxCount);
}

#define UP_VECT XMVectorSet(1,0,0,0)
#define DIR_VECT XMVectorSet(0,-1,0,0)

#define ITERATE_FRUSTUMS(code) for(auto f: frustumMgr->camDataArray){ \
	if((bits & f->bit) == f->bit){ \
	code \
	bits &= ~f->bit; if(bits == 0) break;}}

void LightSystem::RegShadowMaps()
{
	for(auto& i: *components.data())
	{
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.cast_shadows)
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
					((SceneRenderMgr*)f->rendermgr)->shadowsRenderer->RegShadowMap(i.get_id(), FrustumMgr::CalcScreenSize(f->frustum, shadowComp->worldFrustum));
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
						((SceneRenderMgr*)f->rendermgr)->shadowsRenderer->RegShadowMap(i.get_id(), FrustumMgr::CalcScreenSize(f->frustum, shadowComp->worldFrustum));
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
		if( !world->IsEntityNeedProcess(i.get_entity()) )
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
						((SceneRenderMgr*)f->rendermgr)->RegSpotCaster(i.hdr_color, i.nonAreaColor, i.rangeInvSqr, i.cone_data, i.pos, i.dir, i.farNearData, 
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
						((SceneRenderMgr*)f->rendermgr)->RegSpotCasterDisk(i.hdr_color, i.nonAreaColor, i.rangeInvSqr, i.area_data, i.cone_data, i.pos, i.dir, i.virt_pos, 
							i.farNearData, shadowComp->view_proj, shadowComp->proj, i.get_id());
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
						((SceneRenderMgr*)f->rendermgr)->RegSpotCasterRect(i.hdr_color, i.nonAreaColor, i.rangeInvSqr, i.area_data, i.cone_data, i.pos, i.dir, i.dir_up, i.dir_side, i.virt_pos, 
							i.farNearData, shadowComp->view_proj, shadowComp->proj, i.get_id());
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
						((SceneRenderMgr*)f->rendermgr)->RegPointCaster(i.hdr_color, i.nonAreaColor, i.rangeInvSqr, i.pos, i.farNearData, shadowComp->proj, i.get_id());
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
						((SceneRenderMgr*)f->rendermgr)->RegPointCasterSphere(i.hdr_color, i.nonAreaColor, i.rangeInvSqr, i.area_data, i.pos, i.farNearData, shadowComp->proj, i.get_id());
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
						((SceneRenderMgr*)f->rendermgr)->RegPointCasterTube(i.hdr_color, i.nonAreaColor, i.rangeInvSqr, i.area_data, i.pos, i.dir, i.farNearData, shadowComp->proj, shadowComp->view, i.get_id());
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
		if( !world->IsEntityNeedProcess(i.get_entity()) )
			continue;

		if(!i.dirty)
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
	XMMATRIX worldMatrix = transformSys->GetTransform_WInternal(comp.get_entity());

	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, worldMatrix);
	XMMATRIX transform = XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

	comp.pos = XMVector3TransformCoord(XMVectorSet(0,0,0,1), transform);
	comp.dir = XMVector3TransformNormal(DIR_VECT, transform);
	comp.dir_up = XMVector3TransformNormal(UP_VECT, transform);
	comp.dir_side = comp.dir.Cross(comp.dir_up);
			
	if(comp.type != LIGHT_TYPE_SPOT)
		comp.virt_pos = comp.pos - comp.dir * comp.virt_clip;
	else
		comp.virt_pos = comp.pos;
}

void LightSystem::updatePoint(LightComponent& comp)
{
	XMMATRIX worldMatrix = transformSys->GetTransform_WInternal(comp.get_entity());

	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&scale, &rot, &pos, worldMatrix);
	XMMATRIX transform = XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

	XMVECTOR lightPos = XMVector3TransformCoord(XMVectorSet(0,0,0,1), transform);
	XMStoreFloat3(&comp.pos, lightPos);

	XMVECTOR lightDir = XMVector3TransformNormal(DIR_VECT, transform);
	XMStoreFloat3(&comp.dir, lightDir);

	XMVECTOR lightUp = XMVector3TransformNormal(UP_VECT, transform);
	XMStoreFloat3(&comp.dir_up, lightUp);
}

#define GET_COMPONENT(res) size_t idx = components.getArrayIdx(e.index());\
	if(idx == components.capacity())	return res;\
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

	*(Vector3*)t_data = comp.color;
	t_data += sizeof(Vector3);
	size += sizeof(Vector3);
	*(float*)t_data = comp.brightness;
	t_data += sizeof(float);
	size += sizeof(float);
	*(Vector2*)t_data = comp.area;
	t_data += sizeof(Vector2);	
	size += sizeof(Vector2);
	*t_data = comp.type;
	t_data += sizeof(uint8_t);	
	size += sizeof(uint8_t);
	*(float*)t_data = comp.range;
	t_data += sizeof(float);	
	size += sizeof(float);
	*(Vector2*)t_data = comp.cone;
	t_data += sizeof(Vector2);	
	size += sizeof(Vector2);
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
	comp.color = *(Vector3*)t_data;
	t_data += sizeof(Vector3);
	size += sizeof(Vector3);
	comp.brightness = *(float*)t_data;
	t_data += sizeof(float);
	size += sizeof(float);
	comp.area = *(Vector2*)t_data;
	t_data += sizeof(Vector2);
	size += sizeof(Vector2);
	comp.type = *(uint8_t*)t_data;
	t_data += sizeof(uint8_t);
	size += sizeof(uint8_t);
	comp.range = *(float*)t_data;
	t_data += sizeof(float);
	size += sizeof(float);
	comp.cone = *(Vector2*)t_data;
	t_data += sizeof(Vector2);
	size += sizeof(Vector2);
	comp.cast_shadows = *(bool*)t_data;
	t_data += sizeof(bool);
	size += sizeof(bool);
	comp.transparent_shadows = *(bool*)t_data;
	t_data += sizeof(bool);
	size += sizeof(bool);

	// temp
	calcNonAreaBrightness(comp);

	AddComponent(e, comp);

	return size;
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

bool LightSystem::SetColor(Entity e, Vector3 color)
{
	GET_COMPONENT(false)
	comp.color = color;
	comp.dirty = true;
	return true;
}
Vector3 LightSystem::GetColor(Entity e)
{
	GET_COMPONENT(Vector3())
	return comp.color;
}

bool LightSystem::SetBrightness(Entity e, float brightness) // TODO: move intensity calc code in c++ from lua
{
	GET_COMPONENT(false)
	comp.brightness = brightness;

	// temp
	calcNonAreaBrightness(comp);

	comp.dirty = true;
	return true;
}

void LightSystem::calcNonAreaBrightness(LightComponent& comp)
{
	switch (comp.type)
	{
	case LIGHT_TYPE_SPOT:
		comp.nonAreaBrightness = comp.brightness;
		break;
	case LIGHT_TYPE_DISK:
		comp.nonAreaBrightness = comp.brightness * comp.area.x * comp.area.x * XM_PI;
		break;
	case LIGHT_TYPE_RECT:
		comp.nonAreaBrightness = comp.brightness * comp.area.x * comp.area.y;
		break;
	case LIGHT_TYPE_POINT:
		comp.nonAreaBrightness = comp.brightness;
		break;
	case LIGHT_TYPE_SPHERE:
		comp.nonAreaBrightness = comp.brightness * comp.area.x * comp.area.x * XM_PI;
		break;
	case LIGHT_TYPE_TUBE:
		comp.nonAreaBrightness = comp.brightness *  0.5f * XM_PI * comp.area.x * (comp.area.y + 2 * comp.area.x);
		break;
	}
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
			for(uint32_t i=0; i<6; i++)
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

	comp.nonAreaColor.w = comp.nonAreaBrightness;
	comp.nonAreaColor.x = comp.color.x * comp.nonAreaBrightness;
	comp.nonAreaColor.y = comp.color.y * comp.nonAreaBrightness;
	comp.nonAreaColor.z = comp.color.z * comp.nonAreaBrightness;

	comp.range = max<float>(comp.range, 0.001f);
	comp.rangeInvSqr = 1.0f / (comp.range * comp.range);

	if(comp.type >= 3)
	{
		float angleOffset = -cos(comp.cone.y);
		comp.cone_data.x = 1.0f / max<float>(0.005f, cos(comp.cone.x) + angleOffset);
		comp.cone_data.y = angleOffset * comp.cone_data.x;

		switch (comp.type)
		{
		case LIGHT_TYPE_SPOT:
			comp.virt_clip = 0;
			break;
		case LIGHT_TYPE_DISK:
			comp.area.x = max<float>(comp.area.x, 0.05f);
			comp.area_data.x = comp.area.x;
			comp.area_data.y = comp.area.x * comp.area.x;
			comp.area_data.z = 0;
			comp.virt_clip = (max<float>(comp.area_data.x, comp.area_data.y * 2) / tan(comp.cone.y));
			break;
		case LIGHT_TYPE_RECT:
			comp.area.x = max<float>(comp.area.x, 0.05f);
			comp.area.y = max<float>(comp.area.y, 0.05f);
			comp.area_data.x = sqrt(comp.area.x * comp.area.x + comp.area.y * comp.area.y);
			comp.area_data.y = comp.area.y * 0.5f;
			comp.area_data.z = comp.area.x * 0.5f;
			comp.virt_clip = (max<float>(comp.area_data.x, comp.area_data.y * 2) / tan(comp.cone.y));
			break;
		}

		updateSpot(comp);

		earlyVisibilitySys->SetType(comp.get_entity(), BoundingType::BT_FRUSTUM_SPHERE);
		XMMATRIX proj = XMMatrixPerspectiveFovLH(comp.cone.y * 2.0f, 1.0f, SHADOW_NEARCLIP + comp.virt_clip, comp.range + comp.virt_clip);

		BoundingFrustum frust = BoundingFrustum(proj);
		// because Y-up
		frust.Transform(frust, XMMatrixTranslation(0, 0, -comp.virt_clip) * XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0, 0));

		earlyVisibilitySys->SetBFrustum(comp.get_entity(), frust);
		earlyVisibilitySys->SetBSphere(comp.get_entity(), BoundingSphere(Vector3(0,0,0), comp.range));
	}
	else
	{
		switch (comp.type)
		{
		case LIGHT_TYPE_POINT:
			break;
		case LIGHT_TYPE_SPHERE:
			comp.area.x = max<float>(comp.area.x, 0.05f);
			comp.area_data.x = comp.area.x;
			comp.area_data.y = comp.area.x * comp.area.x;
			comp.area_data.z = 0;
			break;
		case LIGHT_TYPE_TUBE:
			comp.area.x = max<float>(comp.area.x, 0.05f);
			comp.area.y = max<float>(comp.area.y, 0.05f);
			comp.area_data.x = comp.area.x;
			comp.area_data.y = comp.area.y;
			comp.area_data.z = comp.area.x * comp.area.x;
			break;
		}

		comp.virt_clip = 0;
		updatePoint(comp);
	
		earlyVisibilitySys->SetType(comp.get_entity(), BoundingType::BT_SPHERE);
		earlyVisibilitySys->SetBSphere(comp.get_entity(), BoundingSphere(Vector3(0,0,0), comp.range));
	}

	comp.farNearData.x = SHADOW_NEARCLIP + comp.virt_clip;
	comp.farNearData.y = comp.range + comp.virt_clip;
	comp.farNearData.z = comp.farNearData.x * comp.farNearData.y;
	comp.farNearData.w = comp.farNearData.y - comp.farNearData.x;
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

XMMATRIX LightSystem::GetView(Entity e, uint8_t num, Vector3* pos)
{
	GET_COMPONENT(XMMatrixIdentity())

	switch (comp.type)
	{
	case LIGHT_TYPE_SPOT:
	case LIGHT_TYPE_DISK:
	case LIGHT_TYPE_RECT:
		*pos = comp.virt_pos;
		return XMMatrixLookAtLH( comp.virt_pos, comp.virt_pos + comp.dir, -comp.dir_up );

	case LIGHT_TYPE_POINT:
	case LIGHT_TYPE_SPHERE:
		{
			Vector3 dir, up;
			switch (num)
			{
			case 0:
				dir = Vector3(1,0,0);
				up = Vector3(0,1,0);
				break;
			case 1:
				dir = Vector3(-1,0,0);
				up = Vector3(0,1,0);
				break;
			case 2:
				dir = Vector3(0,0,1);
				up = Vector3(0,1,0);
				break;
			case 3:
				dir = Vector3(0,0,-1);
				up = Vector3(0,1,0);
				break;
			case 4:
				dir = Vector3(0,1,0);
				up = Vector3(1,0,0);
				break;
			case 5:
				dir = Vector3(0,-1,0);
				up = Vector3(-1,0,0);
				break;
			}

			*pos = comp.pos;
			return XMMatrixLookAtLH( comp.pos, comp.pos + dir, up );
		}
	case LIGHT_TYPE_TUBE: // TODO: broked in voxels
		{
			Vector3 dir = comp.dir;
			Vector3 up = comp.dir_up;
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

			*pos = comp.pos;
			return XMMatrixLookAtLH( comp.pos, comp.pos + dir, up );
		}
	}
	return XMMatrixIdentity();
}