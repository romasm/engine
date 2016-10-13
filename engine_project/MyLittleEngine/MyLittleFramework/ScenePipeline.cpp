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

	rt_OpaqueForward = nullptr;
	rt_AO = nullptr;
	rt_OpaqueDefferedDirect = nullptr;
	rt_OpaqueFinal = nullptr;
	rt_HiZVis = nullptr;
	rt_HiZDepth = nullptr;
	rt_TransparentRecursive = nullptr;
	rt_TransparentForward = nullptr;
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

	sp_OpaqueDefferedDirect = nullptr;
	sp_AO = nullptr;
	sp_FinalOpaque = nullptr;
	sp_HDRtoLDR = nullptr;
	//sp_3DHud = nullptr;

	sp_HiZ = nullptr;

	sp_AvgLum = nullptr;
	sp_Bloom = nullptr;

	sp_SSR = nullptr;

	sp_Antialiased[0] = nullptr;
	sp_Antialiased[1] = nullptr;
	sp_Antialiased[2] = nullptr;

	Materials_Count = 0;

	render_mgr = nullptr;

	b_directSpec = true;
	b_indirectSpec = true;
	b_directDiff = true;
	b_indirectDiff = true;
	b_renderHud = true;

	m_CamMoveBuffer = nullptr;
	m_SharedBuffer = nullptr;
	m_AOBuffer = nullptr;

	codemgr = nullptr;

	render_mode = 0;

	current_camera = nullptr;
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
	
	_RELEASE(lightSpotBuffer);
	_RELEASE(lightPointBuffer);
	_RELEASE(lightDirBuffer);

	_RELEASE(casterSpotBuffer);
	_RELEASE(casterDiskBuffer);
	_RELEASE(casterRectBuffer);
	_RELEASE(casterPointBuffer);
	_RELEASE(casterSphereBuffer);
	_RELEASE(casterTubeBuffer);

	m_LightShadowStructBuffer.Release();

	m_EnvProbBuffer.Release();
	m_MaterialBuffer.Release();

	_RELEASE(m_SharedBuffer);
	_RELEASE(m_CamMoveBuffer);
	_RELEASE(m_AOBuffer);

	_CLOSE(render_mgr);
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

	_CLOSE(rt_OpaqueForward);
	_CLOSE(rt_TransparentForward);
	_CLOSE(rt_AO);
	_CLOSE(rt_OpaqueDefferedDirect);
	_CLOSE(rt_TransparentRecursive);
	_CLOSE(rt_OpaqueFinal);
	_CLOSE(rt_HiZVis);
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

	_DELETE(sp_OpaqueDefferedDirect);
	_DELETE(sp_AO);
	_DELETE(sp_FinalOpaque);
	_DELETE(sp_HDRtoLDR);

	_DELETE(sp_HiZ);

	_DELETE(sp_Bloom);

	_DELETE(sp_SSR);

	_DELETE(sp_Antialiased[0]);
	_DELETE(sp_Antialiased[1]);
	_DELETE(sp_Antialiased[2]);
}

bool ScenePipeline::Init(int t_width, int t_height)
{
	codemgr = ShaderCodeMgr::Get();
	
	render_mgr = new SceneRenderMgr;
	
	m_SharedBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(SharedBuffer), false);

	m_CamMoveBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(XMMATRIX), false);
		
	lightSpotBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(SpotLightBuffer) + sizeof(SpotLightDiskBuffer) + sizeof(SpotLightRectBuffer), true);
	lightPointBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(PointLightBuffer) + sizeof(PointLightSphereBuffer) + sizeof(PointLightTubeBuffer), true);
	lightDirBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(DirLightBuffer), true);

	casterSpotBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(SpotCasterBuffer), true);
	casterDiskBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(SpotCasterDiskBuffer), true);
	casterRectBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(SpotCasterRectBuffer), true);
	casterPointBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(PointCasterBuffer), true);
	casterSphereBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(PointCasterSphereBuffer), true);
	casterTubeBuffer = Buffer::CreateConstantBuffer(Render::Device(), sizeof(PointCasterTubeBuffer), true);
	
	m_MaterialBuffer = Buffer::CreateStructedBuffer(Render::Device(), sizeof(MaterialParamsStructBuffer)*MATERIALS_COUNT, sizeof(MaterialParamsStructBuffer), MATERIALS_COUNT, true);
	Materials[0].unlit = 0;
	Materials[0].ss_direct_pow = 0;
	Materials[0].ss_direct_translucency = 0;
	Materials[0].ss_distortion = 0;
	Materials[0].ss_indirect_translucency = 0;
	Materials[0].subscattering = 0;

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

	sharedconst.uvCorrectionForPow2 = XMFLOAT2(float(width) / width_pow2, float(height) / height_pow2);

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
	bufferDesc.SampleDesc.Count = 1; // TODO MSAA
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture2D(&bufferDesc, NULL, &sceneDepth)) )
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; // TODO MSAA
	shaderResourceViewDesc.Texture2D.MipLevels = -1;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;	
	if( FAILED(Render::CreateShaderResourceView(sceneDepth, &shaderResourceViewDesc, &sceneDepthSRV)) )
		return false;

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // TODO MSAA
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	if( FAILED(Render::CreateDepthStencilView(sceneDepth, &depthStencilViewDesc, &sceneDepthDSV)) )
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

	sp_AvgLum = new ScreenPlane(SP_MATERIAL_AVGLUM);
	sp_AvgLum->SetFloat(CONFIG(hdr_adopt_speed), 1);
	sp_AvgLum->SetFloat(CONFIG(hdr_limit_min), 2);
	sp_AvgLum->SetFloat(CONFIG(hdr_limit_max), 3);

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
	if(!rt_OpaqueForward->AddRT(DXGI_FORMAT_R16G16B16A16_FLOAT))return false; // rgb - normal, a - tangent
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

	rt_HiZVis = new RenderTarget;
	if(!rt_HiZVis->Init(width_pow2 / 2, height_pow2 / 2))return false;
	rt_HiZVis->SetMipmappingMaterial(SP_MATERIAL_HIZ_DEPTH);
	if(!rt_HiZVis->AddRT(DXGI_FORMAT_R8_UNORM, 0))return false;

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

	// OPAQUE
	rt_OpaqueDefferedDirect = new RenderTarget;
	if(!rt_OpaqueDefferedDirect->Init(width, height))return false;
	if(!rt_OpaqueDefferedDirect->AddRT(DXGI_FORMAT_R32G32B32A32_FLOAT, 0))return false; // diffuse, specular.r
	if(!rt_OpaqueDefferedDirect->AddRT(DXGI_FORMAT_R32G32_FLOAT))return false; // specular gb

	rt_OpaqueFinal = new RenderTarget;
	if(!rt_OpaqueFinal->Init(width, height))return false;
	rt_OpaqueFinal->SetMipmappingMaterial(SP_MATERIAL_OPAQUE_BLUR);
	if(!rt_OpaqueFinal->AddRT(DXGI_FORMAT_R32G32B32A32_FLOAT))return false;
	if(!rt_OpaqueFinal->AddRT(DXGI_FORMAT_R16G16B16A16_FLOAT, 0))return false;

	// TRANSPARENT: TODO
	rt_TransparentForward = new RenderTarget;
	if(!rt_TransparentForward->Init(width, height, DXGI_FORMAT_D32_FLOAT))return false;
	if(!rt_TransparentForward->AddRT(DXGI_FORMAT_R32G32B32A32_FLOAT))return false;

	rt_TransparentRecursive = new RenderTarget;
	if(!rt_TransparentRecursive->Init(width, height))return false;
	if(!rt_TransparentRecursive->AddRT(DXGI_FORMAT_R32_UINT, 1, true))return false; // r
	if(!rt_TransparentRecursive->AddRT(DXGI_FORMAT_R32_UINT, 1, true))return false; // g
	if(!rt_TransparentRecursive->AddRT(DXGI_FORMAT_R32_UINT, 1, true))return false; // b

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
	sp_SSR->SetTexture(rt_HiZVis->GetShaderResourceView(0), 5);
	sp_SSR->SetTexture(rt_OpaqueForward->GetShaderResourceView(5), 6);

	sp_OpaqueDefferedDirect = new ScreenPlane(SP_MATERIAL_DEFFERED_OPAC_DIR);
	//sp_OpaqueDefferedDirect->SetTextureByNameS(TEX_NOISE2D, 0);
	sp_OpaqueDefferedDirect->SetTextureByNameS(TEX_PBSENVLUT, 0);
	sp_OpaqueDefferedDirect->SetTexture(rt_OpaqueForward->GetShaderResourceView(0), 3);
	sp_OpaqueDefferedDirect->SetTexture(rt_OpaqueForward->GetShaderResourceView(1), 4);
	sp_OpaqueDefferedDirect->SetTexture(rt_OpaqueForward->GetShaderResourceView(2), 5);
	sp_OpaqueDefferedDirect->SetTexture(rt_OpaqueForward->GetShaderResourceView(3), 6);
	sp_OpaqueDefferedDirect->SetTexture(rt_OpaqueForward->GetShaderResourceView(4), 7);
	sp_OpaqueDefferedDirect->SetTexture(rt_OpaqueForward->GetShaderResourceView(5), 8);
	sp_OpaqueDefferedDirect->SetTexture(rt_OpaqueForward->GetShaderResourceView(6), 9);
	sp_OpaqueDefferedDirect->SetTexture(rt_OpaqueForward->GetShaderResourceView(7), 10);
	sp_OpaqueDefferedDirect->SetTexture(rt_HiZDepth->GetShaderResourceView(0), 11);
	sp_OpaqueDefferedDirect->SetTexture(rt_AO->GetShaderResourceView(0), 12);
	sp_OpaqueDefferedDirect->SetTexture(rt_SSR->GetShaderResourceView(0), 13);

	sp_AO = new ScreenPlane(SP_MATERIAL_AO);
	sp_AO->SetTextureByNameS(TEX_HBAO_DITHER, 0);
	sp_AO->SetTexture(rt_OpaqueForward->GetShaderResourceView(1), 1);
	sp_AO->SetTexture(rt_HiZDepth->GetShaderResourceView(0), 2);
	sp_AO->SetFloat(CONFIG(ao_half_sample_radius), 0);
	sp_AO->SetFloat(CONFIG(ao_inv_distance_falloff), 1);
	sp_AO->SetFloat(CONFIG(ao_hieght_bias), 2);
	sp_AO->SetFloat(CONFIG(ao_hiz_mip_scaler), 3);
	sp_AO->SetFloat(CONFIG(ao_max_dist_sqr), 4);
	
	sp_FinalOpaque = new ScreenPlane(SP_MATERIAL_COMBINE);
	sp_FinalOpaque->SetTexture(rt_OpaqueDefferedDirect->GetShaderResourceView(0), 0);
	sp_FinalOpaque->SetTexture(rt_OpaqueDefferedDirect->GetShaderResourceView(1), 1);
	sp_FinalOpaque->SetTexture(rt_OpaqueForward->GetShaderResourceView(5), 2);
	sp_FinalOpaque->SetTexture(rt_HiZDepth->GetShaderResourceView(0), 3);

	sp_AvgLum->SetTexture(rt_OpaqueFinal->GetShaderResourceView(1), 0);
	sp_AvgLum->SetFloat(float(rt_OpaqueFinal->GetMipsCountInFullChain() - 2), 0);

	sp_Bloom = new ScreenPlane(SP_MATERIAL_BLOOM_FIND);
	sp_Bloom->SetTexture(rt_OpaqueFinal->GetShaderResourceView(0), 0);
	sp_Bloom->SetTexture(rt_AvgLum->GetShaderResourceView(0), 1);
	sp_Bloom->SetFloat(CONFIG(hdr_bloom_threshold), 0);
	sp_Bloom->SetFloat(CONFIG(hdr_bloom_max), 1);

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

	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_shoulder_strength), 0);
	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_linear_strength), 1);
	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_linear_angle), 2);
	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_toe_strength), 3);
	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_toe_numerator), 4);
	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_toe_denominator), 5);
	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_white_point), 6);
	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_exposure_adjustment), 7);
	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_apply_to_lum) ? 1.0f : 0.0f, 8);
	sp_HDRtoLDR->SetFloat(CONFIG(tonemap_middle_gray), 9);
	sp_HDRtoLDR->SetFloat(CONFIG(hdr_bloom_amount), 10);
	sp_HDRtoLDR->SetFloat(CONFIG(hdr_bloom_mul), 11);

	sp_Antialiased[0] = new ScreenPlane(SP_MATERIAL_AA_EDGE);
	sp_Antialiased[0]->SetTexture(rt_Antialiased->GetShaderResourceView(0), 0);
	sp_Antialiased[0]->SetFloat(CONFIG(smaa_threshold), 0);

	sp_Antialiased[1] = new ScreenPlane(SP_MATERIAL_AA_BLEND);
	sp_Antialiased[1]->SetTextureByNameS(TEX_SMAA_AREA, 0);
	sp_Antialiased[1]->SetTextureByNameS(TEX_SMAA_SEARCH, 1);
	sp_Antialiased[1]->SetTexture(rt_Antialiased->GetShaderResourceView(1), 2);
	sp_Antialiased[1]->SetFloat(CONFIG(smaa_search_steps), 0);
	sp_Antialiased[1]->SetFloat(CONFIG(smaa_search_steps_diag), 1);
	sp_Antialiased[1]->SetFloat(CONFIG(smaa_corner_rounding), 2);
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

bool ScenePipeline::StartFrame(LocalTimer* timer)
{
	if(!camera.sys)
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

	sharedconst.perspParam = 0.5f * (sharedconst.projection.r[1].m128_f32[1] + sharedconst.projection.r[2].m128_f32[2]);

	Render::UpdateSubresource(m_SharedBuffer, 0, NULL, &sharedconst, 0, 0);
	Render::PSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	Render::VSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	Render::HSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	Render::DSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	Render::GSSetConstantBuffers(0, 1, &m_SharedBuffer); 
	
	// remove
	float projParam = 0.5f * (sharedconst.projection.r[1].m128_f32[1] + sharedconst.projection.r[2].m128_f32[2]);
	sp_AO->SetFloat(projParam, 5);
#if AO_TYPE == 0
	float maxDistSqr = (CONFIG(ao_half_sample_radius) * sharedconst.screenW * projParam * 0.5f - sharedconst.projection.r[3].m128_f32[3]) / sharedconst.projection.r[3].m128_f32[2];
	maxDistSqr *= maxDistSqr;
	sp_AO->SetFloat(maxDistSqr, 4);
#endif

	XMMATRIX camMove = XMMatrixTranspose(current_camera->prevViewProj) * sharedconst.invViewProjection;
	Render::UpdateSubresource(m_CamMoveBuffer, 0, NULL, &camMove, 0, 0);
	
	return true;
}

void ScenePipeline::EndFrame()
{
	render_mgr->ClearAll();
	current_camera = nullptr;
}

void ScenePipeline::HudStage()
{
	if(!b_renderHud) return;

	rt_3DHud->ClearRenderTargets(false);
	rt_3DHud->SetRenderTarget();

	render_mgr->DrawHud();

	Render::ClearDepthStencilView(rt_3DHud->m_DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	render_mgr->DrawOvHud();
}

void ScenePipeline::OpaqueForwardStage()
{
	rt_OpaqueForward->ClearRenderTargets();
	rt_OpaqueForward->SetRenderTarget();
	Materials_Count = 1;

	render_mgr->DrawOpaque(this);

	//ID3D11Resource* hiz_topmip = nullptr;
	//rt_HiZDepth->GetRenderTargetView(0)->GetResource(&hiz_topmip);
	//CONTEXT->CopyResource(hiz_topmip, sceneDepth);
	rt_HiZDepth->SetRenderTarget();
	sp_OpaqueDepthCopy->Draw();
}

void ScenePipeline::TransparentForwardStage()
{
	/*rt_TransparentForward->ClearRenderTargets();
	rt_TransparentForward->SetRenderTarget();
	rt_TransparentRecursive->ClearRenderTargets();

	// рекурсивный рендеринг
	ID3D11UnorderedAccessView* UAV[3];
	UAV[0] = rt_TransparentRecursive->GetUnorderedAccessView(0);
	UAV[1] = rt_TransparentRecursive->GetUnorderedAccessView(1);
	UAV[2] = rt_TransparentRecursive->GetUnorderedAccessView(2);

	ID3D11RenderTargetView* r_target = rt_TransparentForward->GetRenderTargetView(0);
	ID3D11DepthStencilView* r_depth = rt_TransparentForward->GetDepthStencilView();

	Render::OMSetRenderTargetsAndUnorderedAccessViews(1, &r_target, r_depth, 1, 3, UAV, nullptr);

	unsigned int old_ps_tex_offset = PS_tex_offset;

	// to do: cubemap & lights
	//Render::PSSetShaderResources(0, 1, &m_LightSpotStructBuffer.srv);
	//Render::PSSetShaderResources(1, 1, &m_LightShadowStructBuffer.srv);
	PS_tex_offset += 2;

	ID3D11ShaderResourceView* opaque = rt_OpaqueFinal->GetShaderResourceView(0);
	Render::PSSetShaderResources(PS_tex_offset, 1, &opaque);
	PS_tex_offset += 1;

	unsigned int cubemap_num = (unsigned int)sp_OpaqueDefferedIndirect->m_material->m_material.m_perframetex.size();
	if(cubemap_num)Render::PSSetShaderResources(PS_tex_offset, cubemap_num, &sp_OpaqueDefferedIndirect->m_material->m_material.m_perframetex[0]);
	PS_tex_offset += cubemap_num;

	unsigned int light_num = (unsigned int)sp_OpaqueDefferedDirect->m_material->m_material.m_perframetex.size();
	if(light_num)Render::PSSetShaderResources(PS_tex_offset, light_num, &sp_OpaqueDefferedDirect->m_material->m_material.m_perframetex[0]);
	PS_tex_offset += 64;//light_num

	render_mgr->DrawTransparent(this);

	PS_tex_offset = old_ps_tex_offset;

	r_target = nullptr;
	r_depth = nullptr;
	UAV[0] = nullptr;
	UAV[1] = nullptr;
	UAV[2] = nullptr;
	Render::OMSetRenderTargetsAndUnorderedAccessViews(1, &r_target, r_depth, 1, 3, UAV, nullptr);*/
}
/*
bool ScenePipeline::CompareEnvProbs(EnvProb* first, EnvProb* second)
{
	return first->GetDist() < second->GetDist();
}

void ScenePipeline::LoadEnvProbs(map<int, EnvProb*> ep)
{
	sp_OpaqueDefferedIndirect->ClearPerFrameTex();
	EnvProbs_Count = 0;

	ID3D11ShaderResourceView* skyEnv = nullptr;
	ID3D11ShaderResourceView* skyEnvDiff = nullptr;
	float skyMips = 0;

	int i=0;
	vector<EnvProb*> cur_envprobs;
	for(auto& it : ep)
	{
		if(!skyEnv && it.second->b_sky)
		{
			skyEnv = it.second->GetCubemap();
			skyEnvDiff = it.second->GetDiffuseCubemap();
			skyMips = float(it.second->GetMips()-1);
			continue;
		}
	
		if(it.second->IsInFrust(&t_cam->m_frustum) && it.second->GetCubemap())
		{
			cur_envprobs.push_back(it.second);
			i++;
			if(i>=ENVPROBS_COUNT)break;
		}
	}

	if(skyEnv && skyEnvDiff)
	{
		sp_OpaqueDefferedIndirect->AddPerFrameTex(skyEnv);
		sp_OpaqueDefferedIndirect->AddPerFrameTex(skyEnvDiff);
		sp_OpaqueDefferedIndirect->GetMaterial()->SendFloatToShader( skyMips, 3, ID_PS);
	}

	sort(cur_envprobs.begin(), cur_envprobs.end(), ScenePipeline::CompareEnvProbs );

	for(auto& it : cur_envprobs)
	{
		sp_OpaqueDefferedIndirect->AddPerFrameTex(it->GetCubemap());
		it->AddToBuffer(EnvProbs, &EnvProbs_Count);
	}
}
*/

void ScenePipeline::LoadEnvProbs()
{
	auto& distProb = render_mgr->GetDistEnvProb();

	if(distProb.mipsCount == 0)
		return;

	sp_OpaqueDefferedDirect->SetTexture(distProb.specCube, 14);
	sp_OpaqueDefferedDirect->SetTexture(distProb.diffCube, 15);
	sp_OpaqueDefferedDirect->SetFloat(float(distProb.mipsCount), 13);
	
	// todo: distProb.matrix

	/*D3D11_MAPPED_SUBRESOURCE mappedResourceEP;
	result = Render::Map(m_EnvProbBuffer.buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceEP);
	if(FAILED(result))
		return;

	EnvProbBuffer* ep_dataPtr = (EnvProbBuffer*)mappedResourceEP.pData;
	for(int i=0; i<EnvProbs_Count; i++)
	{
		*ep_dataPtr = EnvProbs[i];
		ep_dataPtr++;
	}

	Render::Unmap(m_EnvProbBuffer.buf, 0);
	
	Render::PSSetShaderResources(0, 1, &m_EnvProbBuffer.srv);*/
	//sp_OpaqueDefferedIndirect->GetMaterial()->SendFloatToShader(float(EnvProbs_Count), 0, ID_PS);
	
}

uint8_t ScenePipeline::LoadLights()
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

	uint8_t offset = 0;

	uint8_t ps_constbuf_offset = 1;

	if(spot_size > 0 || disk_size > 0 || rect_size > 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(FAILED(Render::Map(lightSpotBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return offset;

		uint8_t* v_data = (uint8_t*)mappedResource.pData;
		if(spot_size > 0)
			memcpy((void*)v_data, spot_data, sizeof(SpotLightBuffer));
		v_data += sizeof(SpotLightBuffer);

		if(disk_size > 0)
			memcpy((void*)v_data, disk_data, sizeof(SpotLightDiskBuffer));
		v_data += sizeof(SpotLightDiskBuffer);
		
		if(rect_size > 0)
			memcpy((void*)v_data, rect_data, sizeof(SpotLightRectBuffer));

		Render::Unmap(lightSpotBuffer, 0);
	}
	Render::PSSetConstantBuffers(ps_constbuf_offset, 1, &lightSpotBuffer); 
	offset++;
	
	if(point_size > 0 || sphere_size > 0 || tube_size > 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(FAILED(Render::Map(lightPointBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return offset;

		uint8_t* v_data = (uint8_t*)mappedResource.pData;
		if(point_size > 0)
			memcpy((void*)v_data, point_data, sizeof(PointLightBuffer));
		v_data += sizeof(PointLightBuffer);
		
		if(sphere_size > 0)
			memcpy((void*)v_data, sphere_data, sizeof(PointLightSphereBuffer));
		v_data += sizeof(PointLightSphereBuffer);
		
		if(tube_size > 0)
			memcpy((void*)v_data, tube_data, sizeof(PointLightTubeBuffer));

		Render::Unmap(lightPointBuffer, 0);
	}
	Render::PSSetConstantBuffers(ps_constbuf_offset + offset, 1, &lightPointBuffer);
	offset++;

	if(dir_size > 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(FAILED(Render::Map(lightDirBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return offset;
		memcpy(mappedResource.pData, dir_data, sizeof(DirLightBuffer));
		Render::Unmap(lightDirBuffer, 0);
	}
	Render::PSSetConstantBuffers(ps_constbuf_offset + offset, 1, &lightDirBuffer);
	offset++;

	if(caster_spot_size > 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(FAILED(Render::Map(casterSpotBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return offset;
		memcpy(mappedResource.pData, caster_spot_data, sizeof(SpotCasterBuffer));
		Render::Unmap(casterSpotBuffer, 0);
	}
	Render::PSSetConstantBuffers(ps_constbuf_offset + offset, 1, &casterSpotBuffer); 
	offset++;

	if(caster_disk_size > 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(FAILED(Render::Map(casterDiskBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return offset;
		memcpy(mappedResource.pData, caster_disk_data, sizeof(SpotCasterDiskBuffer));
		Render::Unmap(casterDiskBuffer, 0);
	}
	Render::PSSetConstantBuffers(ps_constbuf_offset + offset, 1, &casterDiskBuffer); 
	offset++;

	if(caster_rect_size > 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(FAILED(Render::Map(casterRectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return offset;
		memcpy(mappedResource.pData, caster_rect_data, sizeof(SpotCasterRectBuffer));
		Render::Unmap(casterRectBuffer, 0);
	}
	Render::PSSetConstantBuffers(ps_constbuf_offset + offset, 1, &casterRectBuffer); 
	offset++;

	if(caster_point_size > 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(FAILED(Render::Map(casterPointBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return offset;
		memcpy(mappedResource.pData, caster_point_data, sizeof(PointCasterBuffer));
		Render::Unmap(casterPointBuffer, 0);
	}
	Render::PSSetConstantBuffers(ps_constbuf_offset + offset, 1, &casterPointBuffer); 
	offset++;

	if(caster_sphere_size > 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(FAILED(Render::Map(casterSphereBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return offset;
		memcpy(mappedResource.pData, caster_sphere_data, sizeof(PointCasterSphereBuffer));
		Render::Unmap(casterSphereBuffer, 0);
	}
	Render::PSSetConstantBuffers(ps_constbuf_offset + offset, 1, &casterSphereBuffer); 
	offset++;

	if(caster_tube_size > 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if(FAILED(Render::Map(casterTubeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return offset;
		memcpy(mappedResource.pData, caster_tube_data, sizeof(PointCasterTubeBuffer));
		Render::Unmap(casterTubeBuffer, 0);
	}
	Render::PSSetConstantBuffers(ps_constbuf_offset + offset, 1, &casterTubeBuffer); 
	offset++;

	sp_OpaqueDefferedDirect->SetFloat((float)spot_size, 0);
	sp_OpaqueDefferedDirect->SetFloat((float)disk_size, 1);
	sp_OpaqueDefferedDirect->SetFloat((float)rect_size, 2);
	sp_OpaqueDefferedDirect->SetFloat((float)point_size, 3);
	sp_OpaqueDefferedDirect->SetFloat((float)sphere_size, 4);
	sp_OpaqueDefferedDirect->SetFloat((float)tube_size, 5);
	sp_OpaqueDefferedDirect->SetFloat((float)dir_size, 6);
	sp_OpaqueDefferedDirect->SetFloat((float)caster_spot_size, 7);
	sp_OpaqueDefferedDirect->SetFloat((float)caster_disk_size, 8);
	sp_OpaqueDefferedDirect->SetFloat((float)caster_rect_size, 9);
	sp_OpaqueDefferedDirect->SetFloat((float)caster_point_size, 10);
	sp_OpaqueDefferedDirect->SetFloat((float)caster_sphere_size, 11);
	sp_OpaqueDefferedDirect->SetFloat((float)caster_tube_size, 12);

	return offset;
}

void ScenePipeline::HiZMips()
{
	int prevX = rt_HiZDepth->t_width;
	int prevY = rt_HiZDepth->t_height;

	auto& miplvl = rt_HiZDepth->mipRes[0];

	for(int j=0; j<miplvl.mipCount-1; j++)
	{
		rt_HiZDepth->m_viewport.Width = float(rt_HiZDepth->mip_res[j].x);
		rt_HiZDepth->m_viewport.Height = float(rt_HiZDepth->mip_res[j].y);
		Render::RSSetViewports(1, &rt_HiZDepth->m_viewport);

		ID3D11RenderTargetView* rts[2];
		rts[0] = miplvl.mip_RTV[j];
		rts[1] = j==0 ? rt_HiZVis->m_RTV[0] : rt_HiZVis->mipRes[0].mip_RTV[j-1];
		Render::OMSetRenderTargets(2, rts, nullptr);
		
		sp_HiZ->SetTexture(miplvl.mip_SRV[j], 0);
		if(j!=0)
			sp_HiZ->SetTexture(rt_HiZVis->mipRes[0].mip_SRV[j-1], 1);
		sp_HiZ->SetFloat(float(j), 0);
		sp_HiZ->Draw();

		prevX = rt_HiZDepth->mip_res[j].x;
		prevY = rt_HiZDepth->mip_res[j].y;
	}
	rt_HiZDepth->m_viewport.Width = float(rt_HiZDepth->t_width);
	rt_HiZDepth->m_viewport.Height = float(rt_HiZDepth->t_height);
}

void ScenePipeline::OpaqueDefferedStage()
{
	int t_width = EngineSettings::EngSets.wres;
	int t_height = EngineSettings::EngSets.hres;
	
	PERF_GPU_TIMESTAMP(_HIZ_GEN);
	HiZMips();

	rt_SSR->ClearRenderTargets();
	rt_SSR->SetRenderTarget();
	
	PERF_GPU_TIMESTAMP(_SSR);
	Render::PSSetConstantBuffers(1, 1, &m_CamMoveBuffer); 
	Render::PSSetShaderResources(0, 1, &m_MaterialBuffer.srv);

	sp_SSR->Draw();

	// ao
	PERF_GPU_TIMESTAMP(_AO);

	rt_AO->ClearRenderTargets();
	rt_AO->SetRenderTarget();

	Render::PSSetConstantBuffers(1, 1, &m_AOBuffer); 

	sp_AO->Draw();
	
#ifdef AO_FILTER
	g_AO->blur(rt_AO, 0, rt_HiZDepth->GetShaderResourceView(0));
#endif
	
	PERF_GPU_TIMESTAMP(_SHADOW_HIZ);
	render_mgr->GenerateShadowHiZ();
	auto shadowBuffer = render_mgr->GetShadowBuffer();
	auto shadowBufferMips = render_mgr->GetShadowBufferMips();

	PERF_GPU_TIMESTAMP(_OPAQUE_MAIN);
	// mat params
	D3D11_MAPPED_SUBRESOURCE mappedResourceM;
	if(FAILED(Render::Map(m_MaterialBuffer.buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceM)))
		return;
	memcpy(mappedResourceM.pData, (void*)Materials, Materials_Count * sizeof(MaterialParamsStructBuffer));
	Render::Unmap(m_MaterialBuffer.buf, 0);

	Render::PSSetShaderResources(0, 1, &m_MaterialBuffer.srv);

	sp_OpaqueDefferedDirect->SetTexture(shadowBuffer, 1);
	sp_OpaqueDefferedDirect->SetTexture(shadowBufferMips, 2);

	LoadLights();

	LoadEnvProbs();
	
	Render::PSSetConstantBuffers(10, 1, &m_CamMoveBuffer); 
	
	rt_OpaqueDefferedDirect->ClearRenderTargets();
	rt_OpaqueDefferedDirect->SetRenderTarget();
	
	sp_OpaqueDefferedDirect->SetFloat(b_directDiff ? 1.0f : 0.0f, 14);
	sp_OpaqueDefferedDirect->SetFloat(b_directSpec ? 1.0f : 0.0f, 15);
	sp_OpaqueDefferedDirect->SetFloat(b_indirectDiff ? 1.0f : 0.0f, 16);
	sp_OpaqueDefferedDirect->SetFloat(b_indirectSpec ? 1.0f : 0.0f, 17);

	sp_OpaqueDefferedDirect->Draw();

	PERF_GPU_TIMESTAMP(_OPAQUE_FINAL);
	// final 
	rt_OpaqueFinal->ClearRenderTargets();
	rt_OpaqueFinal->SetRenderTarget();
	sp_FinalOpaque->Draw();

	// blur
	rt_OpaqueFinal->GenerateMipmaps(this); // !!!!!!!!!!!!!! TODO: separete blur
	
	PERF_GPU_TIMESTAMP(_HDR_BLOOM);
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
	
	// bloom prepare
	rt_Bloom->ClearRenderTargets(); // !!!!!!!!!!!! TODO: only opaque now!
	rt_Bloom->SetRenderTarget();
	sp_Bloom->Draw();
	
	rt_Bloom->GenerateMipmaps(this);

	g_Bloom->blur(rt_Bloom);
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

	sp_HDRtoLDR->Draw();

	PERF_GPU_TIMESTAMP(_AA);
	// smaa
	rt_Antialiased->SetRenderTarget(1, 2);
	sp_Antialiased[0]->Draw();

	rt_Antialiased->SetRenderTarget(0, 1);
	sp_Antialiased[1]->Draw();

	Render::ClearRenderTargetView(rt_Antialiased->m_RTV[1], XMFLOAT4(0,0,0,0));

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

/*void ScenePipeline::AllCombineStage()
{
	// combine with 3dhud
	rt_LDRandHud->ClearRenderTargets();
	rt_LDRandHud->SetRenderTarget();
	sp_3DHud->Draw(this);
}*/