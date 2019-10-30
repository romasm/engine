#include "stdafx.h"
#include "RenderMgrs.h"
#include "ScenePipeline.h"
#include "CameraSystem.h"
#include "ECS\EnvProbSystem.h"
#include "Utils\Profiler.h"
#include "EnvProbMgr.h"

using namespace EngineCore;

SceneRenderMgr::SceneRenderMgr(bool lightweight) : BaseRenderMgr()
{
	b_shadow = false;
	
	opaque_array.create(lightweight ? (OPAQUE_FRAME_MAX / 16) : OPAQUE_FRAME_MAX);
	alphatest_array.create(lightweight ? (OPAQUE_FRAME_ALPHATEST_MAX / 16) : OPAQUE_FRAME_ALPHATEST_MAX);
	transparent_array.create(lightweight ? (TRANSPARENT_FRAME_MAX / 16) : TRANSPARENT_FRAME_MAX);

	hud_array.create(lightweight ? (HUD_FRAME_MAX / 16) : HUD_FRAME_MAX);
	ovhud_array.create(lightweight ? (OV_HUD_FRAME_MAX / 16) : OV_HUD_FRAME_MAX);
	
	currentCamera = nullptr;

	if(lightweight)
	{
		lightSpot_array = nullptr;
		lightPoint_array = nullptr;
		lightDir_array = nullptr;
		casterSpot_array = nullptr;
		casterPoint_array = nullptr;		

		shadowsRenderer = nullptr;
		envProbMgr = new EnvProbMgr(true);
	}
	else
	{
		lightSpot_array = new SpotLightBuffer[LIGHT_SPOT_FRAME_MAX];
		lightPoint_array = new PointLightBuffer[LIGHT_POINT_FRAME_MAX];
		lightDir_array = new DirLightBuffer[LIGHT_DIR_FRAME_MAX];
		casterSpot_array = new SpotCasterBuffer[CASTER_SPOT_FRAME_MAX];
		casterPoint_array = new PointCasterBuffer[CASTER_POINT_FRAME_MAX];

		shadowsRenderer = new ShadowsRenderer(this);
		envProbMgr = new EnvProbMgr(false);
	}
	
	ClearAll();
}

SceneRenderMgr::~SceneRenderMgr()
{
	ClearAll();
	
	_DELETE_ARRAY(lightSpot_array);
	_DELETE_ARRAY(lightPoint_array);
	_DELETE_ARRAY(lightDir_array);
	_DELETE_ARRAY(casterSpot_array);
	_DELETE_ARRAY(casterPoint_array);
		
	_DELETE(shadowsRenderer);
	_DELETE(envProbMgr);
}

void SceneRenderMgr::cleanRenderArrayLights()
{
	lightSpot_count = 0;
	lightPoint_count = 0;
	lightDir_count = 0;
	casterSpot_count = 0;
	casterPoint_count = 0;
	
	if(shadowsRenderer)
		shadowsRenderer->ClearPerFrame();
}

bool SceneRenderMgr::RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
							  uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, IA_TOPOLOGY topo)
{return RegMesh(indexCount, indexBuffer, vertexBuffer, vertexSize, isSkinned, gpuMatrixBuffer, material, Vector3(currentCamera->far_clip,0,0), topo);}

bool SceneRenderMgr::RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
			uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, Vector3& center, IA_TOPOLOGY topo)
{
	if( !material || gpuMatrixBuffer == nullptr || indexCount == 0 )
		return false;
	
	uint32_t skinned = isSkinned ? 1 : 0;

	bool has_tq = false;
	const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + skinned);
	auto queue = material->GetTechQueue(tech, &has_tq);
	if(!has_tq)
		return false;
	
	RenderMesh* mesh_new = nullptr;
	switch(queue)
	{
	case SC_ALPHA:
		ERR("TODO: SC_ALPHA");
		break;
	case SC_TRANSPARENT:
		mesh_new = transparent_array.push_back();
		break;
	case SC_OPAQUE:
		mesh_new = opaque_array.push_back();
		break;
	case SC_ALPHATEST:
		mesh_new = alphatest_array.push_back();
		break;
	case GUI_3D:
		mesh_new = hud_array.push_back();
		break;
	case GUI_3D_OVERLAY:
		mesh_new = ovhud_array.push_back();
		break;
	default:
		return false;
	}

	mesh_new->indexCount = indexCount;
	mesh_new->vertexBuffer = vertexBuffer;
	mesh_new->indexBuffer = indexBuffer;
	mesh_new->gpuMatrixBuffer = gpuMatrixBuffer;
	mesh_new->isSkinned = skinned;
	mesh_new->vertexSize = vertexSize;
	mesh_new->material = material;
	mesh_new->topo = topo;
	mesh_new->distanceSq = (center - cameraPosition).LengthSquared();
	
	return true;
}

#define PIXEL_HALF 0.5f

bool SceneRenderMgr::RegSpotLight(uint8_t type, Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos)
{
	if(lightSpot_count >= LIGHT_SPOT_FRAME_MAX)
		return false;
	
	lightSpot_array[lightSpot_count].Type = Vector4(float(type));
	lightSpot_array[lightSpot_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	lightSpot_array[lightSpot_count].ColorConeX = Vector4(color.x, color.y, color.z, cone.x);
	lightSpot_array[lightSpot_count].DirConeY = Vector4(dir.x, dir.y, dir.z, cone.y);
	lightSpot_array[lightSpot_count].DirUpAreaX = Vector4(up.x, up.y, up.z, area.x);
	lightSpot_array[lightSpot_count].DirSideAreaY = Vector4(side.x, side.y, side.z, area.y);
	lightSpot_array[lightSpot_count].VirtposAreaZ = Vector4(virtpos.x, virtpos.y, virtpos.z, area.z);

	lightSpot_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCaster(uint8_t type, Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos, Vector4& farNear,
			CXMMATRIX vp, CXMMATRIX proj, uint64_t id)
{
	if(casterSpot_count >= CASTER_SPOT_FRAME_MAX) // proj tex coords wrong calc
		return false;
	
	ShadowMap& shm = shadowsRenderer->GetShadowAdress(id);

	casterSpot_array[casterSpot_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	casterSpot_array[casterSpot_count].ColorConeX = Vector4(color.x, color.y, color.z, cone.x);
	casterSpot_array[casterSpot_count].DirConeY = Vector4(dir.x, dir.y, dir.z, cone.y);
	casterSpot_array[casterSpot_count].DirUpAreaX = Vector4(up.x, up.y, up.z, area.x);
	casterSpot_array[casterSpot_count].DirSideAreaY = Vector4(side.x, side.y, side.z, area.y);
	casterSpot_array[casterSpot_count].VirtposAreaZ = Vector4(virtpos.x, virtpos.y, virtpos.z, area.z);
	casterSpot_array[casterSpot_count].ShadowmapAdress = Vector4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP,
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	casterSpot_array[casterSpot_count].ShadowmapParamsType = Vector4(PIXEL_HALF / shm.res, max<float>(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), farNear.x, float(type));
	casterSpot_array[casterSpot_count].farNear = farNear;
	casterSpot_array[casterSpot_count].matViewProj = vp;

	casterSpot_count++;

	return true;
}

bool SceneRenderMgr::RegPointLight(uint8_t type, Vector4& color, float range, Vector3& area, Vector3& pos, Vector3& dir)
{
	if(lightPoint_count >= LIGHT_POINT_FRAME_MAX)
		return false;

	lightPoint_array[lightPoint_count].Type = Vector4(float(type));
	lightPoint_array[lightPoint_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	lightPoint_array[lightPoint_count].Color = Vector4(color.x, color.y, color.z, 0);
	lightPoint_array[lightPoint_count].AreaInfo = Vector4(area.x, area.y, area.z, area.y * area.y);
	lightPoint_array[lightPoint_count].DirAreaA = Vector4(dir.x, dir.y, dir.z, area.y + 2 * area.x);

	lightPoint_count++;

	return true;
}

bool SceneRenderMgr::RegPointCaster(uint8_t type, Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector3& pos, Vector3& dir, Vector4& farNear, CXMMATRIX proj, CXMMATRIX view, uint64_t id)
{
	if(casterPoint_count >= CASTER_POINT_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowsRenderer->GetShadowAdress(id);
	for(uint32_t i=0; i<5; i++)
		shm[i+1] = shadowsRenderer->GetShadowAdressNext(shm[i]);
	
	casterPoint_array[casterPoint_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	casterPoint_array[casterPoint_count].ColorShParams = Vector4(color.x, color.y, color.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	casterPoint_array[casterPoint_count].AreaInfo = Vector4(area.x, area.y, area.z, area.y * area.y);
	casterPoint_array[casterPoint_count].DirAreaA = Vector4(dir.x, dir.y, dir.z, area.y + 2 * area.x);
	casterPoint_array[casterPoint_count].ShadowmapParams0 = Vector4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	casterPoint_array[casterPoint_count].ShadowmapParams1Type = Vector4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, float(type), 0);
	casterPoint_array[casterPoint_count].ShadowmapAdress0 = Vector4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	casterPoint_array[casterPoint_count].ShadowmapAdress1 = Vector4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	casterPoint_array[casterPoint_count].ShadowmapAdress2 = Vector4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	casterPoint_array[casterPoint_count].ShadowmapAdress3 = Vector4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	casterPoint_array[casterPoint_count].ShadowmapAdress4 = Vector4(shm[4].x * SHADOWS_BUF_RES_RCP, shm[4].y * SHADOWS_BUF_RES_RCP, 
		shm[4].res * SHADOWS_BUF_RES_RCP, (float)shm[4].dsv);
	casterPoint_array[casterPoint_count].ShadowmapAdress5 = Vector4(shm[5].x * SHADOWS_BUF_RES_RCP, shm[5].y * SHADOWS_BUF_RES_RCP, 
		shm[5].res * SHADOWS_BUF_RES_RCP, (float)shm[5].dsv);
	casterPoint_array[casterPoint_count].farNear = farNear;
	casterPoint_array[casterPoint_count].matProj = proj;

	//TODO: casterPoint_array[casterPoint_count].matView = view;
	casterPoint_array[casterPoint_count].matView = XMMatrixSet(1.0f, 0, 0, -pos.x, 0, 1.0f, 0, -pos.y, 0, 0, 1.0f, -pos.z, 0, 0, 0, 1.0f);
	
	casterPoint_count++;

	return true;
}

bool SceneRenderMgr::RegDirLight(Vector4& color, Vector2& area, Vector3& dir, XMMATRIX* view_proj, Vector3* pos, uint64_t id)
{
	if(lightDir_count >= LIGHT_DIR_FRAME_MAX)
		return false;

	ShadowMap shm[LIGHT_DIR_NUM_CASCADES];
	shm[0] = shadowsRenderer->GetShadowAdress(id);
	for(uint8_t i=0; i<LIGHT_DIR_NUM_CASCADES-1; i++)
		shm[i+1] = shadowsRenderer->GetShadowAdressNext(shm[i]);

	lightDir_array[lightDir_count].ColorAreaX = Vector4(color.x, color.y, color.z, area.x);
	lightDir_array[lightDir_count].DirAreaY = Vector4(dir.x, dir.y, dir.z, area.y);

	// locked for 4 cascades
	lightDir_array[lightDir_count].Pos0 = Vector4(pos[0].x, pos[0].y, pos[0].z, PIXEL_HALF / shm[0].res);
	lightDir_array[lightDir_count].Pos1 = Vector4(pos[1].x, pos[1].y, pos[1].z, PIXEL_HALF / shm[1].res);
	lightDir_array[lightDir_count].Pos2 = Vector4(pos[2].x, pos[2].y, pos[2].z, PIXEL_HALF / shm[2].res);
	lightDir_array[lightDir_count].Pos3 = Vector4(pos[3].x, pos[3].y, pos[3].z, PIXEL_HALF / shm[3].res);
	lightDir_array[lightDir_count].matViewProj0 = view_proj[0];
	lightDir_array[lightDir_count].matViewProj1 = view_proj[1];
	lightDir_array[lightDir_count].matViewProj2 = view_proj[2];
	lightDir_array[lightDir_count].matViewProj3 = view_proj[3];
	lightDir_array[lightDir_count].ShadowmapAdress0 = Vector4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	lightDir_array[lightDir_count].ShadowmapAdress1 = Vector4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	lightDir_array[lightDir_count].ShadowmapAdress2 = Vector4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	lightDir_array[lightDir_count].ShadowmapAdress3 = Vector4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);

	lightDir_count++;

	return true;
}

void SceneRenderMgr::RegEnvProb(const EnvProbData& data)
{
	envProbMgr->AddEnvProb(data, cameraPosition);
}

void SceneRenderMgr::UpdateCamera(CameraComponent* cam)
{
	currentCamera = cam;
	cameraPosition = currentCamera->camPos;
}

void SceneRenderMgr::DrawHud()
{
	//sort(hud_array.begin(), hud_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;
	
	for(auto cur: hud_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);
		cur.material->Set(tech);
		Render::SetTopology(cur.topo);
		
		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void SceneRenderMgr::DrawOvHud()
{
	sort(ovhud_array.begin(), ovhud_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;
	
	for(auto cur: ovhud_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);
		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void SceneRenderMgr::PrepassOpaque()
{
	const unsigned int offset = 0;

	sort(opaque_array.begin(), opaque_array.end(), BaseRenderMgr::CompareMeshes);
	
	for (auto cur : opaque_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_PREPASS + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);

		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}

	// alphatest
	sort(alphatest_array.begin(), alphatest_array.end(), BaseRenderMgr::CompareMeshes);
	
	for (auto cur : alphatest_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_PREPASS + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);

		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void SceneRenderMgr::DrawOpaque()
{
	const unsigned int offset = 0;
	
	for(auto cur: opaque_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);
		
		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}

	// alphatest
	for (auto cur : alphatest_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);
		
		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void SceneRenderMgr::PrepassTransparent()
{
	sort(transparent_array.begin(), transparent_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;

	for(auto cur: transparent_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_PREPASS + cur.isSkinned);

		if(!cur.material->HasTechnique(tech))
			continue;

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);

		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void SceneRenderMgr::DrawTransparent()
{
	const unsigned int offset = 0;

	for(auto cur: transparent_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + cur.isSkinned);

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned > 0);

		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}