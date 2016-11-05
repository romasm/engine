#include "stdafx.h"
#include "RenderMgrs.h"
#include "ScenePipeline.h"
#include "ECS/CameraSystem.h"

using namespace EngineCore;

SceneRenderMgr::SceneRenderMgr() : BaseRenderMgr()
{
	b_shadow = false;
	
	opaque_array.create(OPAQUE_FRAME_MAX);
	alphatest_array.create(OPAQUE_FRAME_ALPHATEST_MAX);
	transparent_array.create(TRANSPARENT_FRAME_MAX);

	hud_array.create(HUD_FRAME_MAX);
	ovhud_array.create(OV_HUD_FRAME_MAX);

	lightSpot_array = new SpotLightBuffer;
	lightSpotDisk_array = new SpotLightDiskBuffer;
	lightSpotRect_array = new SpotLightRectBuffer;
	lightPoint_array = new PointLightBuffer;
	lightPointSphere_array = new PointLightSphereBuffer;
	lightPointTube_array = new PointLightTubeBuffer;
	lightDir_array = new DirLightBuffer;

	casterSpot_array = new SpotCasterBuffer;
	casterSpotDisk_array = new SpotCasterDiskBuffer;
	casterSpotRect_array = new SpotCasterRectBuffer;
	casterPoint_array = new PointCasterBuffer;	
	casterPointSphere_array = new PointCasterSphereBuffer;	
	casterPointTube_array = new PointCasterTubeBuffer;	
	
	shadowsBuffer = nullptr;
	shadowsBufferSRV = nullptr;
	for(uint8_t i = 0; i < SHADOWS_BUF_SIZE; i++)
		shadowsBufferDSV[i] = nullptr;
	
	shadowmap_array.create(SHADOWMAPS_COUNT);

	castersIdx.resize(ENTITY_COUNT);
	castersIdx.assign(ENTITY_COUNT);

	cascadeShadowRes = 1024;

	if(!initShadowBuffer())
		ERR("Failed init shadows buffer");

	current_cam = nullptr;

	shadows_sizes[0].size = SHADOWS_LOD_0_SIZE;
	shadows_sizes[1].size = SHADOWS_LOD_1_SIZE;
	shadows_sizes[2].size = SHADOWS_LOD_2_SIZE;
	shadows_sizes[3].size = SHADOWS_LOD_3_SIZE;
	shadows_sizes[4].size = SHADOWS_LOD_4_SIZE;
	shadows_sizes[5].size = SHADOWS_LOD_5_SIZE;
	shadows_sizes[0].res = SHADOWS_MAXRES;
	shadows_sizes[1].res = SHADOWS_MAXRES / 2;
	shadows_sizes[2].res = SHADOWS_MAXRES / 4;
	shadows_sizes[3].res = SHADOWS_MAXRES / 8;
	shadows_sizes[4].res = SHADOWS_MAXRES / 16;
	shadows_sizes[5].res = SHADOWS_MAXRES / 32;

	voxelizationDumb = nullptr;
	voxelizationDumbRTV = nullptr;
	voxelScene = nullptr;
	voxelSceneUAV = nullptr;
	voxelSceneSRV = nullptr;
	voxelSceneColor0 = nullptr;
	voxelSceneColor0UAV = nullptr;
	voxelSceneColor0SRV = nullptr;
	voxelSceneColor1 = nullptr;
	voxelSceneColor1UAV = nullptr;
	voxelSceneColor1SRV = nullptr;
	voxelSceneNormal = nullptr;
	voxelSceneNormalUAV = nullptr;
	voxelSceneNormalSRV = nullptr;
	voxelEmittance = nullptr;
	voxelEmittanceUAV = nullptr;
	voxelEmittanceSRV = nullptr;

	volumeBuffer = nullptr;
	volumeInfo = nullptr;
	voxelInjectLight = nullptr;

	if(!initVoxelBuffer())
		ERR("Failed init voxel buffer");

	ClearAll();
}

bool SceneRenderMgr::initShadowBuffer()
{
	D3D11_TEXTURE2D_DESC shadowBufferDesc;
	ZeroMemory(&shadowBufferDesc, sizeof(shadowBufferDesc));
	shadowBufferDesc.Width = SHADOWS_BUF_RES;
	shadowBufferDesc.Height = SHADOWS_BUF_RES;
	shadowBufferDesc.MipLevels = 1;
	shadowBufferDesc.ArraySize = SHADOWS_BUF_SIZE;
	shadowBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS; //DXGI_FORMAT_D32_FLOAT
	shadowBufferDesc.SampleDesc.Count = 1;
	shadowBufferDesc.SampleDesc.Quality = 0;
	shadowBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	shadowBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowBufferDesc.CPUAccessFlags = 0;
	shadowBufferDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture2D(&shadowBufferDesc, NULL, &shadowsBuffer)) )
		return false;
	
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2DArray.MipLevels = -1;	
	shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;	
	shaderResourceViewDesc.Texture2DArray.ArraySize = SHADOWS_BUF_SIZE;	
	if( FAILED(Render::CreateShaderResourceView(shadowsBuffer, &shaderResourceViewDesc, &shadowsBufferSRV)) )
		return false;
	
	for(uint8_t i = 0; i < SHADOWS_BUF_SIZE; i++)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
		depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		depthStencilViewDesc.Texture2DArray.MipSlice = 0;
		depthStencilViewDesc.Texture2DArray.ArraySize = 1;
		depthStencilViewDesc.Texture2DArray.FirstArraySlice = i;
		if( FAILED(Render::CreateDepthStencilView(shadowsBuffer, &depthStencilViewDesc, &shadowsBufferDSV[i])) )
			return false;
	}

	return true;
}

bool SceneRenderMgr::CompareShadows(ShadowMap& first, ShadowMap& second)
{
	return first.size < second.size;
}

void SceneRenderMgr::SwapShadows(ShadowMap* first, ShadowMap* second, RArray<ShadowMap>* arr)
{
	uint32_t fpos = (uint32_t(first) - uint32_t(arr->begin())) / sizeof(ShadowMap);
	uint32_t spos = (uint32_t(second) - uint32_t(arr->begin())) / sizeof(ShadowMap);

	bool second_prev = false;
	bool second_next = false;

	if(first->prev != ENTITY_COUNT)
	{
		if(first->prev == spos)
			second_prev = true;
		(*arr)[first->prev].next = spos;
	}
	
	if(first->next != ENTITY_COUNT)
	{
		if(first->next == spos)
			second_next = true;
		(*arr)[first->next].prev = spos;
	}
	
	if(second->prev != ENTITY_COUNT)
	{
		if(second_next)
			first->next = fpos;
		else
			(*arr)[second->prev].next = fpos;
	}

	if(second->next != ENTITY_COUNT)
	{
		if(second_prev)
			first->prev = fpos;
		else
			(*arr)[second->next].prev = fpos;
	}

	swap(*first, *second);
}

void SceneRenderMgr::PlaceShadowMaps()
{
	SArray<POINT, SHADOWS_STRINGS_NUM> strings;
	POINT z;
	z.x = 0;
	z.y = 0;
	strings.push_back(z);

	uint8_t control_size_id = 0;
	for(uint16_t i=0; i < shadowmap_array.size(); i++)
	{
		auto& e = shadowmap_array[i];
		while(e.size < shadows_sizes[control_size_id].size && control_size_id < SHADOWS_STRINGS_NUM-1)
		{
			strings.push_back(strings[control_size_id]);
			control_size_id++;
		}

		auto& p = strings[control_size_id];

		shadowmap_array[i].res = float(shadows_sizes[control_size_id].res);
		shadowmap_array[i].x = float(p.x);
		shadowmap_array[i].y = float(p.y % SHADOWS_BUF_RES);
		shadowmap_array[i].dsv = uint8_t(p.y / SHADOWS_BUF_RES);
		
		p.x += shadows_sizes[control_size_id].res;
		if(p.x >= SHADOWS_BUF_RES)
		{
			p.y += shadows_sizes[control_size_id].res;

			if(control_size_id > 0)
			{
				auto* curr = &strings[control_size_id];
				auto* prev = &strings[control_size_id - 1];
				p.x = prev->x;

				uint8_t cur_size_id = control_size_id;
				while(cur_size_id > 0 && curr->y - prev->y >= shadows_sizes[cur_size_id - 1].res)
				{
					curr->x = -1;
					curr->y = -1;
					cur_size_id--;
					prev->y += shadows_sizes[cur_size_id].res;
					curr = prev;
					if(cur_size_id <= 0)
						break;
					prev = &strings[cur_size_id - 1];
				}

				if(cur_size_id > 0)
					curr->x = prev->x;
				else
					curr->x = 0;

				for(uint8_t j = 1; j < SHADOWS_STRINGS_NUM; j++)
					if(strings[j].x < 0)
						strings[j] = strings[j-1];
			}
			else
				p.x = 0;
		}
	}

	if(strings[strings.size()-1].y < SHADOWS_BUF_RES * SHADOWS_BUF_SIZE / 4 && shadows_sizes[0].res < SHADOWS_MAXRES)
	{
		LOG("Shadows buffer seams free! Target shadow resolution upscales.");
		for(uint8_t k = 0; k < 6; k++)
			shadows_sizes[k].res = min(shadows_sizes[k].res * 2, SHADOWS_MAXRES / uint16_t(pow(2, k)));
		PlaceShadowMaps();
		return;
	}
	else if(strings[strings.size()-1].y >= SHADOWS_BUF_RES * SHADOWS_BUF_SIZE && strings[strings.size()-1].x > 0 && shadows_sizes[0].res > SHADOWS_MINRES)
	{
		WRN("Shadows buffer overflow! Target shadow resolution downscales.");
		for(uint8_t k = 0; k < 6; k++)
			shadows_sizes[k].res = max(shadows_sizes[k].res / 2, SHADOWS_MINRES);
		PlaceShadowMaps();
		return;
	}
}

void SceneRenderMgr::ResolveShadowMaps()
{
	QSortSwap(shadowmap_array.begin(), shadowmap_array.end(), SceneRenderMgr::CompareShadows, SceneRenderMgr::SwapShadows, &shadowmap_array);

	for(int i=0; i < shadowmap_array.size(); i++)
		if(shadowmap_array[i].prev == ENTITY_COUNT)
			castersIdx[shadowmap_array[i].id] = i;

	for(int i=0; i<SHADOWS_BUF_SIZE; i++)
		Render::ClearDepthStencilView(shadowsBufferDSV[i], D3D11_CLEAR_DEPTH, 1.0f, 0);

	PlaceShadowMaps();
}

bool SceneRenderMgr::RegShadowMap(uint id,  float size)
{
	uint32_t subId = (uint32_t)shadowmap_array.size();
	if(subId >= SHADOWMAPS_COUNT)
		return false;

	ShadowMap shm;
	shm.id = id;
	shm.size = size;
	shm.next = ENTITY_COUNT;
	if(castersIdx[id] == ENTITY_COUNT)
	{
		shm.prev = ENTITY_COUNT;
		castersIdx[id] = subId;
	}
	else
	{
		uint32_t prev = castersIdx[id];
		uint32_t* next = &shadowmap_array[prev].next;
		while (*next != ENTITY_COUNT)
		{
			prev = *next;
			next = &shadowmap_array[prev].next;
		}
		*next = subId;
		shm.prev = prev;
	}
	
	shadowmap_array.push_back(shm);
	
	return true;
}

void SceneRenderMgr::RenderShadow(uint id, uchar num, ShadowRenderMgr* shadow_mgr, ID3D11Buffer* vp)
{
	ShadowMap shm = shadowmap_array[castersIdx[id]];
	for(uint i=0; i<num; i++)
		shm = shadowmap_array[shm.next];

	Render::OMSetRenderTargets(0, nullptr, shadowsBufferDSV[min(shm.dsv, SHADOWS_BUF_SIZE-1)]);
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = shm.x;
	viewport.TopLeftY = shm.y;
	viewport.Height = viewport.Width = shm.res;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	Render::RSSetViewports(1, &viewport);

	Render::VSSetConstantBuffers(0, 1, &vp);

	shadow_mgr->DrawOpaque();
	shadow_mgr->DrawAlphatest();

	/*if(shadow_mgr->IsTranparentShadows())
	{
		// transparent render target
		shadow_mgr->DrawTransparent();
	}*/

	Render::OMSetRenderTargets(0, nullptr, nullptr);
}

bool SceneRenderMgr::initVoxelBuffer()
{
	D3D11_TEXTURE2D_DESC dumbDesc;
	ZeroMemory(&dumbDesc, sizeof(dumbDesc));
	dumbDesc.Width = VOXEL_VOLUME_RES;
	dumbDesc.Height = VOXEL_VOLUME_RES;
	dumbDesc.MipLevels = 1;
	dumbDesc.ArraySize = 1;
	dumbDesc.Format = DXGI_FORMAT_R8_UNORM;
	dumbDesc.SampleDesc.Count = VOXEL_VOLUME_SUBSAMPLES;
	dumbDesc.SampleDesc.Quality = 0;
	dumbDesc.Usage = D3D11_USAGE_DEFAULT;
	dumbDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	dumbDesc.CPUAccessFlags = 0;
	dumbDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture2D(&dumbDesc, NULL, &voxelizationDumb)) )
		return false;

	D3D11_RENDER_TARGET_VIEW_DESC dumbRTVDesc;
	ZeroMemory(&dumbRTVDesc, sizeof(dumbRTVDesc));
	dumbRTVDesc.Format = DXGI_FORMAT_R8_UNORM;
	dumbRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	dumbRTVDesc.Texture2D.MipSlice = 0;
	if( FAILED(Render::CreateRenderTargetView(voxelizationDumb, &dumbRTVDesc, &voxelizationDumbRTV)) )
		return false;

	// visibility
	D3D11_TEXTURE3D_DESC volumeDesc;
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = VOXEL_VOLUME_RES * VOXEL_VOLUME_CLIPMAP_COUNT;
	volumeDesc.Height = VOXEL_VOLUME_RES * 6;
	volumeDesc.Depth = VOXEL_VOLUME_RES;
	volumeDesc.MipLevels = 1;
	volumeDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelScene)) )
		return false;

	D3D11_UNORDERED_ACCESS_VIEW_DESC volumeUAVDesc;
	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = VOXEL_VOLUME_RES;
	if( FAILED(Render::CreateUnorderedAccessView(voxelScene, &volumeUAVDesc, &voxelSceneUAV)) )
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC volumeSRVDesc;
	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	if( FAILED(Render::CreateShaderResourceView(voxelScene, &volumeSRVDesc, &voxelSceneSRV)) )
		return false;

	// color
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = VOXEL_VOLUME_RES * VOXEL_VOLUME_CLIPMAP_COUNT;
	volumeDesc.Height = VOXEL_VOLUME_RES * 6;
	volumeDesc.Depth = VOXEL_VOLUME_RES;
	volumeDesc.MipLevels = 1;
	volumeDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelSceneColor0)) )
		return false;

	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = VOXEL_VOLUME_RES;
	if( FAILED(Render::CreateUnorderedAccessView(voxelSceneColor0, &volumeUAVDesc, &voxelSceneColor0UAV)) )
		return false;

	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	if( FAILED(Render::CreateShaderResourceView(voxelSceneColor0, &volumeSRVDesc, &voxelSceneColor0SRV)) )
		return false;

	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = VOXEL_VOLUME_RES * VOXEL_VOLUME_CLIPMAP_COUNT;
	volumeDesc.Height = VOXEL_VOLUME_RES * 6;
	volumeDesc.Depth = VOXEL_VOLUME_RES;
	volumeDesc.MipLevels = 1;
	volumeDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelSceneColor1)) )
		return false;

	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = VOXEL_VOLUME_RES;
	if( FAILED(Render::CreateUnorderedAccessView(voxelSceneColor1, &volumeUAVDesc, &voxelSceneColor1UAV)) )
		return false;

	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	if( FAILED(Render::CreateShaderResourceView(voxelSceneColor1, &volumeSRVDesc, &voxelSceneColor1SRV)) )
		return false;

	// normal
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = VOXEL_VOLUME_RES * VOXEL_VOLUME_CLIPMAP_COUNT;
	volumeDesc.Height = VOXEL_VOLUME_RES * 6;
	volumeDesc.Depth = VOXEL_VOLUME_RES;
	volumeDesc.MipLevels = 1;
	volumeDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelSceneNormal)) )
		return false;

	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = VOXEL_VOLUME_RES;
	if( FAILED(Render::CreateUnorderedAccessView(voxelSceneNormal, &volumeUAVDesc, &voxelSceneNormalUAV)) )
		return false;

	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.Format = DXGI_FORMAT_R32_UINT;
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	if( FAILED(Render::CreateShaderResourceView(voxelSceneNormal, &volumeSRVDesc, &voxelSceneNormalSRV)) )
		return false;
	
	// emittance
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = VOXEL_VOLUME_RES * VOXEL_VOLUME_CLIPMAP_COUNT;
	volumeDesc.Height = VOXEL_VOLUME_RES * 6;
	volumeDesc.Depth = VOXEL_VOLUME_RES;
	volumeDesc.MipLevels = 1;
	volumeDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelEmittance)) )
		return false;

	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = VOXEL_VOLUME_RES;
	if( FAILED(Render::CreateUnorderedAccessView(voxelEmittance, &volumeUAVDesc, &voxelEmittanceUAV)) )
		return false;

	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	if( FAILED(Render::CreateShaderResourceView(voxelEmittance, &volumeSRVDesc, &voxelEmittanceSRV)) )
		return false;

	volumeBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeData), true);
	volumeInfo = Buffer::CreateConstantBuffer(DEVICE, sizeof(XMFLOAT4), true);

	spotLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, SPOT_VOXEL_FRAME_MAX, sizeof(SpotVoxelBuffer), true);
	pointLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, POINT_VOXEL_FRAME_MAX, sizeof(PointVoxelBuffer), true);
	dirLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, LIGHT_DIR_FRAME_MAX, sizeof(DirVoxelBuffer), true);

	voxelInjectLight = new Compute( COMPUTE_VOXEL_INJECT_LIGHT, "InjectLightToVolume" );

	return true;
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

bool SceneRenderMgr::RegSpotLight(XMFLOAT4 color, float range, XMFLOAT2 cone, XMFLOAT3 pos,	XMFLOAT3 dir)
{
	if(lightSpot_count >= LIGHT_SPOT_FRAME_MAX)
		return false;
	
	lightSpot_array->Pos_Range[lightSpot_count] = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightSpot_array->Color_ConeX[lightSpot_count] = XMFLOAT4(color.x, color.y, color.z, cone.x);
	lightSpot_array->Dir_ConeY[lightSpot_count] = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);

	lightSpot_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCaster(XMFLOAT4 color, float range, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, float nearclip, CXMMATRIX vp, CXMMATRIX proj, UINT id)
{
	if(casterSpot_count >= CASTER_SPOT_FRAME_MAX)
		return false;
	
	ShadowMap shm = shadowmap_array[castersIdx[id]];

	// to voxels
	auto vData = spotVoxel_array.push_back();
	vData->PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData->ColorConeX = XMFLOAT4(color.x, color.y, color.z, cone.x);
	vData->DirConeY = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	vData->Virtpos = vData->PosRange;
	vData->ShadowmapAdress =  XMFLOAT4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP, 
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	vData->ShadowmapHPixProjNearclip = XMFLOAT4(PIXEL_HALF / shm.res, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), nearclip, 0);
	vData->matViewProj = vp;

	// to deffered
	casterSpot_array->Pos_Range[casterSpot_count] = vData->PosRange;
	casterSpot_array->Color_ConeX[casterSpot_count] = vData->ColorConeX;
	casterSpot_array->Dir_ConeY[casterSpot_count] = vData->DirConeY;
	casterSpot_array->ShadowmapAdress[casterSpot_count] = vData->ShadowmapAdress;
	casterSpot_array->ShadowmapParams[casterSpot_count] = vData->ShadowmapHPixProjNearclip;
	casterSpot_array->ViewProj[casterSpot_count] = vp;

	casterSpot_count++;

	return true;
}

bool SceneRenderMgr::RegSpotLightDisk(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 virtpos)
{
	if(lightSpotDisk_count >= LIGHT_SPOT_DISK_FRAME_MAX)
		return false;
	
	lightSpotDisk_array->Pos_Range[lightSpotDisk_count] = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightSpotDisk_array->Color_ConeX[lightSpotDisk_count] = XMFLOAT4(color.x, color.y, color.z, cone.x);
	lightSpotDisk_array->Dir_ConeY[lightSpotDisk_count] = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	lightSpotDisk_array->AreaInfo_Empty[lightSpotDisk_count] = XMFLOAT4(area.x, area.y, 0, 0);
	lightSpotDisk_array->Virtpos_Empty[lightSpotDisk_count] = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, 0);

	lightSpotDisk_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCasterDisk(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 virtpos, float nearclip,
			CXMMATRIX vp, CXMMATRIX proj, UINT id)
{
	if(casterSpotDisk_count >= CASTER_SPOT_DISK_FRAME_MAX)
		return false;
	
	ShadowMap shm = shadowmap_array[castersIdx[id]];
	
	// to voxels
	auto vData = spotVoxel_array.push_back();
	vData->PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData->ColorConeX = XMFLOAT4(color.x, color.y, color.z, cone.x);
	vData->DirConeY = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	vData->Virtpos = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, 0);
	vData->ShadowmapAdress =  XMFLOAT4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP, 
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	vData->ShadowmapHPixProjNearclip = XMFLOAT4(PIXEL_HALF / shm.res, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), nearclip, 0);
	vData->matViewProj = vp;

	// to deffered
	casterSpotDisk_array->Pos_Range[casterSpotDisk_count] = vData->PosRange;
	casterSpotDisk_array->Color_ConeX[casterSpotDisk_count] = vData->ColorConeX;
	casterSpotDisk_array->Dir_ConeY[casterSpotDisk_count] = vData->DirConeY;
	casterSpotDisk_array->AreaInfo_Empty[casterSpotDisk_count] = XMFLOAT4(area.x, area.y, 0, 0);
	casterSpotDisk_array->Virtpos_Empty[casterSpotDisk_count] = vData->Virtpos;
	casterSpotDisk_array->ShadowmapAdress[casterSpotDisk_count] = vData->ShadowmapAdress;
	casterSpotDisk_array->ShadowmapParams[casterSpotDisk_count] = vData->ShadowmapHPixProjNearclip;
	casterSpotDisk_array->ViewProj[casterSpotDisk_count] = vp;
	
	casterSpotDisk_count++;

	return true;
}

bool SceneRenderMgr::RegSpotLightRect(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 up, XMFLOAT3 side, XMFLOAT3 virtpos)
{
	if(lightSpotRect_count >= LIGHT_SPOT_RECT_FRAME_MAX)
		return false;
	
	lightSpotRect_array->Pos_Range[lightSpotRect_count] = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightSpotRect_array->Color_ConeX[lightSpotRect_count] = XMFLOAT4(color.x, color.y, color.z, cone.x);
	lightSpotRect_array->Dir_ConeY[lightSpotRect_count] = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	lightSpotRect_array->DirUp_AreaX[lightSpotRect_count] = XMFLOAT4(up.x, up.y, up.z, area.x);
	lightSpotRect_array->DirSide_AreaY[lightSpotRect_count] = XMFLOAT4(side.x, side.y, side.z, area.y);
	lightSpotRect_array->Virtpos_AreaZ[lightSpotRect_count] = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, area.z);

	lightSpotRect_count++;

	return true;
}

bool SceneRenderMgr::RegSpotCasterRect(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT2 cone, XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 up, XMFLOAT3 side, XMFLOAT3 virtpos, float nearclip, 
			CXMMATRIX vp, CXMMATRIX proj, UINT id)
{
	if(casterSpotRect_count >= CASTER_SPOT_RECT_FRAME_MAX) // proj tex coords wrong calc
		return false;
	
	ShadowMap shm = shadowmap_array[castersIdx[id]];
	
	// to voxels
	auto vData = spotVoxel_array.push_back();
	vData->PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData->ColorConeX = XMFLOAT4(color.x, color.y, color.z, cone.x);
	vData->DirConeY = XMFLOAT4(dir.x, dir.y, dir.z, cone.y);
	vData->Virtpos = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, 0);
	vData->ShadowmapAdress =  XMFLOAT4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP, 
		shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
	vData->ShadowmapHPixProjNearclip = XMFLOAT4(PIXEL_HALF / shm.res, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), nearclip, 0);
	vData->matViewProj = vp;

	// to deffered
	casterSpotRect_array->Pos_Range[casterSpotRect_count] = vData->PosRange;
	casterSpotRect_array->Color_ConeX[casterSpotRect_count] = vData->ColorConeX;
	casterSpotRect_array->Dir_ConeY[casterSpotRect_count] = vData->DirConeY;
	casterSpotRect_array->DirUp_AreaX[casterSpotRect_count] = XMFLOAT4(up.x, up.y, up.z, area.x);
	casterSpotRect_array->DirSide_AreaY[casterSpotRect_count] = XMFLOAT4(side.x, side.y, side.z, area.y);
	casterSpotRect_array->Virtpos_AreaZ[casterSpotRect_count] = XMFLOAT4(virtpos.x, virtpos.y, virtpos.z, area.z);
	casterSpotRect_array->ShadowmapAdress[casterSpotRect_count] = vData->ShadowmapAdress;
	casterSpotRect_array->ShadowmapParams[casterSpotRect_count] = vData->ShadowmapHPixProjNearclip;
	casterSpotRect_array->ViewProj[casterSpotRect_count] = vp;

	casterSpotRect_count++;

	return true;
}

bool SceneRenderMgr::RegPointLight(XMFLOAT4 color, float range, XMFLOAT3 pos)
{
	if(lightPoint_count >= LIGHT_POINT_FRAME_MAX)
		return false;

	lightPoint_array->Pos_Range[lightPoint_count] = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightPoint_array->Color[lightPoint_count] = XMFLOAT4(color.x, color.y, color.z, 0);

	lightPoint_count++;

	return true;
}

bool SceneRenderMgr::RegPointCaster(XMFLOAT4 color, float range, XMFLOAT3 pos, CXMMATRIX proj, UINT id)
{
	if(casterPoint_count >= CASTER_POINT_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowmap_array[castersIdx[id]];
	for(uint i=0; i<5; i++)
		shm[i+1] = shadowmap_array[shm[i].next];

	// to voxels
	auto vData = pointVoxel_array.push_back();
	vData->PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData->ColorShadowmapProj = XMFLOAT4(color.x, color.y, color.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	vData->ShadowmapAdress0 = XMFLOAT4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	vData->ShadowmapAdress1 = XMFLOAT4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	vData->ShadowmapAdress2 = XMFLOAT4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	vData->ShadowmapAdress3 = XMFLOAT4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	vData->ShadowmapAdress4 = XMFLOAT4(shm[4].x * SHADOWS_BUF_RES_RCP, shm[4].y * SHADOWS_BUF_RES_RCP, 
		shm[4].res * SHADOWS_BUF_RES_RCP, (float)shm[4].dsv);
	vData->ShadowmapAdress5 = XMFLOAT4(shm[5].x * SHADOWS_BUF_RES_RCP, shm[5].y * SHADOWS_BUF_RES_RCP, 
		shm[5].res * SHADOWS_BUF_RES_RCP, (float)shm[5].dsv);
	vData->ShadowmapHPix0 = XMFLOAT4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	vData->ShadowmapHPix1 = XMFLOAT4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, 0, 0);
	vData->matProj = proj;

	// to deffered
	casterPoint_array->Pos_Range[casterPoint_count] = vData->PosRange;
	casterPoint_array->Color_ShParams[casterPoint_count] = vData->ColorShadowmapProj;	
	casterPoint_array->ShadowmapParams0[casterPoint_count] = vData->ShadowmapHPix0;
	casterPoint_array->ShadowmapParams1[casterPoint_count] = vData->ShadowmapHPix1;
	casterPoint_array->Proj[casterPoint_count] = proj;
	casterPoint_array->ShadowmapAdress0[casterPoint_count] = vData->ShadowmapAdress0;
	casterPoint_array->ShadowmapAdress1[casterPoint_count] = vData->ShadowmapAdress1;
	casterPoint_array->ShadowmapAdress2[casterPoint_count] = vData->ShadowmapAdress2;
	casterPoint_array->ShadowmapAdress3[casterPoint_count] = vData->ShadowmapAdress3;
	casterPoint_array->ShadowmapAdress4[casterPoint_count] = vData->ShadowmapAdress4;
	casterPoint_array->ShadowmapAdress5[casterPoint_count] = vData->ShadowmapAdress5;
	
	casterPoint_count++;

	return true;
}

bool SceneRenderMgr::RegPointLightSphere(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos)
{
	if(lightPointSphere_count >= LIGHT_POINT_SPHERE_FRAME_MAX)
		return false;

	lightPointSphere_array->Pos_Range[lightPointSphere_count] = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightPointSphere_array->Color_Empty[lightPointSphere_count] = XMFLOAT4(color.x, color.y, color.z, 0);
	lightPointSphere_array->AreaInfo_Empty[lightPointSphere_count] = XMFLOAT4(area.x, area.x * area.x, 0, 0);

	lightPointSphere_count++;

	return true;
}

bool SceneRenderMgr::RegPointCasterSphere(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos, CXMMATRIX proj, UINT id)
{
	if(casterPointSphere_count >= CASTER_POINT_SPHERE_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowmap_array[castersIdx[id]];
	for(uint i=0; i<5; i++)
		shm[i+1] = shadowmap_array[shm[i].next];
	
	// to voxels
	auto vData = pointVoxel_array.push_back();
	vData->PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData->ColorShadowmapProj = XMFLOAT4(color.x, color.y, color.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	vData->ShadowmapAdress0 = XMFLOAT4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	vData->ShadowmapAdress1 = XMFLOAT4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	vData->ShadowmapAdress2 = XMFLOAT4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	vData->ShadowmapAdress3 = XMFLOAT4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	vData->ShadowmapAdress4 = XMFLOAT4(shm[4].x * SHADOWS_BUF_RES_RCP, shm[4].y * SHADOWS_BUF_RES_RCP, 
		shm[4].res * SHADOWS_BUF_RES_RCP, (float)shm[4].dsv);
	vData->ShadowmapAdress5 = XMFLOAT4(shm[5].x * SHADOWS_BUF_RES_RCP, shm[5].y * SHADOWS_BUF_RES_RCP, 
		shm[5].res * SHADOWS_BUF_RES_RCP, (float)shm[5].dsv);
	vData->ShadowmapHPix0 = XMFLOAT4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	vData->ShadowmapHPix1 = XMFLOAT4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, 0, 0);
	vData->matProj = proj;

	// to deffered
	casterPointSphere_array->Pos_Range[casterPointSphere_count] = vData->PosRange;
	casterPointSphere_array->Color_ShParams[casterPointSphere_count] = vData->ColorShadowmapProj;
	casterPointSphere_array->AreaInfo_ShParams[casterPointSphere_count] = XMFLOAT4(area.x, area.x * area.x, vData->ShadowmapHPix1.x, vData->ShadowmapHPix1.y);
	casterPointSphere_array->ShadowmapParams[casterPointSphere_count] = vData->ShadowmapHPix0;
	casterPointSphere_array->Proj[casterPointSphere_count] = proj;
	casterPointSphere_array->ShadowmapAdress0[casterPointSphere_count] = vData->ShadowmapAdress0;
	casterPointSphere_array->ShadowmapAdress1[casterPointSphere_count] = vData->ShadowmapAdress1;
	casterPointSphere_array->ShadowmapAdress2[casterPointSphere_count] = vData->ShadowmapAdress2;
	casterPointSphere_array->ShadowmapAdress3[casterPointSphere_count] = vData->ShadowmapAdress3;
	casterPointSphere_array->ShadowmapAdress4[casterPointSphere_count] = vData->ShadowmapAdress4;
	casterPointSphere_array->ShadowmapAdress5[casterPointSphere_count] = vData->ShadowmapAdress5;
	
	casterPointSphere_count++;

	return true;
}
		
bool SceneRenderMgr::RegPointLightTube(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos, XMFLOAT3 dir)
{
	if(lightPointTube_count >= LIGHT_POINT_TUBE_FRAME_MAX)
		return false;

	lightPointTube_array->Pos_Range[lightPointTube_count] = XMFLOAT4(pos.x, pos.y, pos.z, range);
	lightPointTube_array->Color_Empty[lightPointTube_count] = XMFLOAT4(color.x, color.y, color.z, 0);
	lightPointTube_array->AreaInfo[lightPointTube_count] = XMFLOAT4(area.x, area.y, area.z, area.y * area.y);
	lightPointTube_array->Dir_AreaA[lightPointTube_count] = XMFLOAT4(dir.x, dir.y, dir.z, area.y + 2 * area.x);

	lightPointTube_count++;

	return true;
}

bool SceneRenderMgr::RegPointCasterTube(XMFLOAT4 color, float range, XMFLOAT3 area, XMFLOAT3 pos, XMFLOAT3 dir, CXMMATRIX proj, CXMMATRIX view, UINT id)
{
	if(casterPointTube_count >= CASTER_POINT_TUBE_FRAME_MAX)
		return false;

	ShadowMap shm[6];
	shm[0] = shadowmap_array[castersIdx[id]];
	for(uint i=0; i<5; i++)
		shm[i+1] = shadowmap_array[shm[i].next];
	
	// to voxels
	auto vData = pointVoxel_array.push_back();
	vData->PosRange = XMFLOAT4(pos.x, pos.y, pos.z, range);
	vData->ColorShadowmapProj = XMFLOAT4(color.x, color.y, color.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
	vData->ShadowmapAdress0 = XMFLOAT4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	vData->ShadowmapAdress1 = XMFLOAT4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	vData->ShadowmapAdress2 = XMFLOAT4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	vData->ShadowmapAdress3 = XMFLOAT4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	vData->ShadowmapAdress4 = XMFLOAT4(shm[4].x * SHADOWS_BUF_RES_RCP, shm[4].y * SHADOWS_BUF_RES_RCP, 
		shm[4].res * SHADOWS_BUF_RES_RCP, (float)shm[4].dsv);
	vData->ShadowmapAdress5 = XMFLOAT4(shm[5].x * SHADOWS_BUF_RES_RCP, shm[5].y * SHADOWS_BUF_RES_RCP, 
		shm[5].res * SHADOWS_BUF_RES_RCP, (float)shm[5].dsv);
	vData->ShadowmapHPix0 = XMFLOAT4( PIXEL_HALF / shm[0].res, PIXEL_HALF / shm[1].res, PIXEL_HALF / shm[2].res, PIXEL_HALF / shm[3].res);
	vData->ShadowmapHPix1 = XMFLOAT4( PIXEL_HALF / shm[4].res, PIXEL_HALF / shm[5].res, 0, 0);
	vData->matProj = proj;

	// to deffered
	casterPointTube_array->Pos_Range[casterPointTube_count] = vData->PosRange;
	casterPointTube_array->Color_ShParams[casterPointTube_count] = vData->ColorShadowmapProj;
	casterPointTube_array->AreaInfo[casterPointTube_count] = XMFLOAT4(area.x, area.y, area.z, area.y * area.y);
	casterPointTube_array->Dir_AreaA[casterPointTube_count] = XMFLOAT4(dir.x, dir.y, dir.z, area.y + 2 * area.x);
	casterPointTube_array->ShadowmapParams0[casterPointTube_count] = vData->ShadowmapHPix0;
	casterPointTube_array->ShadowmapParams1[casterPointTube_count] = vData->ShadowmapHPix1;
	casterPointTube_array->Proj[casterPointTube_count] = proj;
	casterPointTube_array->View[casterPointTube_count] = view;
	casterPointTube_array->ShadowmapAdress0[casterPointTube_count] = vData->ShadowmapAdress0;
	casterPointTube_array->ShadowmapAdress1[casterPointTube_count] = vData->ShadowmapAdress1;
	casterPointTube_array->ShadowmapAdress2[casterPointTube_count] = vData->ShadowmapAdress2;
	casterPointTube_array->ShadowmapAdress3[casterPointTube_count] = vData->ShadowmapAdress3;
	casterPointTube_array->ShadowmapAdress4[casterPointTube_count] = vData->ShadowmapAdress4;
	casterPointTube_array->ShadowmapAdress5[casterPointTube_count] = vData->ShadowmapAdress5;
	
	casterPointTube_count++;

	return true;
}

bool SceneRenderMgr::RegDirLight(XMFLOAT4 color, XMFLOAT2 area, XMFLOAT3 dir, XMMATRIX* view_proj, XMFLOAT3* pos, uint64_t id)
{
	if(lightDir_count >= LIGHT_DIR_FRAME_MAX)
		return false;

	ShadowMap shm[LIGHT_DIR_NUM_CASCADES];
	shm[0] = shadowmap_array[castersIdx[id]];
	for(uint8_t i=0; i<LIGHT_DIR_NUM_CASCADES-1; i++)
		shm[i+1] = shadowmap_array[shm[i].next];

	// to voxel
	auto vData = dirVoxel_array.push_back();
	vData->Color = XMFLOAT4(color.x, color.y, color.z, area.x);
	vData->Dir = XMFLOAT4(dir.x, dir.y, dir.z, area.y);
	vData->PosHPix0 = XMFLOAT4(pos[0].x, pos[0].y, pos[0].z, PIXEL_HALF / shm[0].res);
	vData->PosHPix1 = XMFLOAT4(pos[1].x, pos[1].y, pos[1].z, PIXEL_HALF / shm[1].res);
	vData->PosHPix2 = XMFLOAT4(pos[2].x, pos[2].y, pos[2].z, PIXEL_HALF / shm[2].res);
	vData->PosHPix3 = XMFLOAT4(pos[3].x, pos[3].y, pos[3].z, PIXEL_HALF / shm[3].res);
	vData->ShadowmapAdress0 = XMFLOAT4(shm[0].x * SHADOWS_BUF_RES_RCP, shm[0].y * SHADOWS_BUF_RES_RCP, 
		shm[0].res * SHADOWS_BUF_RES_RCP, (float)shm[0].dsv);
	vData->ShadowmapAdress1 = XMFLOAT4(shm[1].x * SHADOWS_BUF_RES_RCP, shm[1].y * SHADOWS_BUF_RES_RCP, 
		shm[1].res * SHADOWS_BUF_RES_RCP, (float)shm[1].dsv);
	vData->ShadowmapAdress2 = XMFLOAT4(shm[2].x * SHADOWS_BUF_RES_RCP, shm[2].y * SHADOWS_BUF_RES_RCP, 
		shm[2].res * SHADOWS_BUF_RES_RCP, (float)shm[2].dsv);
	vData->ShadowmapAdress3 = XMFLOAT4(shm[3].x * SHADOWS_BUF_RES_RCP, shm[3].y * SHADOWS_BUF_RES_RCP, 
		shm[3].res * SHADOWS_BUF_RES_RCP, (float)shm[3].dsv);
	vData->ViewProj0 = view_proj[0];
	vData->ViewProj1 = view_proj[1];
	vData->ViewProj2 = view_proj[2];
	vData->ViewProj3 = view_proj[3];

	// to deffered
	lightDir_array->Color_AreaX[lightDir_count] = vData->Color;
	lightDir_array->Dir_AreaY[lightDir_count] = vData->Dir;

	// locked for 4 cascades
	lightDir_array->Pos0[lightDir_count] = vData->PosHPix0;
	lightDir_array->Pos1[lightDir_count] = vData->PosHPix1;
	lightDir_array->Pos2[lightDir_count] = vData->PosHPix2;
	lightDir_array->Pos3[lightDir_count] = vData->PosHPix3;
	lightDir_array->ViewProj0[lightDir_count] = view_proj[0];
	lightDir_array->ViewProj1[lightDir_count] = view_proj[1];
	lightDir_array->ViewProj2[lightDir_count] = view_proj[2];
	lightDir_array->ViewProj3[lightDir_count] = view_proj[3];
	lightDir_array->ShadowmapAdress0[lightDir_count] = vData->ShadowmapAdress0;
	lightDir_array->ShadowmapAdress1[lightDir_count] = vData->ShadowmapAdress1;
	lightDir_array->ShadowmapAdress2[lightDir_count] = vData->ShadowmapAdress2;
	lightDir_array->ShadowmapAdress3[lightDir_count] = vData->ShadowmapAdress3;

	lightDir_count++;

	return true;
}

void SceneRenderMgr::UpdateCamera(CameraComponent* cam)
{
	current_cam = cam;
	cameraPosition = current_cam->camPos;
}

void SceneRenderMgr::VoxelizeScene()
{
	const unsigned int offset = 0;

	Render::ClearUnorderedAccessViewUint(voxelSceneUAV, XMFLOAT4(0,0,0,0));
	Render::ClearUnorderedAccessViewUint(voxelSceneColor0UAV, XMFLOAT4(0,0,0,0));
	Render::ClearUnorderedAccessViewUint(voxelSceneColor1UAV, XMFLOAT4(0,0,0,0));
	Render::ClearUnorderedAccessViewUint(voxelSceneNormalUAV, XMFLOAT4(0,0,0,0));

	ID3D11UnorderedAccessView* uavs[4];
	uavs[0] = voxelSceneUAV;
	uavs[1] = voxelSceneColor0UAV;
	uavs[2] = voxelSceneColor1UAV;
	uavs[3] = voxelSceneNormalUAV;

	Render::OMSetRenderTargetsAndUnorderedAccessViews(1, &voxelizationDumbRTV, nullptr, 1, 4, uavs, nullptr);
	
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Height = viewport.Width = (float)VOXEL_VOLUME_RES;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	Render::RSSetViewports(1, &viewport);

	Render::SetTopology(IA_TOPOLOGY::TRISLIST);

	VolumeData constBuffer;
	constBuffer.cornerOffset = XMFLOAT3(0,0,0);
	constBuffer.worldSize = VOXEL_VOLUME_SIZE;
	constBuffer.scaleHelper = (float)VOXEL_VOLUME_RES / VOXEL_VOLUME_SIZE;
	constBuffer.volumeRes = VOXEL_VOLUME_RES;
	constBuffer.volumeDoubleRes = VOXEL_VOLUME_RES * 2;
	constBuffer.voxelSize = float(VOXEL_VOLUME_SIZE) / VOXEL_VOLUME_RES;

	// todo
	XMVECTOR camPoses[3];
	camPoses[0] = XMVectorSet(0.0f, VOXEL_VOLUME_SIZE * 0.5f, VOXEL_VOLUME_SIZE * 0.5f, 1.0f);
	camPoses[1] = XMVectorSet(VOXEL_VOLUME_SIZE * 0.5f, 0.0f, VOXEL_VOLUME_SIZE * 0.5f, 1.0f);
	camPoses[2] = XMVectorSet(VOXEL_VOLUME_SIZE * 0.5f, VOXEL_VOLUME_SIZE * 0.5f, 0.0f, 1.0f);
	
	XMVECTOR camDirs[3];
	camDirs[0] = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	camDirs[1] = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	camDirs[2] = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	XMVECTOR camUps[3];
	camUps[0] = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	camUps[1] = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	camUps[2] = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	
	for(uint8_t i = 0; i < 3; i++)
	{
		constBuffer.volumeVP[i] = XMMatrixLookToLH(camPoses[i], camDirs[i], camUps[i]);
		constBuffer.volumeVP[i] *= XMMatrixOrthographicLH(VOXEL_VOLUME_SIZE, VOXEL_VOLUME_SIZE, 0.0f, VOXEL_VOLUME_SIZE);
		constBuffer.volumeVP[i] = XMMatrixTranspose(constBuffer.volumeVP[i]);
	}

	Render::UpdateDynamicResource(volumeBuffer, (void*)&constBuffer, sizeof(VolumeData));
	Render::PSSetConstantBuffers(4, 1, &volumeBuffer); 
	Render::GSSetConstantBuffers(4, 1, &volumeBuffer); 

	// todo
	for(auto cur: opaque_array)
	{
		bool has_tq = false;
		auto queue = cur->material->GetTechQueue(TECHNIQUES::TECHNIQUE_VOXEL, &has_tq);
		if(!has_tq)
			continue;

		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);

		cur->material->Set(TECHNIQUES::TECHNIQUE_VOXEL);

		Render::Context()->DrawIndexed(cur->index_count, 0, 0);
	}

	Render::OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 0, nullptr, nullptr);
}

void SceneRenderMgr::ProcessEmittance()
{
	Render::ClearUnorderedAccessViewFloat(voxelEmittanceUAV, XMFLOAT4(0,0,0,0));

	Render::UpdateDynamicResource(spotLightInjectBuffer.buf, spotVoxel_array.data(), spotVoxel_array.size() * sizeof(SpotVoxelBuffer));
	Render::UpdateDynamicResource(pointLightInjectBuffer.buf, pointVoxel_array.data(), pointVoxel_array.size() * sizeof(SpotVoxelBuffer));
	Render::UpdateDynamicResource(dirLightInjectBuffer.buf, dirVoxel_array.data(), dirVoxel_array.size() * sizeof(SpotVoxelBuffer));

	uint32_t lightCount[4] = {(uint32_t)spotVoxel_array.size(), (uint32_t)pointVoxel_array.size(), 
		(uint32_t)dirVoxel_array.size(), 0};
	Render::UpdateDynamicResource(volumeInfo, lightCount, sizeof(XMFLOAT4));

	voxelInjectLight->BindUAV(voxelEmittanceUAV);

	Render::CSSetShaderResources(0, 1, &shadowsBufferSRV);

	Render::CSSetShaderResources(1, 1, &voxelSceneSRV);
	Render::CSSetShaderResources(2, 1, &voxelSceneColor0SRV);
	Render::CSSetShaderResources(3, 1, &voxelSceneColor1SRV);
	Render::CSSetShaderResources(4, 1, &voxelSceneNormalSRV);

	Render::CSSetShaderResources(5, 1, &spotLightInjectBuffer.srv);
	Render::CSSetShaderResources(6, 1, &pointLightInjectBuffer.srv);
	Render::CSSetShaderResources(7, 1, &dirLightInjectBuffer.srv);

	Render::CSSetConstantBuffers(0, 1, &volumeBuffer);
	Render::CSSetConstantBuffers(1, 1, &volumeInfo);

	voxelInjectLight->Dispatch( 8, 8, 8 );
	voxelInjectLight->UnbindUAV();
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

void SceneRenderMgr::DrawTransparent(ScenePipeline* scene)
{
	sort(transparent_array.begin(), transparent_array.end(), BaseRenderMgr::InvCompareMeshes );

	const unsigned int offset = 0;

	for(auto cur: transparent_array)
	{
		Render::Context()->IASetVertexBuffers(0, 1, &(cur->vertex_buffer), &(cur->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(cur->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		cur->material->SetMatrixBuffer(cur->constant_buffer);

		//cur->material->SendVectorToShader(XMFLOAT4(float(scene->Light_NoShadows_Count), float(scene->Light_Shadows_Count), 0, 0), 0, ID_PS);

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