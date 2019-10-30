#include "stdafx.h"
#include "LightFrameMgr.h"
#include "ScenePipeline.h"
#include "EnvProbMgr.h"

using namespace EngineCore;

LightFrameMgr::LightFrameMgr()
{
	cameraPosition = Vector3::Zero;
}

void LightFrameMgr::ClearPerFrame()
{
	globalLightBuffer.clear();

	if (shadowsRenderer)
		shadowsRenderer->ClearPerFrame();
}


void LightFrameMgr::RegSpotLight(uint8_t type, Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos)
{
	if (globalLightBuffer.size() >= LIGHT_PROB_FRAME_MAX)
	{
		ERR("Per frame lights & probs overflow");
		return;
	}

	UnifiedDataPackage* dataPtr = globalLightBuffer.push_back();
	dataPtr->SetSpotLight(type, color, range, area, cone, pos, dir, up, side, virtpos);
}

void LightFrameMgr::RegSpotCaster(uint8_t type, Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos, Vector4& farNear,
	CXMMATRIX vp, CXMMATRIX proj, uint64_t id)
{
	if (globalLightBuffer.size() >= LIGHT_PROB_FRAME_MAX)
	{
		ERR("Per frame lights & probs overflow");
		return;
	}

	ShadowMap& shadowMap = shadowsRenderer->GetShadowAdress(id);
	
	UnifiedDataPackage* dataPtr = globalLightBuffer.push_back();
	dataPtr->SetSpotCaster(type, color, range, area, cone, pos, dir, up, side, virtpos, farNear, vp, proj, shadowMap);
}

void LightFrameMgr::RegPointLight(uint8_t type, Vector4& color, float range, Vector3& area, Vector3& pos, Vector3& dir)
{
	if (globalLightBuffer.size() >= LIGHT_PROB_FRAME_MAX)
	{
		ERR("Per frame lights & probs overflow");
		return;
	}

	UnifiedDataPackage* dataPtr = globalLightBuffer.push_back();
	dataPtr->SetPointLight(type, color, range, area, pos, dir);
}

void LightFrameMgr::RegPointCaster(uint8_t type, Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector3& pos, Vector3& dir, Vector4& farNear, CXMMATRIX proj, CXMMATRIX view, uint64_t id)
{
	if (globalLightBuffer.size() >= LIGHT_PROB_FRAME_MAX)
	{
		ERR("Per frame lights & probs overflow");
		return;
	}

	ShadowMap* shadowMaps[6];
	shadowMaps[0] = &shadowsRenderer->GetShadowAdress(id);
	for (uint32_t i = 0; i < 5; i++)
		shadowMaps[i + 1] = &shadowsRenderer->GetShadowAdressNext(*shadowMaps[i]);

	UnifiedDataPackage* dataPtr = globalLightBuffer.push_back();
	dataPtr->SetPointCaster(type, color, range, area, pos, dir, farNear, proj, view, shadowMaps);

	//TODO:
	//casterPoint_array[casterPoint_count].matView = XMMatrixSet(1.0f, 0, 0, -pos.x, 0, 1.0f, 0, -pos.y, 0, 0, 1.0f, -pos.z, 0, 0, 0, 1.0f);
}

void LightFrameMgr::RegDirLight(Vector4& color, Vector2& area, Vector3& dir, XMMATRIX* view_proj, Vector3* pos, uint64_t id)
{
	if (lightDirectionalBuffer.size() >= LIGHT_DIR_FRAME_MAX)
	{
		ERR("Per frame lights & probs overflow");
		return;
	}

	ShadowMap* shadowMaps[LIGHT_DIR_NUM_CASCADES];
	shadowMaps[0] = &shadowsRenderer->GetShadowAdress(id);
	for (uint8_t i = 0; i<LIGHT_DIR_NUM_CASCADES - 1; i++)
		shadowMaps[i + 1] = &shadowsRenderer->GetShadowAdressNext(*shadowMaps[i]);

	lightDirectionalBuffer.push_back(DirLightBuffer(color, area, dir, view_proj, pos, shadowMaps));
}

void LightFrameMgr::RegEnvProb(const EnvProbData& data)
{
	envProbMgr->AddEnvProb(data, cameraPosition);
}

void LightFrameMgr::ClearPerFrame()
{
	globalLightBuffer.clear();
	lightDirectionalBuffer.clear();

	if (shadowsRenderer)
		shadowsRenderer->ClearPerFrame();
}