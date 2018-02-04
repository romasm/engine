#include "stdafx.h"
#include "RenderMgrs.h"
#include "ScenePipeline.h"
#include "CameraSystem.h"
#include "ECS\EnvProbSystem.h"
#include "Utils\Profiler.h"

using namespace EngineCore;

SceneRenderMgr::SceneRenderMgr(bool lightweight) : BaseRenderMgr()
{
	b_shadow = false;
	
	opaque_array.create(lightweight ? (OPAQUE_FRAME_MAX / 16) : OPAQUE_FRAME_MAX);
	alphatest_array.create(lightweight ? (OPAQUE_FRAME_ALPHATEST_MAX / 16) : OPAQUE_FRAME_ALPHATEST_MAX);
	transparent_array.create(lightweight ? (TRANSPARENT_FRAME_MAX / 16) : TRANSPARENT_FRAME_MAX);

	hud_array.create(lightweight ? (HUD_FRAME_MAX / 16) : HUD_FRAME_MAX);
	ovhud_array.create(lightweight ? (OV_HUD_FRAME_MAX / 16) : OV_HUD_FRAME_MAX);
	
	current_cam = nullptr;

	if(lightweight)
	{
		lightSpot_array = nullptr;
		lightSpotDisk_array = nullptr;
		lightSpotRect_array = nullptr;
		lightPoint_array = nullptr;
		lightPointSphere_array = nullptr;
		lightPointTube_array = nullptr;
		lightDir_array = nullptr;
		casterSpot_array = nullptr;
		casterSpotDisk_array = nullptr;
		casterSpotRect_array = nullptr;
		casterPoint_array = nullptr;	
		casterPointSphere_array = nullptr;	
		casterPointTube_array = nullptr;	
		shadowsRenderer = nullptr;
	}
	else
	{
		lightSpot_array = new SpotLightBuffer[LIGHT_SPOT_FRAME_MAX];
		lightSpotDisk_array = new DiskLightBuffer[LIGHT_SPOT_DISK_FRAME_MAX];
		lightSpotRect_array = new RectLightBuffer[LIGHT_SPOT_RECT_FRAME_MAX];
		lightPoint_array = new PointLightBuffer[LIGHT_POINT_FRAME_MAX];
		lightPointSphere_array = new SphereLightBuffer[LIGHT_POINT_SPHERE_FRAME_MAX];
		lightPointTube_array = new TubeLightBuffer[LIGHT_POINT_TUBE_FRAME_MAX];
		lightDir_array = new DirLightBuffer[LIGHT_DIR_FRAME_MAX];

		casterSpot_array = new SpotCasterBuffer[CASTER_SPOT_FRAME_MAX];
		casterSpotDisk_array = new DiskCasterBuffer[CASTER_SPOT_DISK_FRAME_MAX];
		casterSpotRect_array = new RectCasterBuffer[CASTER_SPOT_RECT_FRAME_MAX];
		casterPoint_array = new PointCasterBuffer[CASTER_POINT_FRAME_MAX];	
		casterPointSphere_array = new SphereCasterBuffer[CASTER_POINT_SPHERE_FRAME_MAX];	
		casterPointTube_array = new TubeCasterBuffer[CASTER_POINT_TUBE_FRAME_MAX];	

		shadowsRenderer = new ShadowsRenderer(this);
	}
	
	ClearAll();
}

SceneRenderMgr::~SceneRenderMgr()
{
	ClearAll();
	
	_DELETE_ARRAY(lightSpot_array);
	_DELETE_ARRAY(lightSpotDisk_array);
	_DELETE_ARRAY(lightSpotRect_array);
	_DELETE_ARRAY(lightPoint_array);
	_DELETE_ARRAY(lightPointSphere_array);
	_DELETE_ARRAY(lightPointTube_array);
	_DELETE_ARRAY(lightDir_array);

	_DELETE_ARRAY(casterSpot_array);
	_DELETE_ARRAY(casterSpotDisk_array);
	_DELETE_ARRAY(casterSpotRect_array);
	_DELETE_ARRAY(casterPoint_array);
	_DELETE_ARRAY(casterPointSphere_array);
	_DELETE_ARRAY(casterPointTube_array);
		
	_DELETE(shadowsRenderer);
}

void SceneRenderMgr::cleanRenderArrayLights()
{
	lightSpot_count = 0;
	lightSpotDisk_count = 0;
	lightSpotRect_count = 0;
	lightPoint_count = 0;
	lightPointSphere_count = 0;
	lightPointTube_count = 0;
	lightDir_count = 0;

	casterSpot_count = 0;
	casterSpotDisk_count = 0;
	casterSpotRect_count = 0;
	casterPoint_count = 0;
	casterPointSphere_count = 0;
	casterPointTube_count = 0;
	
	if(shadowsRenderer)
		shadowsRenderer->ClearPerFrame();
}

bool SceneRenderMgr::RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
							  uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, IA_TOPOLOGY topo)
{return RegMesh(indexCount, indexBuffer, vertexBuffer, vertexSize, isSkinned, gpuMatrixBuffer, material, Vector3(current_cam->far_clip,0,0), topo);}

bool SceneRenderMgr::RegMesh(uint32_t indexCount, ID3D11Buffer* indexBuffer, ID3D11Buffer* vertexBuffer, 
			uint32_t vertexSize, bool isSkinned, void* gpuMatrixBuffer, Material* material, Vector3& center, IA_TOPOLOGY topo)
{
	if( !material || gpuMatrixBuffer == nullptr || indexCount == 0 )
		return false;
	
	bool has_tq = false;
	const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + (isSkinned ? 1 : 0));
	auto queue = material->GetTechQueue(tech, &has_tq);
	if(!has_tq)
		return false;
	
	RenderMesh* mesh_new = nullptr;
	switch(queue)
	{
	case SC_TRANSPARENT:
	case SC_ALPHA:
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
	mesh_new->isSkinned = isSkinned;
	mesh_new->vertexSize = vertexSize;
	mesh_new->material = material;
	mesh_new->topo = topo;
	mesh_new->distanceSq = (center - cameraPosition).LengthSquared();
	
	return true;
}

#define PIXEL_HALF 0.5f

bool SceneRenderMgr::RegSpotLight(Vector4& color, float range, Vector2& cone, Vector3& pos,	Vector3& dir)
{
	if(lightSpot_count >= LIGHT_SPOT_FRAME_MAX)
		return false;
	
	lightSpot_array[lightSpot_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	lightSpot_array[lightSpot_count].ColorConeX = Vector4(color.x, color.y, color.z, cone.x);
	lightSpot_array[lightSpot_count].DirConeY = Vector4(dir.x, dir.y, dir.z, cone.y);

	lightSpot_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCaster(Vector4& color, Vector4& nonAreaColor, float range, Vector2& cone, Vector3& pos, Vector3& dir, Vector4& farNear, CXMMATRIX vp, CXMMATRIX proj, uint64_t id)
{
	if(casterSpot_count >= CASTER_SPOT_FRAME_MAX)
		return false;
	
	ShadowMap& shm = shadowsRenderer->GetShadowAdress(id);

	casterSpot_array[casterSpot_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	casterSpot_array[casterSpot_count].ColorConeX = Vector4(color.x, color.y, color.z, cone.x);
	casterSpot_array[casterSpot_count].DirConeY = Vector4(dir.x, dir.y, dir.z, cone.y);
	casterSpot_array[casterSpot_count].ShadowmapAdress = Vector4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP, 
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	casterSpot_array[casterSpot_count].ShadowmapParams = Vector4(PIXEL_HALF / shm.res, max<float>(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), farNear.x, 0);
	casterSpot_array[casterSpot_count].farNear = farNear;
	casterSpot_array[casterSpot_count].matViewProj = vp;

	casterSpot_count++;

	return true;
}

bool SceneRenderMgr::RegSpotLightDisk(Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& virtpos)
{
	if(lightSpotDisk_count >= LIGHT_SPOT_DISK_FRAME_MAX)
		return false;
	
	lightSpotDisk_array[lightSpotDisk_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	lightSpotDisk_array[lightSpotDisk_count].ColorConeX = Vector4(color.x, color.y, color.z, cone.x);
	lightSpotDisk_array[lightSpotDisk_count].DirConeY = Vector4(dir.x, dir.y, dir.z, cone.y);
	lightSpotDisk_array[lightSpotDisk_count].AreaInfoEmpty = Vector4(area.x, area.y, 0, 0);
	lightSpotDisk_array[lightSpotDisk_count].VirtposEmpty = Vector4(virtpos.x, virtpos.y, virtpos.z, 0);

	lightSpotDisk_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCasterDisk(Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& virtpos, Vector4& farNear, 
			CXMMATRIX vp, CXMMATRIX proj, uint64_t id)
{
	if(casterSpotDisk_count >= CASTER_SPOT_DISK_FRAME_MAX)
		return false;
	
	ShadowMap& shm = shadowsRenderer->GetShadowAdress(id);
	
	casterSpotDisk_array[casterSpotDisk_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	casterSpotDisk_array[casterSpotDisk_count].ColorConeX = Vector4(color.x, color.y, color.z, cone.x);
	casterSpotDisk_array[casterSpotDisk_count].DirConeY = Vector4(dir.x, dir.y, dir.z, cone.y);
	casterSpotDisk_array[casterSpotDisk_count].AreaInfoEmpty = Vector4(area.x, area.y, 0, 0);
	casterSpotDisk_array[casterSpotDisk_count].VirtposEmpty = Vector4(virtpos.x, virtpos.y, virtpos.z, 0);
	casterSpotDisk_array[casterSpotDisk_count].ShadowmapAdress = Vector4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP, 
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	casterSpotDisk_array[casterSpotDisk_count].ShadowmapParams = Vector4(PIXEL_HALF / shm.res, max<float>(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), farNear.x, 0);
	casterSpotDisk_array[casterSpotDisk_count].farNear = farNear;
	casterSpotDisk_array[casterSpotDisk_count].matViewProj = vp;
	
	casterSpotDisk_count++;

	return true;
}

bool SceneRenderMgr::RegSpotLightRect(Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos)
{
	if(lightSpotRect_count >= LIGHT_SPOT_RECT_FRAME_MAX)
		return false;
	
	lightSpotRect_array[lightSpotRect_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	lightSpotRect_array[lightSpotRect_count].ColorConeX = Vector4(color.x, color.y, color.z, cone.x);
	lightSpotRect_array[lightSpotRect_count].DirConeY = Vector4(dir.x, dir.y, dir.z, cone.y);
	lightSpotRect_array[lightSpotRect_count].DirUpAreaX = Vector4(up.x, up.y, up.z, area.x);
	lightSpotRect_array[lightSpotRect_count].DirSideAreaY = Vector4(side.x, side.y, side.z, area.y);
	lightSpotRect_array[lightSpotRect_count].VirtposAreaZ = Vector4(virtpos.x, virtpos.y, virtpos.z, area.z);

	lightSpotRect_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCasterRect(Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos, Vector4& farNear, 
			CXMMATRIX vp, CXMMATRIX proj, uint64_t id)
{
	if(casterSpotRect_count >= CASTER_SPOT_RECT_FRAME_MAX) // proj tex coords wrong calc
		return false;
	
	ShadowMap& shm = shadowsRenderer->GetShadowAdress(id);
	
	casterSpotRect_array[casterSpotRect_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	casterSpotRect_array[casterSpotRect_count].ColorConeX = Vector4(color.x, color.y, color.z, cone.x);
	casterSpotRect_array[casterSpotRect_count].DirConeY = Vector4(dir.x, dir.y, dir.z, cone.y);
	casterSpotRect_array[casterSpotRect_count].DirUpAreaX = Vector4(up.x, up.y, up.z, area.x);
	casterSpotRect_array[casterSpotRect_count].DirSideAreaY = Vector4(side.x, side.y, side.z, area.y);
	casterSpotRect_array[casterSpotRect_count].VirtposAreaZ = Vector4(virtpos.x, virtpos.y, virtpos.z, area.z);
	casterSpotRect_array[casterSpotRect_count].ShadowmapAdress = Vector4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP, 
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	casterSpotRect_array[casterSpotRect_count].ShadowmapParams = Vector4(PIXEL_HALF / shm.res, max<float>(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), farNear.x, 0);
	casterSpotRect_array[casterSpotRect_count].farNear = farNear;
	casterSpotRect_array[casterSpotRect_count].matViewProj = vp;

	casterSpotRect_count++;

	return true;
}

bool SceneRenderMgr::RegPointLight(Vector4& color, float range, Vector3& pos)
{
	if(lightPoint_count >= LIGHT_POINT_FRAME_MAX)
		return false;

	lightPoint_array[lightPoint_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	lightPoint_array[lightPoint_count].Color = Vector4(color.x, color.y, color.z, 0);

	lightPoint_count++;

	return true;
}

bool SceneRenderMgr::RegPointCaster(Vector4& color, Vector4& nonAreaColor, float range, Vector3& pos, Vector4& farNear, CXMMATRIX proj, uint64_t id)
{
	if(casterPoint_count >= CASTER_POINT_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowsRenderer->GetShadowAdress(id);
	for(uint32_t i=0; i<5; i++)
		shm[i+1] = shadowsRenderer->GetShadowAdressNext(shm[i]);

	casterPoint_array[casterPoint_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	casterPoint_array[casterPoint_count].ColorShParams = Vector4(color.x, color.y, color.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	casterPoint_array[casterPoint_count].ShadowmapParams0 = Vector4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	casterPoint_array[casterPoint_count].ShadowmapParams1 = Vector4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, 0, 0);
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
	casterPoint_array[casterPoint_count].matView = XMMatrixSet(1.0f,0,0,-pos.x, 0,1.0f,0,-pos.y, 0,0,1.0f,-pos.z, 0,0,0,1.0f);
	
	casterPoint_count++;

	return true;
}

bool SceneRenderMgr::RegPointLightSphere(Vector4& color, float range, Vector3& area, Vector3& pos)
{
	if(lightPointSphere_count >= LIGHT_POINT_SPHERE_FRAME_MAX)
		return false;

	lightPointSphere_array[lightPointSphere_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	lightPointSphere_array[lightPointSphere_count].Color = Vector4(color.x, color.y, color.z, 0);
	lightPointSphere_array[lightPointSphere_count].AreaInfo = Vector4(area.x, area.x * area.x, 0, 0);

	lightPointSphere_count++;

	return true;
}

bool SceneRenderMgr::RegPointCasterSphere(Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector3& pos, Vector4& farNear, CXMMATRIX proj, uint64_t id)
{
	if(casterPointSphere_count >= CASTER_POINT_SPHERE_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowsRenderer->GetShadowAdress(id);
	for(uint32_t i=0; i<5; i++)
		shm[i+1] = shadowsRenderer->GetShadowAdressNext(shm[i]);
	
	casterPointSphere_array[casterPointSphere_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	casterPointSphere_array[casterPointSphere_count].ColorShParams = Vector4(color.x, color.y, color.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	casterPointSphere_array[casterPointSphere_count].AreaInfo = Vector4(area.x, area.x * area.x, 0, 0);
	casterPointSphere_array[casterPointSphere_count].ShadowmapParams0 = Vector4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	casterPointSphere_array[casterPointSphere_count].ShadowmapParams1 = Vector4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, 0, 0);
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress0 = Vector4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress1 = Vector4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress2 = Vector4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress3 = Vector4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress4 = Vector4(shm[4].x * SHADOWS_BUF_RES_RCP, shm[4].y * SHADOWS_BUF_RES_RCP, 
		shm[4].res * SHADOWS_BUF_RES_RCP, (float)shm[4].dsv);
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress5 = Vector4(shm[5].x * SHADOWS_BUF_RES_RCP, shm[5].y * SHADOWS_BUF_RES_RCP, 
		shm[5].res * SHADOWS_BUF_RES_RCP, (float)shm[5].dsv);
	casterPointSphere_array[casterPointSphere_count].farNear = farNear;
	casterPointSphere_array[casterPointSphere_count].matProj = proj;
	casterPointSphere_array[casterPointSphere_count].matView = XMMatrixSet(1.0f,0,0,-pos.x, 0,1.0f,0,-pos.y, 0,0,1.0f,-pos.z, 0,0,0,1.0f);
	
	casterPointSphere_count++;

	return true;
}
		
bool SceneRenderMgr::RegPointLightTube(Vector4& color, float range, Vector3& area, Vector3& pos, Vector3& dir)
{
	if(lightPointTube_count >= LIGHT_POINT_TUBE_FRAME_MAX)
		return false;

	lightPointTube_array[lightPointTube_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	lightPointTube_array[lightPointTube_count].Color = Vector4(color.x, color.y, color.z, 0);
	lightPointTube_array[lightPointTube_count].AreaInfo = Vector4(area.x, area.y, area.z, area.y * area.y);
	lightPointTube_array[lightPointTube_count].DirAreaA = Vector4(dir.x, dir.y, dir.z, area.y + 2 * area.x);

	lightPointTube_count++;

	return true;
}

bool SceneRenderMgr::RegPointCasterTube(Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector3& pos, Vector3& dir, Vector4& farNear, CXMMATRIX proj, CXMMATRIX view, uint64_t id)
{
	if(casterPointTube_count >= CASTER_POINT_TUBE_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowsRenderer->GetShadowAdress(id);
	for(uint32_t i=0; i<5; i++)
		shm[i+1] = shadowsRenderer->GetShadowAdressNext(shm[i]);
	
	casterPointTube_array[casterPointTube_count].PosRange = Vector4(pos.x, pos.y, pos.z, range);
	casterPointTube_array[casterPointTube_count].ColorShParams = Vector4(color.x, color.y, color.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	casterPointTube_array[casterPointTube_count].AreaInfo = Vector4(area.x, area.y, area.z, area.y * area.y);
	casterPointTube_array[casterPointTube_count].DirAreaA = Vector4(dir.x, dir.y, dir.z, area.y + 2 * area.x);
	casterPointTube_array[casterPointTube_count].ShadowmapParams0 = Vector4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	casterPointTube_array[casterPointTube_count].ShadowmapParams1 = Vector4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, 0, 0);
	casterPointTube_array[casterPointTube_count].ShadowmapAdress0 = Vector4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	casterPointTube_array[casterPointTube_count].ShadowmapAdress1 = Vector4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	casterPointTube_array[casterPointTube_count].ShadowmapAdress2 = Vector4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	casterPointTube_array[casterPointTube_count].ShadowmapAdress3 = Vector4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	casterPointTube_array[casterPointTube_count].ShadowmapAdress4 = Vector4(shm[4].x * SHADOWS_BUF_RES_RCP, shm[4].y * SHADOWS_BUF_RES_RCP, 
		shm[4].res * SHADOWS_BUF_RES_RCP, (float)shm[4].dsv);
	casterPointTube_array[casterPointTube_count].ShadowmapAdress5 = Vector4(shm[5].x * SHADOWS_BUF_RES_RCP, shm[5].y * SHADOWS_BUF_RES_RCP, 
		shm[5].res * SHADOWS_BUF_RES_RCP, (float)shm[5].dsv);
	casterPointTube_array[casterPointTube_count].farNear = farNear;
	casterPointTube_array[casterPointTube_count].matProj = proj;
	casterPointTube_array[casterPointTube_count].matView = view;
	
	casterPointTube_count++;

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

bool SceneRenderMgr::RegEnvProb(const EnvProbData& data)
{
	envProbMgr->AddEnvProb(data, cameraPosition);
}

void SceneRenderMgr::UpdateCamera(CameraComponent* cam)
{
	current_cam = cam;
	cameraPosition = current_cam->camPos;
}

void SceneRenderMgr::DrawHud()
{
	//sort(hud_array.begin(), hud_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;
	
	for(auto cur: hud_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + (cur.isSkinned ? 1 : 0));

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned);
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
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + (cur.isSkinned ? 1 : 0));

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned);
		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void SceneRenderMgr::DrawOpaque(ScenePipeline* scene)
{
	sort(opaque_array.begin(), opaque_array.end(), BaseRenderMgr::CompareMeshes );

	const unsigned int offset = 0;
	
	for(auto cur: opaque_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + (cur.isSkinned ? 1 : 0));

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned);

		if(scene->Materials_Count < MATERIALS_COUNT)
			cur.material->AddToFrameBuffer(&scene->Materials[scene->Materials_Count], &scene->Materials_Count);

		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void SceneRenderMgr::DrawAlphatest(ScenePipeline* scene)
{
	sort(alphatest_array.begin(), alphatest_array.end(), BaseRenderMgr::CompareMeshes );

	const unsigned int offset = 0;

	for(auto cur: alphatest_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + (cur.isSkinned ? 1 : 0));

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned);
		
		if(scene->Materials_Count < MATERIALS_COUNT)
			cur.material->AddToFrameBuffer(&scene->Materials[scene->Materials_Count], &scene->Materials_Count);

		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void SceneRenderMgr::PrepassTransparent(ScenePipeline* scene)
{
	sort(transparent_array.begin(), transparent_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;

	for(auto cur: transparent_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_PREPASS + (cur.isSkinned ? 1 : 0));

		if(!cur.material->HasTechnique(tech))
			continue;

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned);

		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}

void SceneRenderMgr::DrawTransparent(ScenePipeline* scene)
{
	const unsigned int offset = 0;

	for(auto cur: transparent_array)
	{
		const TECHNIQUES tech = TECHNIQUES(TECHNIQUES::TECHNIQUE_DEFAULT + (cur.isSkinned ? 1 : 0));

		Render::Context()->IASetVertexBuffers(0, 1, &(cur.vertexBuffer), &(cur.vertexSize), &offset);
		Render::Context()->IASetIndexBuffer(cur.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		cur.material->SetMatrixBuffer(cur.gpuMatrixBuffer, cur.isSkinned);

		cur.material->Set(tech);
		Render::SetTopology(cur.topo);

		Render::Context()->DrawIndexed(cur.indexCount, 0, 0);
	}
}