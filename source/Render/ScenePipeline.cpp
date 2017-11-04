#include "stdafx.h"
#include "ScenePipeline.h"
#include "Render.h"
#include "Utils\Profiler.h"

using namespace EngineCore;

ScenePipeline::ScenePipeline()
{
	sceneDepth = nullptr;
	sceneDepthDSV = nullptr;
	sceneDepthSRV = nullptr;

	transparencyDepth = nullptr;
	transparencyDepthDSV = nullptr;
	transparencyDepthSRV = nullptr;
	
	rt_OpaqueForward = nullptr;
	rt_AO = nullptr;
	rt_OpaqueDefferedDirect = nullptr;
	rt_OpaqueFinal = nullptr;
	rt_HiZDepth = nullptr;
	rt_TransparentForward = nullptr;
	rt_TransparentPrepass = nullptr;
	rt_FinalLDR = nullptr;
	rt_3DHud = nullptr;
	//rt_LDRandHud = nullptr;

	rt_AvgLum = nullptr;
	rt_AvgLumCurrent = nullptr;

	rt_SSR = nullptr;

	rt_Bloom = nullptr;
	g_Bloom = nullptr;

#ifdef AO_FILTER
	g_AO = nullptr;
#endif

	rt_Antialiased = nullptr;

	sp_OpaqueDepthCopy = nullptr;

	sp_AO = nullptr;
	sp_FinalOpaque = nullptr;
	sp_HDRtoLDR = nullptr;
	//sp_3DHud = nullptr;

	sp_HiZ = nullptr;

	sp_AvgLum = nullptr;
	sp_Bloom = nullptr;

	sp_SSR = nullptr;
	g_SSR = nullptr;

	sp_Antialiased[0] = nullptr;
	sp_Antialiased[1] = nullptr;
	sp_Antialiased[2] = nullptr;

	Materials_Count = 0;

	render_mgr = nullptr;
		
	m_CamMoveBuffer = nullptr;
	m_SharedBuffer = nullptr;
	m_AOBuffer = nullptr;

	ZeroMemory(&lightsIDs, sizeof(LightsIDs));

	lightsPerTileCount = nullptr;
	ZeroMemory(&lightsCount, sizeof(lightsCount));

	codemgr = nullptr;
	
	current_camera = nullptr;

	defferedOpaqueCompute = nullptr;
	defferedConfigBuffer = nullptr;
}

ScenePipeline::~ScenePipeline()
{
	Close();
}

void ScenePipeline::Close()
{
	if(camera.sys)
		camera.sys->Deactivate(camera.e, this);

	CloseRts();
	CloseAvgRt();
	
	TEXTURE_DROP(textureIBLLUT);

	_DELETE(defferedOpaqueCompute);
	_RELEASE(defferedConfigBuffer);

	lightSpotBuffer.Release();
	lightDiskBuffer.Release();
	lightRectBuffer.Release();
	lightPointBuffer.Release();
	lightSphereBuffer.Release();
	lightTubeBuffer.Release();
	lightDirBuffer.Release();

	casterSpotBuffer.Release();
	casterDiskBuffer.Release();
	casterRectBuffer.Release();
	casterPointBuffer.Release();
	casterSphereBuffer.Release();
	casterTubeBuffer.Release();

	lightsPerTile.Release();

	_RELEASE(lightsPerTileCount);

	m_LightShadowStructBuffer.Release();

	m_EnvProbBuffer.Release();
	m_MaterialBuffer.Release();

	_RELEASE(m_SharedBuffer);
	_RELEASE(m_CamMoveBuffer);
	_RELEASE(m_AOBuffer);

	_DELETE(render_mgr);
}

void ScenePipeline::CloseAvgRt()
{
	_CLOSE(rt_AvgLum);
	_CLOSE(rt_AvgLumCurrent);
	_DELETE(sp_AvgLum);
}

void ScenePipeline::CloseRts()
{
	_RELEASE(sceneDepth);
	_RELEASE(sceneDepthDSV);
	_RELEASE(sceneDepthSRV);

	_RELEASE(transparencyDepth);
	_RELEASE(transparencyDepthDSV);
	_RELEASE(transparencyDepthSRV);
	
	_CLOSE(rt_OpaqueForward);
	_CLOSE(rt_TransparentForward);
	_CLOSE(rt_TransparentPrepass);
	_CLOSE(rt_AO);
	_CLOSE(rt_OpaqueDefferedDirect);
	_CLOSE(rt_OpaqueFinal);
	_CLOSE(rt_HiZDepth);
	_CLOSE(rt_FinalLDR);
	_CLOSE(rt_3DHud);
	//_CLOSE(rt_LDRandHud);

	_CLOSE(rt_SSR);
	
	_CLOSE(rt_Bloom);
	_CLOSE(g_Bloom);
#ifdef AO_FILTER
	_CLOSE(g_AO);
#endif

	_CLOSE(rt_Antialiased);

	_DELETE(sp_OpaqueDepthCopy);

	_DELETE(sp_AO);
	_DELETE(sp_FinalOpaque);
	_DELETE(sp_HDRtoLDR);

	_DELETE(sp_HiZ);

	_DELETE(sp_Bloom);

	_DELETE(sp_SSR);
	_CLOSE(g_SSR);

	_DELETE(sp_Antialiased[0]);
	_DELETE(sp_Antialiased[1]);
	_DELETE(sp_Antialiased[2]);
}

bool ScenePipeline::Init(int t_width, int t_height, bool lightweight)
{
	isLightweight = lightweight;

	codemgr = ShaderCodeMgr::Get();
	
	render_mgr = new SceneRenderMgr(isLightweight);
	
	m_SharedBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(SharedBuffer), true);

	m_CamMoveBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(XMMATRIX), true);
		
	if(!isLightweight)
	{
		lightSpotBuffer = Buffer::CreateStructedBuffer(Render::Device(), LIGHT_SPOT_FRAME_MAX, sizeof(SpotLightBuffer), true);
		lightDiskBuffer = Buffer::CreateStructedBuffer(Render::Device(), LIGHT_SPOT_DISK_FRAME_MAX, sizeof(DiskLightBuffer), true);
		lightRectBuffer = Buffer::CreateStructedBuffer(Render::Device(), LIGHT_SPOT_RECT_FRAME_MAX, sizeof(RectLightBuffer), true);
		lightPointBuffer = Buffer::CreateStructedBuffer(Render::Device(), LIGHT_POINT_FRAME_MAX, sizeof(PointLightBuffer), true);
		lightSphereBuffer = Buffer::CreateStructedBuffer(Render::Device(), LIGHT_POINT_SPHERE_FRAME_MAX, sizeof(SphereLightBuffer), true);
		lightTubeBuffer = Buffer::CreateStructedBuffer(Render::Device(), LIGHT_POINT_TUBE_FRAME_MAX, sizeof(TubeLightBuffer), true);
		lightDirBuffer = Buffer::CreateStructedBuffer(Render::Device(), LIGHT_DIR_FRAME_MAX, sizeof(DirLightBuffer), true);

		casterSpotBuffer = Buffer::CreateStructedBuffer(Render::Device(), CASTER_SPOT_FRAME_MAX, sizeof(SpotCasterBuffer), true);
		casterDiskBuffer = Buffer::CreateStructedBuffer(Render::Device(), CASTER_SPOT_DISK_FRAME_MAX, sizeof(DiskCasterBuffer), true);
		casterRectBuffer = Buffer::CreateStructedBuffer(Render::Device(), CASTER_SPOT_RECT_FRAME_MAX, sizeof(RectCasterBuffer), true);
		casterPointBuffer = Buffer::CreateStructedBuffer(Render::Device(), CASTER_POINT_FRAME_MAX, sizeof(PointCasterBuffer), true);
		casterSphereBuffer = Buffer::CreateStructedBuffer(Render::Device(), CASTER_POINT_SPHERE_FRAME_MAX, sizeof(SphereCasterBuffer), true);
		casterTubeBuffer = Buffer::CreateStructedBuffer(Render::Device(), CASTER_POINT_TUBE_FRAME_MAX, sizeof(TubeCasterBuffer), true);

		lightsPerTile = Buffer::CreateStructedBuffer(Render::Device(), TOTAL_LIGHT_COUNT, sizeof(int32_t), true);

		lightsPerTileCount = Buffer::CreateConstantBuffer(Render::Device(), sizeof(LightsCount), true);
	}

	m_MaterialBuffer = Buffer::CreateStructedBuffer(Render::Device(), MATERIALS_COUNT, sizeof(MaterialParamsStructBuffer), true);
	Materials[0].unlit = 0;
	Materials[0].ior = 0.0f;
	Materials[0].asymmetry = 0.0f;
	Materials[0].attenuation = 0.0f;
	
	if(!isLightweight)
		defferedOpaqueCompute = new Compute( SHADER_DEFFERED_OPAQUE_FULL );
	else
		defferedOpaqueCompute = new Compute( SHADER_DEFFERED_OPAQUE_IBL );

	defferedConfigBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(DefferedConfigData), true);
	ZeroMemory(&defferedConfigData, sizeof(DefferedConfigData));

	textureIBLLUT = TEXTURE(string(TEX_PBSENVLUT));

	if(!InitAvgRt())
		return false;
	
	if(!Resize(t_width, t_height))
		return false;

	sp_HDRtoLDR->SetFloat(0.0f, 12);

	return true;
}

void ScenePipeline::SetCamera(CameraLink cam)
{
	camera = cam;
	if(!camera.sys)
		return;
	current_camera = camera.Get();
	camera.sys->SetAspect(camera.e, float(width)/float(height));
}

bool ScenePipeline::Resize(int t_width, int t_height)
{
	CloseRts();

	width = t_width;
	height = t_height;
	
	width_pow2 = (int32_t)pow(2.0f, ceil(log(float(width)) / log(2.0f)));
	height_pow2 = (int32_t)pow(2.0f, ceil(log(float(height)) / log(2.0f)));

	sharedconst.screenW = width;
	sharedconst.screenH = height;
	sharedconst.PixSize.x = 1.0f/width;
	sharedconst.PixSize.y = 1.0f/height;

	sharedconst.uvCorrectionForPow2 = Vector2(float(width) / width_pow2, float(height) / height_pow2);

	sharedconst.maxScreenCoords.x = float(width - 1);
	sharedconst.maxScreenCoords.y = float(height - 1);
	sharedconst.maxScreenCoords.z = 1.0f / sharedconst.maxScreenCoords.x;
	sharedconst.maxScreenCoords.w = 1.0f / sharedconst.maxScreenCoords.y;

	if(camera.sys)
		camera.sys->SetAspect(camera.e, float(width)/float(height));

	if(!InitRts())
		return false;
	
	if(rt_HiZDepth)
		sharedconst.mipCount = (float)rt_HiZDepth->mipRes[0].mipCount - 1.0f;
	else
		sharedconst.mipCount = 0.0f;

	ApplyConfig();

	return true;
}

bool ScenePipeline::InitDepth()
{
	D3D11_TEXTURE2D_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Width = width;
	bufferDesc.Height = height;
	bufferDesc.MipLevels = 1;
	bufferDesc.ArraySize = 1;
	bufferDesc.Format = DXGI_FORMAT_R32_TYPELESS; //DXGI_FORMAT_D32_FLOAT
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture2D(&bufferDesc, NULL, &sceneDepth)) )
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MipLevels = -1;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;	
	if( FAILED(Render::CreateShaderResourceView(sceneDepth, &shaderResourceViewDesc, &sceneDepthSRV)) )
		return false;

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	if( FAILED(Render::CreateDepthStencilView(sceneDepth, &depthStencilViewDesc, &sceneDepthDSV)) )
		return false;

	if( FAILED(Render::CreateTexture2D(&bufferDesc, NULL, &transparencyDepth)) )
		return false;
	if( FAILED(Render::CreateShaderResourceView(transparencyDepth, &shaderResourceViewDesc, &transparencyDepthSRV)) )
		return false;
	if( FAILED(Render::CreateDepthStencilView(transparencyDepth, &depthStencilViewDesc, &transparencyDepthDSV)) )
		return false;

	return true;
}

bool ScenePipeline::InitAvgRt()
{
	rt_AvgLum = new RenderTarget;
	if(!rt_AvgLum->Init(1, 1))return false;
	if(!rt_AvgLum->AddRT(DXGI_FORMAT_R32_FLOAT))return false;

	rt_AvgLumCurrent = new RenderTarget;
	if(!rt_AvgLumCurrent->Init(1, 1))return false;
	if(!rt_AvgLumCurrent->AddRT(DXGI_FORMAT_R32_UINT, 1, true))return false;
	rt_AvgLumCurrent->ClearRenderTargets(0.5,0.5,0.5,0.5);

	if(!renderConfig.cameraAdoptEnable)
	{
		rt_AvgLum->ClearRenderTargets(renderConfig.cameraConstExposure,renderConfig.cameraConstExposure,renderConfig.cameraConstExposure,renderConfig.cameraConstExposure);
		rt_AvgLumCurrent->ClearRenderTargets(renderConfig.cameraConstExposure,renderConfig.cameraConstExposure,renderConfig.cameraConstExposure,renderConfig.cameraConstExposure);
	}

	sp_AvgLum = new ScreenPlane(SP_MATERIAL_AVGLUM);
	sp_AvgLum->SetFloat(CONFIG(float, hdr_adopt_speed), 1);
	sp_AvgLum->SetFloat(CONFIG(float, hdr_limit_min), 2);
	sp_AvgLum->SetFloat(CONFIG(float, hdr_limit_max), 3);

	return true;
}

bool ScenePipeline::InitRts()
{
	int32_t t_width_half = width / 2;
	int32_t t_height_half = height / 2;

	if(!InitDepth())
		return false;

	rt_OpaqueForward = new RenderTarget;
	if(!rt_OpaqueForward->Init(width, height, sceneDepthDSV))return false;
	if(!rt_OpaqueForward->AddRT(DXGI_FORMAT_R8G8B8A8_UNORM))return false; // albedo roughnessY
	if(!rt_OpaqueForward->AddRT(DXGI_FORMAT_R32G32B32A32_FLOAT))return false; // rgb - normal, a - tangent
	if(!rt_OpaqueForward->AddRT(DXGI_FORMAT_R16G16_FLOAT))return false; // vertex normal xy 
	if(!rt_OpaqueForward->AddRT(DXGI_FORMAT_R8G8B8A8_UNORM))return false; // spec roughnessX
	if(!rt_OpaqueForward->AddRT(DXGI_FORMAT_R16G16B16A16_FLOAT))return false; // emissive(32 bit sky?), a - vertex normal z
	if(!rt_OpaqueForward->AddRT(DXGI_FORMAT_R32_UINT))return false; // 16 bits - mat id, 16 bits - obj id	
	if(!rt_OpaqueForward->AddRT(DXGI_FORMAT_R8G8B8A8_UNORM))return false; // subsurface, thickness	
	if(!rt_OpaqueForward->AddRT(DXGI_FORMAT_R8_UNORM, 0))return false; // ao
	// 296b per pixel + 32b depth

	rt_HiZDepth = new RenderTarget;
	if(!rt_HiZDepth->Init(width_pow2, height_pow2))return false;
	rt_HiZDepth->SetMipmappingMaterial(SP_MATERIAL_HIZ_DEPTH);
	if(!rt_HiZDepth->AddRT(DXGI_FORMAT_R32G32_FLOAT, 0))return false;

	rt_AO = new RenderTarget;
	if(!rt_AO->Init(width, height))return false; // TODO
	if(!rt_AO->AddRT(DXGI_FORMAT_R8_UNORM))return false; // AO

#ifdef AO_FILTER
	g_AO = new GaussianBlur;
	if(!g_AO->Init(width, height, DXGI_FORMAT_R8_UNORM, GB_KERNEL7, GB_ONLYMIP0, true))return false;
#endif

	rt_SSR = new RenderTarget;
	if(!rt_SSR->Init(width, height))return false;
	if(!rt_SSR->AddRT(DXGI_FORMAT_R16G16B16A16_FLOAT))return false;
	
	g_SSR = new GaussianBlur;
	if(!g_SSR->Init(width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, GB_KERNEL3, GB_ONLYMIP0, true))return false;

	// OPAQUE
	rt_OpaqueDefferedDirect = new RenderTarget;
	if(!rt_OpaqueDefferedDirect->Init(width, height))return false;
	if(!rt_OpaqueDefferedDirect->AddRT(DXGI_FORMAT_R32G32B32A32_FLOAT, 0, true))return false; // diffuse, second specular
	if(!rt_OpaqueDefferedDirect->AddRT(DXGI_FORMAT_R32G32B32A32_FLOAT, 1, true))return false; // specular, second specular
	if(!rt_OpaqueDefferedDirect->AddRT(DXGI_FORMAT_R32G32_FLOAT, 1, true))return false; // second specular

	rt_OpaqueFinal = new RenderTarget;
	if(!rt_OpaqueFinal->Init(width, height))return false;
	rt_OpaqueFinal->SetMipmappingMaterial(SP_MATERIAL_OPAQUE_BLUR);
	if(!rt_OpaqueFinal->AddRT(DXGI_FORMAT_R32G32B32A32_FLOAT, 0))return false;
	if(!rt_OpaqueFinal->AddRT(DXGI_FORMAT_R16G16B16A16_FLOAT, 0))return false;

	// TRANSPARENT	
	rt_TransparentForward = new RenderTarget;
	if(!rt_TransparentForward->Init(width, height, sceneDepthDSV))return false;
	if(!rt_TransparentForward->AddRT(DXGI_FORMAT_R32G32B32A32_FLOAT))return false;

	rt_TransparentPrepass = new RenderTarget;
	if(!rt_TransparentPrepass->Init(width, height, transparencyDepthDSV))return false;
	if(!rt_TransparentPrepass->AddRT(DXGI_FORMAT_R16G16B16A16_FLOAT))return false;

	// FINAL
	rt_Bloom = new RenderTarget;
	if(!rt_Bloom->Init(t_width_half, t_height_half))return false;
	//rt_Bloom->SetMipmappingMaterial(SP_MATERIAL_BLOOM_MIPS);
	if(!rt_Bloom->AddRT(DXGI_FORMAT_R16G16B16A16_FLOAT, 0, false, true))return false;

	g_Bloom = new GaussianBlur;
	if(!g_Bloom->Init(t_width_half, t_height_half, DXGI_FORMAT_R16G16B16A16_FLOAT, GB_KERNEL21, GB_ALLMIPS))return false;

	rt_FinalLDR = new RenderTarget;
	if(!rt_FinalLDR->Init(width, height))return false;
	if(!rt_FinalLDR->AddRT(DXGI_FORMAT_R8G8B8A8_UNORM))return false;

	rt_Antialiased = new RenderTarget;
	if(!rt_Antialiased->Init(width, height))return false;
	if(!rt_Antialiased->AddRT(DXGI_FORMAT_R8G8B8A8_UNORM))return false; // blend
	if(!rt_Antialiased->AddRT(DXGI_FORMAT_R8G8B8A8_UNORM))return false; // edge & result

	/*rt_LDRandHud = new RenderTarget;
	if(!rt_LDRandHud->Init(width, height))return false;
	if(!rt_LDRandHud->AddRT(DXGI_FORMAT_R8G8B8A8_UNORM))return false;*/

	rt_3DHud = new RenderTarget;
	if(!rt_3DHud->Init(width, height, sceneDepthDSV))return false;
	if(!rt_3DHud->AddRT(DXGI_FORMAT_R8G8B8A8_UNORM))return false;
	
	sp_OpaqueDepthCopy = new ScreenPlane(SP_MATERIAL_DEPTH_OPAC_DIR);
	sp_OpaqueDepthCopy->SetTexture(sceneDepthSRV, 0);

	sp_SSR = new ScreenPlane(SP_SHADER_SSR);
	sp_SSR->SetTexture(rt_OpaqueForward->GetShaderResourceView(1), 0);
	sp_SSR->SetTexture(rt_OpaqueForward->GetShaderResourceView(3), 1);
	sp_SSR->SetTexture(rt_OpaqueForward->GetShaderResourceView(0), 2);
	sp_SSR->SetTexture(rt_OpaqueFinal->GetShaderResourceView(1), 3);
	sp_SSR->SetTexture(rt_HiZDepth->GetShaderResourceView(0), 4);
	sp_SSR->SetTexture(rt_OpaqueForward->GetShaderResourceView(5), 5);
	sp_SSR->SetTexture(rt_OpaqueForward->GetShaderResourceView(2), 6);
	sp_SSR->SetTexture(rt_OpaqueForward->GetShaderResourceView(4), 7);

	sp_AO = new ScreenPlane(SP_MATERIAL_AO);
	sp_AO->SetTextureByNameS(TEX_HBAO_DITHER, 0);
	sp_AO->SetTexture(rt_OpaqueForward->GetShaderResourceView(1), 1);
	sp_AO->SetTexture(rt_HiZDepth->GetShaderResourceView(0), 2);
	sp_AO->SetFloat(CONFIG(float, ao_half_sample_radius), 0);
	sp_AO->SetFloat(CONFIG(float, ao_inv_distance_falloff), 1);
	sp_AO->SetFloat(CONFIG(float, ao_hieght_bias), 2);
	sp_AO->SetFloat(CONFIG(float, ao_hiz_mip_scaler), 3);
	sp_AO->SetFloat(CONFIG(float, ao_max_dist_sqr), 4);
	
	sp_FinalOpaque = new ScreenPlane(SP_MATERIAL_COMBINE);
	sp_FinalOpaque->SetTexture(rt_OpaqueDefferedDirect->GetShaderResourceView(0), 0);
	sp_FinalOpaque->SetTexture(rt_OpaqueDefferedDirect->GetShaderResourceView(1), 1);
	sp_FinalOpaque->SetTexture(rt_OpaqueDefferedDirect->GetShaderResourceView(2), 2);
	//sp_FinalOpaque->SetTexture(rt_OpaqueForward->GetShaderResourceView(5), 3);
	sp_FinalOpaque->SetTexture(rt_HiZDepth->GetShaderResourceView(0), 3);

	sp_AvgLum->SetTexture(rt_OpaqueFinal->GetShaderResourceView(1), 0);
	sp_AvgLum->SetFloat(float(rt_OpaqueFinal->GetMipsCountInFullChain() - 2), 0);

	sp_Bloom = new ScreenPlane(SP_MATERIAL_BLOOM_FIND);
	sp_Bloom->SetTexture(rt_OpaqueFinal->GetShaderResourceView(0), 0);
	sp_Bloom->SetTexture(rt_AvgLum->GetShaderResourceView(0), 1);
	sp_Bloom->SetFloat(CONFIG(float, hdr_bloom_threshold), 0);
	sp_Bloom->SetFloat(CONFIG(float, hdr_bloom_max), 1);

	sp_HDRtoLDR = new ScreenPlane(SP_MATERIAL_HDR);
	sp_HDRtoLDR->SetTexture(rt_OpaqueFinal->GetShaderResourceView(0), 0);
	sp_HDRtoLDR->SetTexture(rt_TransparentForward->GetShaderResourceView(0), 1);
	sp_HDRtoLDR->SetTexture(rt_3DHud->GetShaderResourceView(0), 2);
	sp_HDRtoLDR->SetTexture(rt_AvgLum->GetShaderResourceView(0), 3);
	sp_HDRtoLDR->SetTexture(rt_Bloom->GetShaderResourceView(0), 4);
	sp_HDRtoLDR->SetTexture(rt_OpaqueForward->GetShaderResourceView(0), 5);
	sp_HDRtoLDR->SetTexture(rt_OpaqueForward->GetShaderResourceView(1), 6);
	sp_HDRtoLDR->SetTexture(rt_OpaqueForward->GetShaderResourceView(3), 7);
	sp_HDRtoLDR->SetTexture(rt_OpaqueForward->GetShaderResourceView(4), 8);
	sp_HDRtoLDR->SetTexture(rt_OpaqueForward->GetShaderResourceView(6), 9);
	sp_HDRtoLDR->SetTexture(rt_OpaqueForward->GetShaderResourceView(7), 10);
	sp_HDRtoLDR->SetTexture(rt_AO->GetShaderResourceView(0), 11);
	sp_HDRtoLDR->SetTexture(rt_HiZDepth->GetShaderResourceView(0), 12);
	// debug
	sp_HDRtoLDR->SetTexture(rt_SSR->GetShaderResourceView(0), 13);

	if(!isLightweight)
	{
		sp_HDRtoLDR->SetTexture(render_mgr->voxelRenderer->GetVoxelSRV(), 14);
		sp_HDRtoLDR->SetTexture(render_mgr->voxelRenderer->GetVoxelColor0SRV(), 15);
		sp_HDRtoLDR->SetTexture(render_mgr->voxelRenderer->GetVoxelColor1SRV(), 16);
		sp_HDRtoLDR->SetTexture(render_mgr->voxelRenderer->GetVoxelNormalSRV(), 17);
		sp_HDRtoLDR->SetTexture(render_mgr->voxelRenderer->GetVoxelEmittanceSRV(), 18);
	}

	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_shoulder_strength), 0);
	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_linear_strength), 1);
	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_linear_angle), 2);
	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_toe_strength), 3);
	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_toe_numerator), 4);
	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_toe_denominator), 5);
	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_white_point), 6);
	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_exposure_adjustment), 7);
	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_apply_to_lum) ? 1.0f : 0.0f, 8);
	sp_HDRtoLDR->SetFloat(CONFIG(float, tonemap_middle_gray), 9);
	sp_HDRtoLDR->SetFloat(CONFIG(float, hdr_bloom_amount), 10);
	sp_HDRtoLDR->SetFloat(CONFIG(float, hdr_bloom_mul), 11);

	sp_Antialiased[0] = new ScreenPlane(SP_MATERIAL_AA_EDGE);
	sp_Antialiased[0]->SetTexture(rt_Antialiased->GetShaderResourceView(0), 0);
	sp_Antialiased[0]->SetFloat(CONFIG(float, smaa_threshold), 0);

	sp_Antialiased[1] = new ScreenPlane(SP_MATERIAL_AA_BLEND);
	sp_Antialiased[1]->SetTextureByNameS(TEX_SMAA_AREA, 0);
	sp_Antialiased[1]->SetTextureByNameS(TEX_SMAA_SEARCH, 1);
	sp_Antialiased[1]->SetTexture(rt_Antialiased->GetShaderResourceView(1), 2);
	sp_Antialiased[1]->SetFloat(CONFIG(float, smaa_search_steps), 0);
	sp_Antialiased[1]->SetFloat(CONFIG(float, smaa_search_steps_diag), 1);
	sp_Antialiased[1]->SetFloat(CONFIG(float, smaa_corner_rounding), 2);
	sp_Antialiased[1]->SetFloat(0, 3);// sub indecies used for temporal aa 
	sp_Antialiased[1]->SetFloat(0, 4);
	sp_Antialiased[1]->SetFloat(0, 5);
	sp_Antialiased[1]->SetFloat(0, 6);

	sp_Antialiased[2] = new ScreenPlane(SP_MATERIAL_AA);
	sp_Antialiased[2]->SetTexture(rt_FinalLDR->GetShaderResourceView(0), 0);
	sp_Antialiased[2]->SetTexture(rt_Antialiased->GetShaderResourceView(0), 1);

	/*sp_3DHud = new ScreenPlane;
	if(!sp_3DHud->Init(width, height, SP_MATERIAL_3DHUD, t_ortho))return false;
	sp_3DHud->AddTex(rt_Antialiased->GetShaderResourceView(1));
	sp_3DHud->AddTex(rt_3DHud->GetShaderResourceView(0));	*/

	sp_HiZ = new ScreenPlane(SP_MATERIAL_HIZ_DEPTH);

	return true;
}

bool ScenePipeline::ApplyConfig()
{
	if(!renderConfig._dirty)
		return renderConfig.active;

	if(!renderConfig.editorGuiEnable)
		rt_3DHud->ClearRenderTargets(false);

	if(!renderConfig.cameraAdoptEnable)
	{
		rt_AvgLum->ClearRenderTargets(renderConfig.cameraConstExposure,renderConfig.cameraConstExposure,renderConfig.cameraConstExposure,renderConfig.cameraConstExposure);
		rt_AvgLumCurrent->ClearRenderTargets(renderConfig.cameraConstExposure,renderConfig.cameraConstExposure,renderConfig.cameraConstExposure,renderConfig.cameraConstExposure);
	}

	sp_HDRtoLDR->SetFloat((float)renderConfig.bufferViewMode, 12);
	sp_HDRtoLDR->SetFloat((float)renderConfig.voxelViewMode, 13);
	sp_HDRtoLDR->SetFloat((float)renderConfig.voxelCascade, 14);

	if(!renderConfig.bloomEnable)
		rt_Bloom->ClearRenderTargets();

	renderConfig._dirty = false;
	return renderConfig.active;
}

bool ScenePipeline::StartFrame(LocalTimer* timer)
{
	if(!camera.sys)
		return false;

	if(!ApplyConfig())
		return false;

	sharedconst.time = timer->GetTime();
	sharedconst.dt = timer->dt();

	current_camera = camera.Get();
	XMStoreFloat3(&sharedconst.CamDir, current_camera->camLookDir);
	XMStoreFloat3(&sharedconst.CamPos, current_camera->camPos);
	XMStoreFloat3(&sharedconst.CamTangent, XMVector3Normalize(XMVector3Cross(current_camera->camLookDir, current_camera->camUp)));
	XMStoreFloat3(&sharedconst.CamBinormal, current_camera->camUp);

	sharedconst.view = XMMatrixTranspose(current_camera->viewMatrix);
	sharedconst.projection = XMMatrixTranspose(current_camera->projMatrix);
	sharedconst.viewProjection = sharedconst.projection * sharedconst.view;
	sharedconst.invViewProjection = XMMatrixInverse(nullptr, sharedconst.viewProjection);

	sharedconst.g_far = current_camera->far_clip;
	sharedconst.g_farMinusNear = current_camera->far_clip - current_camera->near_clip;
	sharedconst.g_nearMulFar = current_camera->far_clip * current_camera->near_clip;

	// frustum vectors
	XMMATRIX invView = XMMatrixInverse(nullptr, current_camera->viewMatrix);
	XMMATRIX invProj = XMMatrixInverse(nullptr, current_camera->projMatrix);

	XMVECTOR camCorners[4];
	camCorners[0] = XMVectorSet(-1.0f, 1.0f, current_camera->near_clip, 1.0f);
	camCorners[1] = XMVectorSet(1.0f, 1.0f, current_camera->near_clip, 1.0f);
	camCorners[2] = XMVectorSet(-1.0f, -1.0f, current_camera->near_clip, 1.0f);
	camCorners[3] = XMVectorSet(1.0f, -1.0f, current_camera->near_clip, 1.0f);

	for(uint8_t i = 0; i < 4; i++)
	{
		camCorners[i] = XMVector3Normalize(XMVector3TransformCoord(camCorners[i], invProj));
		camCorners[i] = XMVectorSetW(camCorners[i], 0.0f);
		camCorners[i] = XMVector4Transform(camCorners[i], invView);
	}

	XMStoreFloat3(&sharedconst.g_CamFrust0, camCorners[0]);
	XMStoreFloat3(&sharedconst.g_CamFrust1, camCorners[1]);
	XMStoreFloat3(&sharedconst.g_CamFrust2, camCorners[2]);
	XMStoreFloat3(&sharedconst.g_CamFrust3, camCorners[3]);
	// frustum vectors

	sharedconst.perspParam = 0.5f * (sharedconst.projection.r[1].m128_f32[1] + sharedconst.projection.r[2].m128_f32[2]);

	Render::UpdateDynamicResource(m_SharedBuffer, (void*)&sharedconst, sizeof(sharedconst));
	Render::PSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	Render::VSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	Render::HSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	Render::DSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	Render::GSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	
	// remove
	float projParam = 0.5f * (sharedconst.projection.r[1].m128_f32[1] + sharedconst.projection.r[2].m128_f32[2]);
	sp_AO->SetFloat(projParam, 5);
/*#if AO_TYPE == 0
	float maxDistSqr = (CONFIG(ao_half_sample_radius) * sharedconst.screenW * projParam * 0.5f - sharedconst.projection.r[3].m128_f32[3]) / sharedconst.projection.r[3].m128_f32[2];
	maxDistSqr *= maxDistSqr;
	sp_AO->SetFloat(maxDistSqr, 4);
#endif*/

	XMMATRIX camMove = XMMatrixTranspose(current_camera->prevViewProj) * sharedconst.invViewProjection;
	Render::UpdateDynamicResource(m_CamMoveBuffer, (void*)&camMove, sizeof(XMMATRIX));
	
	// TEMP
	defferedConfigData.dirDiff = renderConfig.analyticLightDiffuse;
	defferedConfigData.dirSpec = renderConfig.analyticLightSpecular;
	defferedConfigData.indirDiff = renderConfig.ambientLightDiffuse;
	defferedConfigData.indirSpec = renderConfig.ambientLightSpecular;

	defferedConfigData.isLightweight = isLightweight ? 1.0f : 0.0f;
	Render::UpdateDynamicResource(defferedConfigBuffer, &defferedConfigData, sizeof(DefferedConfigData));

	return true;
}

void ScenePipeline::EndFrame()
{
	render_mgr->ClearAll();
	current_camera = nullptr;
}

bool ScenePipeline::UIStage()
{
	if(!renderConfig.editorGuiEnable)
		return false;

	rt_3DHud->ClearRenderTargets(false);
	rt_3DHud->SetRenderTarget();

	render_mgr->DrawHud();
	return true;
}

void ScenePipeline::UIOverlayStage()
{
	if(!renderConfig.editorGuiEnable) return;
	
	rt_3DHud->SetRenderTarget();
	Render::ClearDepthStencilView(rt_3DHud->m_DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	render_mgr->DrawOvHud();
}

void ScenePipeline::OpaqueForwardStage()
{
	if(!isLightweight)
	{
		PERF_CPU_BEGIN(_VOXELIZATION);
		PERF_GPU_TIMESTAMP(_VOXELIZATION);

		render_mgr->voxelRenderer->VoxelizeScene();
		PERF_CPU_END(_VOXELIZATION);
	
		PERF_CPU_BEGIN(_VOXELLIGHT);
		PERF_GPU_TIMESTAMP(_VOXELLIGHT);

		render_mgr->voxelRenderer->ProcessEmittance();
		PERF_CPU_END(_VOXELLIGHT);
	}

	PERF_GPU_TIMESTAMP(_GEOMETRY);

	rt_OpaqueForward->ClearRenderTargets();
	rt_OpaqueForward->SetRenderTarget();
	Materials_Count = 1;

	render_mgr->DrawOpaque(this);
	render_mgr->DrawAlphatest(this);

	PERF_GPU_TIMESTAMP(_DEPTH_COPY);

	//ID3D11Resource* hiz_topmip = nullptr;
	//rt_HiZDepth->GetRenderTargetView(0)->GetResource(&hiz_topmip);
	//CONTEXT->CopyResource(hiz_topmip, sceneDepth);
	rt_HiZDepth->SetRenderTarget();
	sp_OpaqueDepthCopy->Draw();
}

void ScenePipeline::TransparentForwardStage()
{
	// PREPASS
	rt_TransparentPrepass->ClearRenderTargets(true);
	rt_TransparentPrepass->SetRenderTarget();

	render_mgr->PrepassTransparent(this);

	Render::OMSetRenderTargets(0, nullptr, nullptr);

	// RENDER 
	rt_TransparentForward->ClearRenderTargets(false);
	rt_TransparentForward->SetRenderTarget();

	const uint32_t srvs_size = 8;
	ID3D11ShaderResourceView* srvs[srvs_size];
	srvs[0] = TEXTURE_GETPTR(textureIBLLUT);
	srvs[1] = nullptr;
	srvs[2] = nullptr;
	srvs[3] = nullptr;
	srvs[4] = nullptr;
	srvs[5] = rt_OpaqueFinal->GetShaderResourceView(0);
	srvs[6] = transparencyDepthSRV;
	srvs[7] = rt_TransparentPrepass->GetShaderResourceView(0);
				
	auto& distProb = render_mgr->GetDistEnvProb();
	if(distProb.mipsCount != 0)
	{
		srvs[1] = distProb.specCube;
		srvs[2] = distProb.diffCube;
	}

	if(!isLightweight)
	{
		srvs[3] = render_mgr->shadowsRenderer->GetShadowBuffer();
		srvs[4] = render_mgr->voxelRenderer->GetVoxelEmittanceSRV();
	}
		
	Render::PSSetShaderResources(0, srvs_size, srvs);

	if(!isLightweight)
		LoadLights(srvs_size, false);
	
	Render::PSSetConstantBuffers(3, 1, &defferedConfigBuffer); 

	if(!isLightweight)
	{
		Render::PSSetShaderResources(srvs_size + 13, 1, &lightsPerTile.srv); 

		Render::PSSetConstantBuffers(4, 1, &lightsPerTileCount); 

		auto volumeBuffer = render_mgr->voxelRenderer->GetVolumeBuffer();
		Render::PSSetConstantBuffers(5, 1, &volumeBuffer); 

		volumeBuffer = render_mgr->voxelRenderer->GetVolumeTraceBuffer();
		Render::PSSetConstantBuffers(6, 1, &volumeBuffer); 
	}

	render_mgr->DrawTransparent(this);
}

uint8_t ScenePipeline::LoadLights(uint8_t startOffset, bool isCS)
{
	size_t spot_size, disk_size, rect_size, point_size, sphere_size, tube_size, dir_size;
	void* spot_data = (void*)render_mgr->GetSpotLightDataPtr(&spot_size);
	void* disk_data = (void*)render_mgr->GetSpotLightDiskDataPtr(&disk_size);
	void* rect_data = (void*)render_mgr->GetSpotLightRectDataPtr(&rect_size);
	void* point_data = (void*)render_mgr->GetPointLightDataPtr(&point_size);
	void* sphere_data = (void*)render_mgr->GetPointLightSphereDataPtr(&sphere_size);
	void* tube_data = (void*)render_mgr->GetPointLightTubeDataPtr(&tube_size);
	void* dir_data = (void*)render_mgr->GetDirLightDataPtr(&dir_size);

	size_t caster_spot_size, caster_disk_size, caster_rect_size, caster_point_size, caster_sphere_size, caster_tube_size;
	void* caster_spot_data = (void*)render_mgr->GetSpotCasterDataPtr(&caster_spot_size);
	void* caster_disk_data = (void*)render_mgr->GetSpotCasterDiskDataPtr(&caster_disk_size);
	void* caster_rect_data = (void*)render_mgr->GetSpotCasterRectDataPtr(&caster_rect_size);
	void* caster_point_data = (void*)render_mgr->GetPointCasterDataPtr(&caster_point_size);
	void* caster_sphere_data = (void*)render_mgr->GetPointCasterSphereDataPtr(&caster_sphere_size);
	void* caster_tube_data = (void*)render_mgr->GetPointCasterTubeDataPtr(&caster_tube_size);

	uint8_t structed_offset = startOffset;

	ID3D11ShaderResourceView* srvs[13];

	if(spot_size > 0)
		Render::UpdateDynamicResource(lightSpotBuffer.buf, spot_data, sizeof(SpotLightBuffer) * spot_size);
	srvs[0] = lightSpotBuffer.srv; 
	structed_offset++;

	if(disk_size > 0)
		Render::UpdateDynamicResource(lightDiskBuffer.buf, disk_data, sizeof(DiskLightBuffer) * disk_size);
	srvs[1] = lightDiskBuffer.srv; 
	structed_offset++;

	if(rect_size > 0)
		Render::UpdateDynamicResource(lightRectBuffer.buf, rect_data, sizeof(RectLightBuffer) * rect_size);
	srvs[2] = lightRectBuffer.srv; 
	structed_offset++;

	if(caster_spot_size > 0)
		Render::UpdateDynamicResource(casterSpotBuffer.buf, caster_spot_data, sizeof(SpotCasterBuffer) * caster_spot_size);
	srvs[3] = casterSpotBuffer.srv; 
	structed_offset++;

	if(caster_disk_size > 0)
		Render::UpdateDynamicResource(casterDiskBuffer.buf, caster_disk_data, sizeof(DiskCasterBuffer) * caster_disk_size);
	srvs[4] = casterDiskBuffer.srv; 
	structed_offset++;

	if(caster_rect_size > 0)
		Render::UpdateDynamicResource(casterRectBuffer.buf, caster_rect_data, sizeof(RectCasterBuffer) * caster_rect_size);
	srvs[5] = casterRectBuffer.srv; 
	structed_offset++;
	
	if(point_size > 0)
		Render::UpdateDynamicResource(lightPointBuffer.buf, point_data, sizeof(PointLightBuffer) * point_size);
	srvs[6] = lightPointBuffer.srv; 
	structed_offset++;
	
	if(sphere_size > 0)
		Render::UpdateDynamicResource(lightSphereBuffer.buf, sphere_data, sizeof(SphereLightBuffer) * sphere_size);
	srvs[7] = lightSphereBuffer.srv; 
	structed_offset++;
	
	if(tube_size > 0)
		Render::UpdateDynamicResource(lightTubeBuffer.buf, tube_data, sizeof(TubeLightBuffer) * tube_size);
	srvs[8] = lightTubeBuffer.srv; 
	structed_offset++;

	if(caster_point_size > 0)
		Render::UpdateDynamicResource(casterPointBuffer.buf, caster_point_data, sizeof(PointCasterBuffer) * caster_point_size);
	srvs[9] = casterPointBuffer.srv; 
	structed_offset++;

	if(caster_sphere_size > 0)
		Render::UpdateDynamicResource(casterSphereBuffer.buf, caster_sphere_data, sizeof(SphereCasterBuffer) * caster_sphere_size);
	srvs[10] = casterSphereBuffer.srv; 
	structed_offset++;

	if(caster_tube_size > 0)
		Render::UpdateDynamicResource(casterTubeBuffer.buf, caster_tube_data, sizeof(TubeCasterBuffer) * caster_tube_size);
	srvs[11] = casterTubeBuffer.srv; 
	structed_offset++;

	if(dir_size > 0)
		Render::UpdateDynamicResource(lightDirBuffer.buf, dir_data, sizeof(DirLightBuffer) * dir_size);
	srvs[12] = lightDirBuffer.srv; 
	structed_offset++;

	if(isCS)
		Render::CSSetShaderResources(startOffset, 13, srvs);
	else
		Render::PSSetShaderResources(startOffset, 13, srvs);

	lightsCount.spot_count = (int32_t)spot_size;
	lightsCount.disk_count = (int32_t)disk_size;
	lightsCount.rect_count = (int32_t)rect_size;
	lightsCount.caster_spot_count = (int32_t)caster_spot_size;
	lightsCount.caster_disk_count = (int32_t)caster_disk_size;
	lightsCount.caster_rect_count = (int32_t)caster_rect_size;
	lightsCount.point_count = (int32_t)point_size;
	lightsCount.sphere_count = (int32_t)sphere_size;
	lightsCount.tube_count = (int32_t)tube_size;
	lightsCount.caster_point_count = (int32_t)caster_point_size;
	lightsCount.caster_sphere_count = (int32_t)caster_sphere_size;
	lightsCount.caster_tube_count = (int32_t)caster_tube_size;
	lightsCount.dir_count = (int32_t)dir_size;
		
		// TEMP
		for(uint16_t i = 0; i < LIGHT_SPOT_FRAME_MAX; i++)
			lightsIDs[SPOT_L_ID(i)] = i;
		for(uint16_t i = 0; i < LIGHT_SPOT_DISK_FRAME_MAX; i++)
			lightsIDs[DISK_L_ID(i)] = i;
		for(uint16_t i = 0; i < LIGHT_SPOT_RECT_FRAME_MAX; i++)
			lightsIDs[RECT_L_ID(i)] = i;
		for(uint16_t i = 0; i < CASTER_SPOT_FRAME_MAX; i++)
			lightsIDs[SPOT_C_ID(i)] = i;
		for(uint16_t i = 0; i < CASTER_SPOT_DISK_FRAME_MAX; i++)
			lightsIDs[DISK_C_ID(i)] = i;
		for(uint16_t i = 0; i < CASTER_SPOT_RECT_FRAME_MAX; i++)
			lightsIDs[RECT_C_ID(i)] = i;
		for(uint16_t i = 0; i < LIGHT_POINT_FRAME_MAX; i++)
			lightsIDs[POINT_L_ID(i)] = i;
		for(uint16_t i = 0; i < LIGHT_POINT_SPHERE_FRAME_MAX; i++)
			lightsIDs[SPHERE_L_ID(i)] = i;
		for(uint16_t i = 0; i < LIGHT_POINT_TUBE_FRAME_MAX; i++)
			lightsIDs[TUBE_L_ID(i)] = i;
		for(uint16_t i = 0; i < CASTER_POINT_FRAME_MAX; i++)
			lightsIDs[POINT_C_ID(i)] = i;
		for(uint16_t i = 0; i < CASTER_POINT_SPHERE_FRAME_MAX; i++)
			lightsIDs[SPHERE_C_ID(i)] = i;
		for(uint16_t i = 0; i < CASTER_POINT_TUBE_FRAME_MAX; i++)
			lightsIDs[TUBE_C_ID(i)] = i;
		for(uint16_t i = 0; i < LIGHT_DIR_FRAME_MAX; i++)
			lightsIDs[DIR_ID(i)] = i;

	Render::UpdateDynamicResource(lightsPerTile.buf, &lightsIDs, sizeof(LightsIDs));

	Render::UpdateDynamicResource(lightsPerTileCount, &lightsCount, sizeof(LightsCount));

	return structed_offset;
}

void ScenePipeline::HiZMips()
{
	auto& miplvl = rt_HiZDepth->mipRes[0];

	for(int j=0; j<miplvl.mipCount-1; j++)
	{
		rt_HiZDepth->m_viewport.Width = float(rt_HiZDepth->mip_res[j].x);
		rt_HiZDepth->m_viewport.Height = float(rt_HiZDepth->mip_res[j].y);
		Render::RSSetViewports(1, &rt_HiZDepth->m_viewport);

		Render::OMSetRenderTargets(1, &miplvl.mip_RTV[j], nullptr);
		
		sp_HiZ->SetTexture(miplvl.mip_SRV[j], 0);
		sp_HiZ->Draw();
	}
	rt_HiZDepth->m_viewport.Width = float(rt_HiZDepth->t_width);
	rt_HiZDepth->m_viewport.Height = float(rt_HiZDepth->t_height);
}

void ScenePipeline::OpaqueDefferedStage()
{	
	PERF_GPU_TIMESTAMP(_HIZ_GEN);
	HiZMips();

	rt_SSR->ClearRenderTargets();
	rt_SSR->SetRenderTarget();

	PERF_GPU_TIMESTAMP(_SSR);
	// mat params
	D3D11_MAPPED_SUBRESOURCE mappedResourceM;
	if(FAILED(Render::Map(m_MaterialBuffer.buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceM)))
		return;
	memcpy(mappedResourceM.pData, (void*)Materials, Materials_Count * sizeof(MaterialParamsStructBuffer));
	Render::Unmap(m_MaterialBuffer.buf, 0);

	Render::PSSetConstantBuffers(1, 1, &m_CamMoveBuffer); 
	Render::PSSetShaderResources(0, 1, &m_MaterialBuffer.srv);

	sp_SSR->Draw();

	g_SSR->blur(rt_SSR, 0, rt_HiZDepth->GetShaderResourceView(0));

	// ao
	PERF_GPU_TIMESTAMP(_AO);

	rt_AO->ClearRenderTargets(1.0f,1.0f,1.0f,1.0f);
	rt_AO->SetRenderTarget();

	Render::PSSetConstantBuffers(1, 1, &m_AOBuffer); 

	sp_AO->Draw();
	
#ifdef AO_FILTER
	g_AO->blur(rt_AO, 0, rt_HiZDepth->GetShaderResourceView(0));
#endif

	PERF_GPU_TIMESTAMP(_OPAQUE_MAIN);
	
	Render::OMUnsetRenderTargets();

	rt_OpaqueDefferedDirect->ClearRenderTargets();

	ID3D11ShaderResourceView* srvs[15];
	srvs[0] = m_MaterialBuffer.srv;
	srvs[1] = rt_OpaqueForward->GetShaderResourceView(0);
	srvs[2] = rt_OpaqueForward->GetShaderResourceView(1);
	srvs[3] = rt_OpaqueForward->GetShaderResourceView(2);
	srvs[4] = rt_OpaqueForward->GetShaderResourceView(3);
	srvs[5] = rt_OpaqueForward->GetShaderResourceView(4);
	srvs[6] = rt_OpaqueForward->GetShaderResourceView(5);
	srvs[7] = rt_OpaqueForward->GetShaderResourceView(6);
	srvs[8] = rt_OpaqueForward->GetShaderResourceView(7);
	srvs[9] = rt_HiZDepth->GetShaderResourceView(0);
	srvs[10] = rt_AO->GetShaderResourceView(0);
	srvs[11] = rt_SSR->GetShaderResourceView(0);
	srvs[12] = TEXTURE_GETPTR(textureIBLLUT);
	srvs[13] = nullptr;
	srvs[14] = nullptr;
				
	auto& distProb = render_mgr->GetDistEnvProb();
	if(distProb.mipsCount != 0)
	{
		srvs[13] = distProb.specCube;
		srvs[14] = distProb.diffCube;
	}
		
	Render::CSSetShaderResources(0, 15, srvs);

	if(!isLightweight)
	{
		auto shadowBuffer = render_mgr->shadowsRenderer->GetShadowBuffer();
		auto volumeEmittance = render_mgr->voxelRenderer->GetVoxelEmittanceSRV();

		Render::CSSetShaderResources(15, 1, &shadowBuffer);
		Render::CSSetShaderResources(16, 1, &volumeEmittance);

		LoadLights(17, true);

		Render::CSSetShaderResources(30, 1, &lightsPerTile.srv); 
	}
	
	Render::CSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	Render::CSSetConstantBuffers(1, 1, &defferedConfigBuffer); 

	if(!isLightweight)
	{
		Render::CSSetConstantBuffers(2, 1, &lightsPerTileCount); 

		auto volumeBuffer = render_mgr->voxelRenderer->GetVolumeBuffer();
		Render::CSSetConstantBuffers(3, 1, &volumeBuffer); 

		volumeBuffer = render_mgr->voxelRenderer->GetVolumeTraceBuffer();
		Render::CSSetConstantBuffers(4, 1, &volumeBuffer); 
	}
		
	defferedOpaqueCompute->BindUAV( rt_OpaqueDefferedDirect->GetUnorderedAccessView(0) );
	defferedOpaqueCompute->BindUAV( rt_OpaqueDefferedDirect->GetUnorderedAccessView(1) );
	defferedOpaqueCompute->BindUAV( rt_OpaqueDefferedDirect->GetUnorderedAccessView(2) );

	//temp
	uint16_t group_count_x = (uint16_t)ceil(float(width) / 8);
	uint16_t group_count_y = (uint16_t)ceil(float(height) / 8);

	defferedOpaqueCompute->Dispatch(group_count_x, group_count_y, 1);
	defferedOpaqueCompute->UnbindUAV();
	
	PERF_GPU_TIMESTAMP(_OPAQUE_FINAL);
	// final 
	rt_OpaqueFinal->ClearRenderTargets();
	rt_OpaqueFinal->SetRenderTarget();
	sp_FinalOpaque->Draw();

	// blur
	rt_OpaqueFinal->GenerateMipmaps(this); // !!!!!!!!!!!!!! TODO: separete blur
	
	PERF_GPU_TIMESTAMP(_HDR_BLOOM);

	if(renderConfig.cameraAdoptEnable)
	{
		// avglum
		rt_AvgLum->ClearRenderTargets();

		ID3D11UnorderedAccessView* UAV = rt_AvgLumCurrent->GetUnorderedAccessView(0);
		ID3D11RenderTargetView* r_target = rt_AvgLum->GetRenderTargetView(0);
		if(!UAV)
			return;

		Render::OMSetRenderTargetsAndUnorderedAccessViews(1, &r_target, nullptr, 1, 1, &UAV, nullptr);
		Render::RSSetViewports(1, &rt_AvgLum->m_viewport);
		sp_AvgLum->Draw();
		r_target = nullptr;
		UAV = nullptr;
		Render::OMSetRenderTargetsAndUnorderedAccessViews(1, &r_target, nullptr, 1, 1, &UAV, nullptr);
	}

	if(renderConfig.bloomEnable)
	{
		// bloom prepare
		rt_Bloom->ClearRenderTargets(); // !!!!!!!!!!!! TODO: only opaque now!
		rt_Bloom->SetRenderTarget();
		sp_Bloom->Draw();

		rt_Bloom->GenerateMipmaps(this);

		g_Bloom->blur(rt_Bloom);
	}
}

void ScenePipeline::HDRtoLDRStage()
{
	PERF_GPU_TIMESTAMP(_COMBINE);
	rt_Antialiased->ClearRenderTargets();

	// combine opaque and transparent, hdr, tonemap
	rt_FinalLDR->ClearRenderTargets();
	rt_FinalLDR->SetRenderTarget();

	ID3D11RenderTargetView* r_target[2];
	r_target[0] = rt_FinalLDR->GetRenderTargetView(0);
	r_target[1] = rt_Antialiased->GetRenderTargetView(0);
	Render::OMSetRenderTargets(2, r_target, nullptr);

	if(!isLightweight)
	{
		auto volumeBuffer = render_mgr->voxelRenderer->GetVolumeBuffer();
		Render::PSSetConstantBuffers(2, 1, &volumeBuffer); 

		volumeBuffer = render_mgr->voxelRenderer->GetVolumeTraceBuffer();
		Render::PSSetConstantBuffers(3, 1, &volumeBuffer); 
	}

	sp_HDRtoLDR->Draw();

	PERF_GPU_TIMESTAMP(_AA);
	// smaa
	rt_Antialiased->SetRenderTarget(1, 2);
	sp_Antialiased[0]->Draw();

	rt_Antialiased->SetRenderTarget(0, 1);
	sp_Antialiased[1]->Draw();

	Render::ClearRenderTargetView(rt_Antialiased->m_RTV[1], Vector4(0,0,0,0));

	rt_Antialiased->SetRenderTarget(1, 2);
	sp_Antialiased[2]->Draw();
}

void ScenePipeline::LinearAndDepthToRT(RenderTarget* rt, ScreenPlane* sp)
{
	sp->SetTexture(rt_OpaqueFinal->GetShaderResourceView(0), 0);
	// to do: separate combine?

	rt->ClearRenderTargets();
	rt->SetRenderTarget();

	sp->Draw();

	sp->ClearTex();
}

bool ScenePipeline::SaveScreenshot(string path, uint32_t targetX, uint32_t targetY)
{
	unique_ptr<ScreenPlane> sp(new ScreenPlane(SP_SHADER_SCREENSHOT));
	unique_ptr<RenderTarget> rt(new RenderTarget);
	if(!rt->Init(targetX, targetY))
		return false;

	if(!rt->AddRT(DXGI_FORMAT_B8G8R8A8_UNORM))
		return false;

	struct ParamsBuffer
	{
		Vector4 samplesPerPixel;
		Vector4 pixelSize;
	} params;

	auto paramsBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(ParamsBuffer), true);
	
	params.samplesPerPixel.x = (float)(width / targetX);
	params.samplesPerPixel.y = (float)(height / targetY);
	params.samplesPerPixel.z = 1.0f / ( params.samplesPerPixel.x * params.samplesPerPixel.y );
	params.samplesPerPixel.w = 0;
	params.pixelSize.x = 1.0f / width;
	params.pixelSize.y = 1.0f / height;
	params.pixelSize.z = 0;
	params.pixelSize.w = 0;

	Render::UpdateDynamicResource(paramsBuffer, &params, sizeof(ParamsBuffer));
	Render::PSSetConstantBuffers(0, 1, &paramsBuffer); 

	sp->SetTexture(rt_Antialiased->GetShaderResourceView(1), 0);

	rt->ClearRenderTargets();
	rt->SetRenderTarget();

	sp->Draw();
	sp->ClearTex();
		
	_RELEASE(paramsBuffer);

	return TexLoader::SaveTexture(path, rt->GetShaderResourceView(0));
}