#include "stdafx.h"
#include "RenderMgrs.h"
#include "ScenePipeline.h"
#include "CameraSystem.h"
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
		voxelRenderer = nullptr;
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
		voxelRenderer = new VoxelRenderer(this);
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
	_DELETE(voxelRenderer);
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
	if(voxelRenderer)
		voxelRenderer->ClearPerFrame();
}

bool SceneRenderMgr::RegMesh(uint32_t index_count, ID3D11Buffer* vertex_buffer, ID3D11Buffer* index_buffer, 
							ID3D11Buffer* constant_buffer, uint32_t vertex_size, Material* material, IA_TOPOLOGY topo)
{const float far_clip = EngineSettings::EngSets.cam_far_clip;
			return regToDraw(index_count, vertex_buffer, index_buffer, constant_buffer, vertex_size, material, XMVectorSet(current_cam->far_clip,0,0,0), topo);}
bool SceneRenderMgr::RegMesh(uint32_t index_count, ID3D11Buffer* vertex_buffer, ID3D11Buffer* index_buffer, 
							 ID3D11Buffer* constant_buffer, uint32_t vertex_size, Material* material, XMVECTOR center, IA_TOPOLOGY topo)
{return regToDraw(index_count, vertex_buffer, index_buffer, constant_buffer, vertex_size, material, center - cameraPosition, topo);}

bool SceneRenderMgr::RegMultiMesh(uint32_t* index_count, ID3D11Buffer** vertex_buffer, 
			ID3D11Buffer** index_buffer, ID3D11Buffer* constant_buffer, uint32_t vertex_size, RArray<Material*>& material, IA_TOPOLOGY topo)
{const float far_clip = EngineSettings::EngSets.cam_far_clip;
			return regToDraw(index_count, vertex_buffer, index_buffer, constant_buffer, vertex_size, material, XMVectorSet(current_cam->far_clip,0,0,0), topo);}
bool SceneRenderMgr::RegMultiMesh(uint32_t* index_count, ID3D11Buffer** vertex_buffer, 
			ID3D11Buffer** index_buffer, ID3D11Buffer* constant_buffer, uint32_t vertex_size, RArray<Material*>& material, XMVECTOR center, IA_TOPOLOGY topo)
{return regToDraw(index_count, vertex_buffer, index_buffer, constant_buffer, vertex_size, material, center - cameraPosition, topo);}

bool SceneRenderMgr::regToDraw(uint32_t index_count, 
			ID3D11Buffer* vertex_buffer, ID3D11Buffer* index_buffer, ID3D11Buffer* constant_buffer,
			uint32_t vertex_size, Material* material, XMVECTOR center, IA_TOPOLOGY topo)
{
	bool has_tq = false;
	auto queue = material->GetTechQueue(TECHNIQUES::TECHNIQUE_DEFAULT, &has_tq);

	if(!has_tq)
		return false;

	RenderMesh* mesh_new = new RenderMesh;
	mesh_new->index_count = index_count;
	mesh_new->vertex_buffer = vertex_buffer;
	mesh_new->index_buffer = index_buffer;
	mesh_new->constant_buffer = constant_buffer;
	mesh_new->vertex_size = vertex_size;
	mesh_new->material = material;
	mesh_new->topo = topo;

	MeshGroup<RenderMesh>* group_new = new MeshGroup<RenderMesh>();
	group_new->ID = meshgroup_count;
	meshgroup_count++;
	group_new->meshes = new RenderMesh*[1];
	group_new->mesh_count = 1;
	group_new->meshes[0] = mesh_new;
	group_new->center = center;
	mesh_new->group = group_new;

	switch(queue)
	{
	case SC_TRANSPARENT:
	case SC_ALPHA:
		transparent_array.push_back(mesh_new);
		break;
	case SC_OPAQUE:
		opaque_array.push_back(mesh_new);
		break;
	case SC_ALPHATEST:
		alphatest_array.push_back(mesh_new);
		break;
	case GUI_3D:
		hud_array.push_back(mesh_new);
		break;
	case GUI_3D_OVERLAY:
		ovhud_array.push_back(mesh_new);
		break;
	}

	return true;
}

bool SceneRenderMgr::regToDraw(uint32_t* index_count, 
			ID3D11Buffer** vertex_buffer, ID3D11Buffer** index_buffer, ID3D11Buffer* constant_buffer,
			uint32_t vertex_size, RArray<Material*>& material, XMVECTOR center, IA_TOPOLOGY topo)
{
	MeshGroup<RenderMesh>* group_new = new MeshGroup<RenderMesh>();
	group_new->ID = meshgroup_count;
	meshgroup_count++;
	group_new->meshes = new RenderMesh*[material.size()];
	group_new->center = center;
	group_new->mesh_count = (uint)material.size();

	uint reged = 0;
	for(uint16_t i=0; i<material.size(); i++)
	{
		bool has_tq = false;
		auto queue = material[i]->GetTechQueue(TECHNIQUES::TECHNIQUE_DEFAULT, &has_tq);

		if(!has_tq)
			return false;

		RenderMesh* mesh_new = new RenderMesh;
		mesh_new->index_count = index_count[i];
		mesh_new->vertex_buffer = vertex_buffer[i];
		mesh_new->index_buffer = index_buffer[i];
		mesh_new->constant_buffer = constant_buffer;
		mesh_new->vertex_size = vertex_size;
		mesh_new->material = material[i];
		mesh_new->topo = topo;

		group_new->meshes[i] = mesh_new;
		mesh_new->group = group_new;

		reged++;
		switch(queue)
		{
		case SC_TRANSPARENT:
		case SC_ALPHA:
			transparent_array.push_back(mesh_new);
			break;
		case SC_OPAQUE:
			opaque_array.push_back(mesh_new);
			break;
		case SC_ALPHATEST:
			alphatest_array.push_back(mesh_new);
			break;
		case GUI_3D:
			hud_array.push_back(mesh_new);
			break;
		case GUI_3D_OVERLAY:
			ovhud_array.push_back(mesh_new);
			break;
		}
	}

	if(!reged)
	{
		_DELETE(group_new);
		meshgroup_count--;
	}

	return true;
}


#define PIXEL_HALF 0.5f

bool SceneRenderMgr::RegSpotLight(XMFLOAT4& color, float range, XMFLOAT2& cone, XMFLOAT3& pos,	XMFLOAT3& dir)
{
	if(lightSpot_count >= LIGHT_SPOT_FRAME_MAX)
		return false;
	
	lightSpot_array[lightSpot_count].PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightSpot_array[lightSpot_count].ColorConeX = XMFLOAT4(color.x, color.y, color.z, cone.x);
	lightSpot_array[lightSpot_count].DirConeY = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);

	lightSpot_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCaster(XMFLOAT4& color, XMFLOAT4& nonAreaColor, float range, XMFLOAT2& cone, XMFLOAT3& pos, XMFLOAT3& dir, XMFLOAT4& farNear, CXMMATRIX vp, CXMMATRIX proj, uint64_t id)
{
	if(casterSpot_count >= CASTER_SPOT_FRAME_MAX)
		return false;
	
	ShadowMap& shm = shadowsRenderer->GetShadowAdress(id);

	// to voxels
	SpotVoxelBuffer vData;
	vData.PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData.ColorConeX = XMFLOAT4(nonAreaColor.x, nonAreaColor.y, nonAreaColor.z, cone.x);
	vData.DirConeY = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	vData.Virtpos = vData.PosRange;
	vData.ShadowmapAdress =  XMFLOAT4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP, 
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	vData.ShadowmapHPixProjNearclip = XMFLOAT4(PIXEL_HALF / shm.res, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), farNear.x, 0);
	vData.matViewProj = vp;

	voxelRenderer->RegisterSpotCaster(vData);

	// to deffered
	casterSpot_array[casterSpot_count].PosRange = vData.PosRange;
	casterSpot_array[casterSpot_count].ColorConeX = XMFLOAT4(color.x, color.y, color.z, vData.ColorConeX.w);
	casterSpot_array[casterSpot_count].DirConeY = vData.DirConeY;
	casterSpot_array[casterSpot_count].ShadowmapAdress = vData.ShadowmapAdress;
	casterSpot_array[casterSpot_count].ShadowmapParams = vData.ShadowmapHPixProjNearclip;
	casterSpot_array[casterSpot_count].farNear = farNear;
	casterSpot_array[casterSpot_count].matViewProj = vp;

	casterSpot_count++;

	return true;
}

bool SceneRenderMgr::RegSpotLightDisk(XMFLOAT4& color, float range, XMFLOAT3& area, XMFLOAT2& cone, XMFLOAT3& pos, XMFLOAT3& dir, XMFLOAT3& virtpos)
{
	if(lightSpotDisk_count >= LIGHT_SPOT_DISK_FRAME_MAX)
		return false;
	
	lightSpotDisk_array[lightSpotDisk_count].PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightSpotDisk_array[lightSpotDisk_count].ColorConeX = XMFLOAT4(color.x, color.y, color.z, cone.x);
	lightSpotDisk_array[lightSpotDisk_count].DirConeY = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	lightSpotDisk_array[lightSpotDisk_count].AreaInfoEmpty = XMFLOAT4(area.x, area.y, 0, 0);
	lightSpotDisk_array[lightSpotDisk_count].VirtposEmpty = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, 0);

	lightSpotDisk_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCasterDisk(XMFLOAT4& color, XMFLOAT4& nonAreaColor, float range, XMFLOAT3& area, XMFLOAT2& cone, XMFLOAT3& pos, XMFLOAT3& dir, XMFLOAT3& virtpos, XMFLOAT4& farNear, 
			CXMMATRIX vp, CXMMATRIX proj, uint64_t id)
{
	if(casterSpotDisk_count >= CASTER_SPOT_DISK_FRAME_MAX)
		return false;
	
	ShadowMap& shm = shadowsRenderer->GetShadowAdress(id);
	
	// to voxels
	SpotVoxelBuffer vData;
	vData.PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData.ColorConeX = XMFLOAT4(nonAreaColor.x, nonAreaColor.y, nonAreaColor.z, cone.x);
	vData.DirConeY = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	vData.Virtpos = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, 0);
	vData.ShadowmapAdress =  XMFLOAT4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP, 
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	vData.ShadowmapHPixProjNearclip = XMFLOAT4(PIXEL_HALF / shm.res, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), farNear.x, 0);
	vData.matViewProj = vp;

	voxelRenderer->RegisterSpotCaster(vData);

	// to deffered
	casterSpotDisk_array[casterSpotDisk_count].PosRange = vData.PosRange;
	casterSpotDisk_array[casterSpotDisk_count].ColorConeX = XMFLOAT4(color.x, color.y, color.z, vData.ColorConeX.w);
	casterSpotDisk_array[casterSpotDisk_count].DirConeY = vData.DirConeY;
	casterSpotDisk_array[casterSpotDisk_count].AreaInfoEmpty = XMFLOAT4(area.x, area.y, 0, 0);
	casterSpotDisk_array[casterSpotDisk_count].VirtposEmpty = vData.Virtpos;
	casterSpotDisk_array[casterSpotDisk_count].ShadowmapAdress = vData.ShadowmapAdress;
	casterSpotDisk_array[casterSpotDisk_count].ShadowmapParams = vData.ShadowmapHPixProjNearclip;
	casterSpotDisk_array[casterSpotDisk_count].farNear = farNear;
	casterSpotDisk_array[casterSpotDisk_count].matViewProj = vp;
	
	casterSpotDisk_count++;

	return true;
}

bool SceneRenderMgr::RegSpotLightRect(XMFLOAT4& color, float range, XMFLOAT3& area, XMFLOAT2& cone, XMFLOAT3& pos, XMFLOAT3& dir, XMFLOAT3& up, XMFLOAT3& side, XMFLOAT3& virtpos)
{
	if(lightSpotRect_count >= LIGHT_SPOT_RECT_FRAME_MAX)
		return false;
	
	lightSpotRect_array[lightSpotRect_count].PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightSpotRect_array[lightSpotRect_count].ColorConeX = XMFLOAT4(color.x, color.y, color.z, cone.x);
	lightSpotRect_array[lightSpotRect_count].DirConeY = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	lightSpotRect_array[lightSpotRect_count].DirUpAreaX = XMFLOAT4(up.x, up.y, up.z, area.x);
	lightSpotRect_array[lightSpotRect_count].DirSideAreaY = XMFLOAT4(side.x, side.y, side.z, area.y);
	lightSpotRect_array[lightSpotRect_count].VirtposAreaZ = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, area.z);

	lightSpotRect_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCasterRect(XMFLOAT4& color, XMFLOAT4& nonAreaColor, float range, XMFLOAT3& area, XMFLOAT2& cone, XMFLOAT3& pos, XMFLOAT3& dir, XMFLOAT3& up, XMFLOAT3& side, XMFLOAT3& virtpos, XMFLOAT4& farNear, 
			CXMMATRIX vp, CXMMATRIX proj, uint64_t id)
{
	if(casterSpotRect_count >= CASTER_SPOT_RECT_FRAME_MAX) // proj tex coords wrong calc
		return false;
	
	ShadowMap& shm = shadowsRenderer->GetShadowAdress(id);
	
	// to voxels
	SpotVoxelBuffer vData;
	vData.PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData.ColorConeX = XMFLOAT4(nonAreaColor.x, nonAreaColor.y, nonAreaColor.z, cone.x);
	vData.DirConeY = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	vData.Virtpos = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, 0);
	vData.ShadowmapAdress =  XMFLOAT4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP, 
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	vData.ShadowmapHPixProjNearclip = XMFLOAT4(PIXEL_HALF / shm.res, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), farNear.x, 0);
	vData.matViewProj = vp;
	
	voxelRenderer->RegisterSpotCaster(vData);

	// to deffered
	casterSpotRect_array[casterSpotRect_count].PosRange = vData.PosRange;
	casterSpotRect_array[casterSpotRect_count].ColorConeX = XMFLOAT4(color.x, color.y, color.z, vData.ColorConeX.w);
	casterSpotRect_array[casterSpotRect_count].DirConeY = vData.DirConeY;
	casterSpotRect_array[casterSpotRect_count].DirUpAreaX = XMFLOAT4(up.x, up.y, up.z, area.x);
	casterSpotRect_array[casterSpotRect_count].DirSideAreaY = XMFLOAT4(side.x, side.y, side.z, area.y);
	casterSpotRect_array[casterSpotRect_count].VirtposAreaZ = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, area.z);
	casterSpotRect_array[casterSpotRect_count].ShadowmapAdress = vData.ShadowmapAdress;
	casterSpotRect_array[casterSpotRect_count].ShadowmapParams = vData.ShadowmapHPixProjNearclip;
	casterSpotRect_array[casterSpotRect_count].farNear = farNear;
	casterSpotRect_array[casterSpotRect_count].matViewProj = vp;

	casterSpotRect_count++;

	return true;
}

bool SceneRenderMgr::RegPointLight(XMFLOAT4& color, float range, XMFLOAT3& pos)
{
	if(lightPoint_count >= LIGHT_POINT_FRAME_MAX)
		return false;

	lightPoint_array[lightPoint_count].PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightPoint_array[lightPoint_count].Color = XMFLOAT4(color.x, color.y, color.z, 0);

	lightPoint_count++;

	return true;
}

bool SceneRenderMgr::RegPointCaster(XMFLOAT4& color, XMFLOAT4& nonAreaColor, float range, XMFLOAT3& pos, XMFLOAT4& farNear, CXMMATRIX proj, uint64_t id)
{
	if(casterPoint_count >= CASTER_POINT_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowsRenderer->GetShadowAdress(id);
	for(uint i=0; i<5; i++)
		shm[i+1] = shadowsRenderer->GetShadowAdressNext(shm[i]);

	// to voxels
	PointVoxelBuffer vData;
	vData.PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData.ColorShadowmapProj = XMFLOAT4(nonAreaColor.x, nonAreaColor.y, nonAreaColor.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	vData.ShadowmapAdress0 = XMFLOAT4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	vData.ShadowmapAdress1 = XMFLOAT4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	vData.ShadowmapAdress2 = XMFLOAT4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	vData.ShadowmapAdress3 = XMFLOAT4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	vData.ShadowmapAdress4 = XMFLOAT4(shm[4].x * SHADOWS_BUF_RES_RCP, shm[4].y * SHADOWS_BUF_RES_RCP, 
		shm[4].res * SHADOWS_BUF_RES_RCP, (float)shm[4].dsv);
	vData.ShadowmapAdress5 = XMFLOAT4(shm[5].x * SHADOWS_BUF_RES_RCP, shm[5].y * SHADOWS_BUF_RES_RCP, 
		shm[5].res * SHADOWS_BUF_RES_RCP, (float)shm[5].dsv);
	vData.ShadowmapHPix0 = XMFLOAT4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	vData.ShadowmapHPix1 = XMFLOAT4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, 0, 0);
	vData.matProj = proj;
	vData.matView = XMMatrixSet(1.0f,0,0,-pos.x, 0,1.0f,0,-pos.y, 0,0,1.0f,-pos.z, 0,0,0,1.0f);

	voxelRenderer->RegisterPointCaster(vData);

	// to deffered
	casterPoint_array[casterPoint_count].PosRange = vData.PosRange;
	casterPoint_array[casterPoint_count].ColorShParams = XMFLOAT4(color.x, color.y, color.z, vData.ColorShadowmapProj.w);
	casterPoint_array[casterPoint_count].ShadowmapParams0 = vData.ShadowmapHPix0;
	casterPoint_array[casterPoint_count].ShadowmapParams1 = vData.ShadowmapHPix1;
	casterPoint_array[casterPoint_count].ShadowmapAdress0 = vData.ShadowmapAdress0;
	casterPoint_array[casterPoint_count].ShadowmapAdress1 = vData.ShadowmapAdress1;
	casterPoint_array[casterPoint_count].ShadowmapAdress2 = vData.ShadowmapAdress2;
	casterPoint_array[casterPoint_count].ShadowmapAdress3 = vData.ShadowmapAdress3;
	casterPoint_array[casterPoint_count].ShadowmapAdress4 = vData.ShadowmapAdress4;
	casterPoint_array[casterPoint_count].ShadowmapAdress5 = vData.ShadowmapAdress5;
	casterPoint_array[casterPoint_count].farNear = farNear;
	casterPoint_array[casterPoint_count].matProj = vData.matProj;
	casterPoint_array[casterPoint_count].matView = vData.matView;
	
	casterPoint_count++;

	return true;
}

bool SceneRenderMgr::RegPointLightSphere(XMFLOAT4& color, float range, XMFLOAT3& area, XMFLOAT3& pos)
{
	if(lightPointSphere_count >= LIGHT_POINT_SPHERE_FRAME_MAX)
		return false;

	lightPointSphere_array[lightPointSphere_count].PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightPointSphere_array[lightPointSphere_count].Color = XMFLOAT4(color.x, color.y, color.z, 0);
	lightPointSphere_array[lightPointSphere_count].AreaInfo = XMFLOAT4(area.x, area.x * area.x, 0, 0);

	lightPointSphere_count++;

	return true;
}

bool SceneRenderMgr::RegPointCasterSphere(XMFLOAT4& color, XMFLOAT4& nonAreaColor, float range, XMFLOAT3& area, XMFLOAT3& pos, XMFLOAT4& farNear, CXMMATRIX proj, uint64_t id)
{
	if(casterPointSphere_count >= CASTER_POINT_SPHERE_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowsRenderer->GetShadowAdress(id);
	for(uint i=0; i<5; i++)
		shm[i+1] = shadowsRenderer->GetShadowAdressNext(shm[i]);
	
	// to voxels
	PointVoxelBuffer vData;
	vData.PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData.ColorShadowmapProj = XMFLOAT4(nonAreaColor.x, nonAreaColor.y, nonAreaColor.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	vData.ShadowmapAdress0 = XMFLOAT4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	vData.ShadowmapAdress1 = XMFLOAT4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	vData.ShadowmapAdress2 = XMFLOAT4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	vData.ShadowmapAdress3 = XMFLOAT4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	vData.ShadowmapAdress4 = XMFLOAT4(shm[4].x * SHADOWS_BUF_RES_RCP, shm[4].y * SHADOWS_BUF_RES_RCP, 
		shm[4].res * SHADOWS_BUF_RES_RCP, (float)shm[4].dsv);
	vData.ShadowmapAdress5 = XMFLOAT4(shm[5].x * SHADOWS_BUF_RES_RCP, shm[5].y * SHADOWS_BUF_RES_RCP, 
		shm[5].res * SHADOWS_BUF_RES_RCP, (float)shm[5].dsv);
	vData.ShadowmapHPix0 = XMFLOAT4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	vData.ShadowmapHPix1 = XMFLOAT4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, 0, 0);
	vData.matProj = proj;
	vData.matView = XMMatrixSet(1.0f,0,0,-pos.x, 0,1.0f,0,-pos.y, 0,0,1.0f,-pos.z, 0,0,0,1.0f);

	voxelRenderer->RegisterPointCaster(vData);

	// to deffered
	casterPointSphere_array[casterPointSphere_count].PosRange = vData.PosRange;
	casterPointSphere_array[casterPointSphere_count].ColorShParams = XMFLOAT4(color.x, color.y, color.z, vData.ColorShadowmapProj.w);
	casterPointSphere_array[casterPointSphere_count].AreaInfo = XMFLOAT4(area.x, area.x * area.x, 0, 0);
	casterPointSphere_array[casterPointSphere_count].ShadowmapParams0 = vData.ShadowmapHPix0;
	casterPointSphere_array[casterPointSphere_count].ShadowmapParams1 = vData.ShadowmapHPix1;
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress0 = vData.ShadowmapAdress0;
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress1 = vData.ShadowmapAdress1;
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress2 = vData.ShadowmapAdress2;
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress3 = vData.ShadowmapAdress3;
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress4 = vData.ShadowmapAdress4;
	casterPointSphere_array[casterPointSphere_count].ShadowmapAdress5 = vData.ShadowmapAdress5;
	casterPointSphere_array[casterPointSphere_count].farNear = farNear;
	casterPointSphere_array[casterPointSphere_count].matProj = vData.matProj;
	casterPointSphere_array[casterPointSphere_count].matView = vData.matView;
	
	casterPointSphere_count++;

	return true;
}
		
bool SceneRenderMgr::RegPointLightTube(XMFLOAT4& color, float range, XMFLOAT3& area, XMFLOAT3& pos, XMFLOAT3& dir)
{
	if(lightPointTube_count >= LIGHT_POINT_TUBE_FRAME_MAX)
		return false;

	lightPointTube_array[lightPointTube_count].PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightPointTube_array[lightPointTube_count].Color = XMFLOAT4(color.x, color.y, color.z, 0);
	lightPointTube_array[lightPointTube_count].AreaInfo = XMFLOAT4(area.x, area.y, area.z, area.y * area.y);
	lightPointTube_array[lightPointTube_count].DirAreaA = XMFLOAT4(dir.x, dir.y, dir.z, area.y + 2 * area.x);

	lightPointTube_count++;

	return true;
}

bool SceneRenderMgr::RegPointCasterTube(XMFLOAT4& color, XMFLOAT4& nonAreaColor, float range, XMFLOAT3& area, XMFLOAT3& pos, XMFLOAT3& dir, XMFLOAT4& farNear, CXMMATRIX proj, CXMMATRIX view, uint64_t id)
{
	if(casterPointTube_count >= CASTER_POINT_TUBE_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowsRenderer->GetShadowAdress(id);
	for(uint i=0; i<5; i++)
		shm[i+1] = shadowsRenderer->GetShadowAdressNext(shm[i]);
	
	// to voxels
	PointVoxelBuffer vData;
	vData.PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData.ColorShadowmapProj = XMFLOAT4(nonAreaColor.x, nonAreaColor.y, nonAreaColor.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	vData.ShadowmapAdress0 = XMFLOAT4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	vData.ShadowmapAdress1 = XMFLOAT4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	vData.ShadowmapAdress2 = XMFLOAT4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	vData.ShadowmapAdress3 = XMFLOAT4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	vData.ShadowmapAdress4 = XMFLOAT4(shm[4].x * SHADOWS_BUF_RES_RCP, shm[4].y * SHADOWS_BUF_RES_RCP, 
		shm[4].res * SHADOWS_BUF_RES_RCP, (float)shm[4].dsv);
	vData.ShadowmapAdress5 = XMFLOAT4(shm[5].x * SHADOWS_BUF_RES_RCP, shm[5].y * SHADOWS_BUF_RES_RCP, 
		shm[5].res * SHADOWS_BUF_RES_RCP, (float)shm[5].dsv);
	vData.ShadowmapHPix0 = XMFLOAT4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	vData.ShadowmapHPix1 = XMFLOAT4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, 0, 0);
	vData.matProj = proj;
	vData.matView = view;

	voxelRenderer->RegisterPointCaster(vData);

	// to deffered
	casterPointTube_array[casterPointTube_count].PosRange = vData.PosRange;
	casterPointTube_array[casterPointTube_count].ColorShParams = XMFLOAT4(color.x, color.y, color.z, vData.ColorShadowmapProj.w);
	casterPointTube_array[casterPointTube_count].AreaInfo = XMFLOAT4(area.x, area.y, area.z, area.y * area.y);
	casterPointTube_array[casterPointTube_count].DirAreaA = XMFLOAT4(dir.x, dir.y, dir.z, area.y + 2 * area.x);
	casterPointTube_array[casterPointTube_count].ShadowmapParams0 = vData.ShadowmapHPix0;
	casterPointTube_array[casterPointTube_count].ShadowmapParams1 = vData.ShadowmapHPix1;
	casterPointTube_array[casterPointTube_count].ShadowmapAdress0 = vData.ShadowmapAdress0;
	casterPointTube_array[casterPointTube_count].ShadowmapAdress1 = vData.ShadowmapAdress1;
	casterPointTube_array[casterPointTube_count].ShadowmapAdress2 = vData.ShadowmapAdress2;
	casterPointTube_array[casterPointTube_count].ShadowmapAdress3 = vData.ShadowmapAdress3;
	casterPointTube_array[casterPointTube_count].ShadowmapAdress4 = vData.ShadowmapAdress4;
	casterPointTube_array[casterPointTube_count].ShadowmapAdress5 = vData.ShadowmapAdress5;
	casterPointTube_array[casterPointTube_count].farNear = farNear;
	casterPointTube_array[casterPointTube_count].matProj = vData.matProj;
	casterPointTube_array[casterPointTube_count].matView = vData.matView;
	
	casterPointTube_count++;

	return true;
}

bool SceneRenderMgr::RegDirLight(XMFLOAT4& color, XMFLOAT2& area, XMFLOAT3& dir, XMMATRIX* view_proj, XMFLOAT3* pos, uint64_t id)
{
	if(lightDir_count >= LIGHT_DIR_FRAME_MAX)
		return false;

	ShadowMap shm[LIGHT_DIR_NUM_CASCADES];
	shm[0] = shadowsRenderer->GetShadowAdress(id);
	for(uint8_t i=0; i<LIGHT_DIR_NUM_CASCADES-1; i++)
		shm[i+1] = shadowsRenderer->GetShadowAdressNext(shm[i]);

	// to voxel
	DirVoxelBuffer vData;
	vData.Color = XMFLOAT4(color.x, color.y, color.z, area.x);
	vData.Dir = XMFLOAT4(dir.x, dir.y, dir.z, area.y);
	vData.PosHPix0 = XMFLOAT4(pos[0].x, pos[0].y, pos[0].z, PIXEL_HALF / shm[0].res);
	vData.PosHPix1 = XMFLOAT4(pos[1].x, pos[1].y, pos[1].z, PIXEL_HALF / shm[1].res);
	vData.PosHPix2 = XMFLOAT4(pos[2].x, pos[2].y, pos[2].z, PIXEL_HALF / shm[2].res);
	vData.PosHPix3 = XMFLOAT4(pos[3].x, pos[3].y, pos[3].z, PIXEL_HALF / shm[3].res);
	vData.ShadowmapAdress0 = XMFLOAT4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	vData.ShadowmapAdress1 = XMFLOAT4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	vData.ShadowmapAdress2 = XMFLOAT4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	vData.ShadowmapAdress3 = XMFLOAT4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	vData.ViewProj0 = view_proj[0];
	vData.ViewProj1 = view_proj[1];
	vData.ViewProj2 = view_proj[2];
	vData.ViewProj3 = view_proj[3];

	voxelRenderer->RegisterDirCaster(vData);

	// to deffered
	lightDir_array[lightDir_count].ColorAreaX = vData.Color;
	lightDir_array[lightDir_count].DirAreaY = vData.Dir;

	// locked for 4 cascades
	lightDir_array[lightDir_count].Pos0 = vData.PosHPix0;
	lightDir_array[lightDir_count].Pos1 = vData.PosHPix1;
	lightDir_array[lightDir_count].Pos2 = vData.PosHPix2;
	lightDir_array[lightDir_count].Pos3 = vData.PosHPix3;
	lightDir_array[lightDir_count].matViewProj0 = view_proj[0];
	lightDir_array[lightDir_count].matViewProj1 = view_proj[1];
	lightDir_array[lightDir_count].matViewProj2 = view_proj[2];
	lightDir_array[lightDir_count].matViewProj3 = view_proj[3];
	lightDir_array[lightDir_count].ShadowmapAdress0 = vData.ShadowmapAdress0;
	lightDir_array[lightDir_count].ShadowmapAdress1 = vData.ShadowmapAdress1;
	lightDir_array[lightDir_count].ShadowmapAdress2 = vData.ShadowmapAdress2;
	lightDir_array[lightDir_count].ShadowmapAdress3 = vData.ShadowmapAdress3;

	lightDir_count++;

	return true;
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
		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);
		cur->material->Set(TECHNIQUES::TECHNIQUE_DEFAULT);
		Render::SetTopology(cur->topo);
		
		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}
}

void SceneRenderMgr::DrawOvHud()
{
	sort(ovhud_array.begin(), ovhud_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;
	
	for(auto cur: ovhud_array)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);
		cur->material->Set(TECHNIQUES::TECHNIQUE_DEFAULT);
		Render::SetTopology(cur->topo);

		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}
}

void SceneRenderMgr::DrawOpaque(ScenePipeline* scene)
{
	sort(opaque_array.begin(), opaque_array.end(), BaseRenderMgr::CompareMeshes );

	const unsigned int offset = 0;
	
	for(auto cur: opaque_array)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);

		if(scene->Materials_Count < MATERIALS_COUNT)
			cur->material->AddToFrameBuffer(&scene->Materials[scene->Materials_Count], &scene->Materials_Count);

		cur->material->Set(TECHNIQUES::TECHNIQUE_DEFAULT);
		Render::SetTopology(cur->topo);

		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}
}

void SceneRenderMgr::DrawAlphatest(ScenePipeline* scene)
{
	sort(alphatest_array.begin(), alphatest_array.end(), BaseRenderMgr::CompareMeshes );

	const unsigned int offset = 0;

	for(auto cur: alphatest_array)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);
		
		if(scene->Materials_Count < MATERIALS_COUNT)
			cur->material->AddToFrameBuffer(&scene->Materials[scene->Materials_Count], &scene->Materials_Count);

		cur->material->Set(TECHNIQUES::TECHNIQUE_DEFAULT);
		Render::SetTopology(cur->topo);

		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}
}

void SceneRenderMgr::PrepassTransparent(ScenePipeline* scene)
{
	sort(transparent_array.begin(), transparent_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;

	for(auto cur: transparent_array)
	{
		if(!cur->material->HasTechnique(TECHNIQUES::TECHNIQUE_PREPASS))
			continue;

		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);

		cur->material->Set(TECHNIQUES::TECHNIQUE_PREPASS);
		Render::SetTopology(cur->topo);

		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}
}

void SceneRenderMgr::DrawTransparent(ScenePipeline* scene)
{
	const unsigned int offset = 0;

	for(auto cur: transparent_array)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);

		cur->material->Set(TECHNIQUES::TECHNIQUE_DEFAULT);
		Render::SetTopology(cur->topo);

		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}
}

void SceneRenderMgr::cleanRenderArrayHud()
{
	for(auto cur: hud_array)
	{
		if(!cur->index_count)
			continue;
		if(cur->group)
		{
			MeshGroup<RenderMesh>* temp_group = cur->group;

			for(unsigned int i=0; i<temp_group->mesh_count; i++)
			{
				temp_group->meshes[i]->group = nullptr;
			}
			_DELETE_ARRAY(temp_group->meshes);
			_DELETE(temp_group);
		}
		cur->index_count = 0;
		_DELETE(cur);
	}
	hud_array.clear();

	for(auto cur: ovhud_array)
	{
		if(!cur->index_count)
			continue;
		if(cur->group)
		{
			MeshGroup<RenderMesh>* temp_group = cur->group;

			for(unsigned int i=0; i<temp_group->mesh_count; i++)
			{
				temp_group->meshes[i]->group = nullptr;
			}
			_DELETE_ARRAY(temp_group->meshes);
			_DELETE(temp_group);
		}
		cur->index_count = 0;
		_DELETE(cur);
	}
	ovhud_array.clear();
}