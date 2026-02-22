#include "stdafx.h"

#include "VoxelRenderer.h"
#include "RenderMgrs.h"
#include "Render.h"
#include "Utils\Profiler.h"

using namespace EngineCore;

//#define NO_VOXEL_GI

VoxelRenderer::VoxelRenderer(SceneRenderMgr* rndm)
{
	render_mgr = rndm;
	
	voxelizationDumb = nullptr;
	voxelizationDumbRTV = nullptr;
	voxelEmittance = nullptr;
	voxelEmittanceUAV = nullptr;
	voxelEmittanceSRV = nullptr;
	voxelLight0 = nullptr;
	voxelLight0UAV = nullptr;
	voxelLight0SRV = nullptr;
	voxelLight1 = nullptr;
	voxelLight1UAV = nullptr;
	voxelLight1SRV = nullptr;
	voxelDownsampleTemp = nullptr;
	voxelDownsampleTempUAV = nullptr;
	voxelDownsampleTempSRV = nullptr;
	
	volumeMatBuffer = nullptr;
	volumeDataBuffer = nullptr;
	volumeDataPrevBuffer = nullptr;
	volumeTraceDataBuffer = nullptr;
	levelBuffer = nullptr;

	volumeLightInfo = nullptr;
	volumeDownsampleBuffer = nullptr;

	voxelPropagateLight = nullptr;
	
	for(auto i = 0; i < 4; i++)
	{
		voxelDownsample[i] = nullptr;
		voxelDownsampleMove[i] = nullptr;
	}

	injectGroupsCount[0] = 0;
	injectGroupsCount[1] = 0;
	injectGroupsCount[2] = 0;

	volumeResolution = 0;
	clipmapCount = 0;
	mipmapCount = 0;
	volumeSize = 0;
	
	calcVolumesConfigs();

	if(!initVoxelBuffers())
		ERR("Failed init voxel buffers");

	instanceMatrixBuffer = Buffer::CreateConstantBuffer(sizeof(StmMatrixBuffer) * VCT_MESH_MAX_INSTANCE, true);

	meshesToRender.create(clipmapCount);
	meshesToRender.resize(clipmapCount);
	matrixPerMesh.create(clipmapCount);
	matrixPerMesh.resize(clipmapCount);
	meshInstanceGroups.create(clipmapCount);
	meshInstanceGroups.resize(clipmapCount);
}

VoxelRenderer::~VoxelRenderer()
{
	render_mgr = nullptr;

	GFX_DEVICE->DestroyRTV(voxelizationDumbRTV); voxelizationDumbRTV = nullptr;
	GFX_DEVICE->DestroyTexture(voxelizationDumb); voxelizationDumb = nullptr;
	GFX_DEVICE->DestroyView(voxelEmittanceUAV); voxelEmittanceUAV = nullptr;
	GFX_DEVICE->DestroyView(voxelEmittanceSRV); voxelEmittanceSRV = nullptr;
	GFX_DEVICE->DestroyTexture(voxelEmittance); voxelEmittance = nullptr;
	GFX_DEVICE->DestroyView(voxelLight0UAV); voxelLight0UAV = nullptr;
	GFX_DEVICE->DestroyView(voxelLight0SRV); voxelLight0SRV = nullptr;
	GFX_DEVICE->DestroyTexture(voxelLight0); voxelLight0 = nullptr;
	GFX_DEVICE->DestroyView(voxelLight1UAV); voxelLight1UAV = nullptr;
	GFX_DEVICE->DestroyView(voxelLight1SRV); voxelLight1SRV = nullptr;
	GFX_DEVICE->DestroyTexture(voxelLight1); voxelLight1 = nullptr;
	GFX_DEVICE->DestroyView(voxelDownsampleTempUAV); voxelDownsampleTempUAV = nullptr;
	GFX_DEVICE->DestroyView(voxelDownsampleTempSRV); voxelDownsampleTempSRV = nullptr;
	GFX_DEVICE->DestroyTexture(voxelDownsampleTemp); voxelDownsampleTemp = nullptr;
	
	_DELETE(volumeMatBuffer);
	_DELETE(volumeDataBuffer);
	_DELETE(volumeDataPrevBuffer);
	_DELETE(volumeTraceDataBuffer);
	_DELETE(levelBuffer);

	_DELETE(volumeLightInfo);
	_DELETE(volumeDownsampleBuffer);

	_DELETE(voxelPropagateLight);

	for(auto i = 0; i < 4; i++)
	{
		_DELETE(voxelDownsample[i]);
		_DELETE(voxelDownsampleMove[i]);
	}

	_DELETE(instanceMatrixBuffer);
}

void VoxelRenderer::ClearPerFrame()
{
	for(uint8_t i = 0; i < clipmapCount; i++)
	{
		meshesToRender[i].resize(0);
		matrixPerMesh[i].resize(0);
		meshInstanceGroups[i].resize(0);
	}

	spotVoxel_array.resize(0);
	pointVoxel_array.resize(0);
	dirVoxel_array.resize(0);
}

void VoxelRenderer::calcVolumesConfigs()
{
	// temp configs
	volumeResolution = VCT_VOLUME_RES;
	clipmapCount = VCT_CLIPMAP_COUNT;
	mipmapCount = VCT_MIPMAP_COUNT;
	volumeSize = VCT_VOLUME_SIZE;
	AAquality = VCT_SUBSAMPLES;

	uint16_t levelsCount = clipmapCount + mipmapCount;

	injectGroupsCount[2] = volumeResolution / 4;
	injectGroupsCount[0] = injectGroupsCount[2] * VCT_CLIPMAP_COUNT;
	injectGroupsCount[1] = injectGroupsCount[2];

	volumesConfig.destroy();
	volumesConfig.create(levelsCount);
	volumesConfig.resize(levelsCount);

	float fullXRes = (float)(volumeResolution * clipmapCount + volumeResolution / 2);
	float fullYRes = (float)(volumeResolution/* * 6*/);
	for(uint16_t i = 0; i < clipmapCount; i++)
	{
		volumesConfig[i].corner = Vector3::Zero;
		volumesConfig[i].worldSize = volumeSize * pow(2.0f, (float)i);
		volumesConfig[i].voxelSize = volumesConfig[i].worldSize / volumeResolution;
		float halfWorldSize = volumesConfig[i].worldSize * 0.5f;
		volumesConfig[i].volumeBox.Extents = Vector3(halfWorldSize, halfWorldSize, halfWorldSize);

		volumeData[i].worldSize = volumesConfig[i].worldSize;
		volumeData[i].worldSizeRcp = 1.0f / volumesConfig[i].worldSize;
		volumeData[i].scaleHelper = (float)volumeResolution / volumesConfig[i].worldSize;
		volumeData[i].volumeRes = volumeResolution;
		volumeData[i].levelOffset = Vector2((float)(volumeResolution * i), 0);
		volumeData[i].levelOffsetTex = Vector2( volumeData[i].levelOffset.x / fullXRes, 0);
		volumeData[i].voxelSize = float(volumesConfig[i].worldSize) / volumeResolution;
		volumeData[i].voxelSizeRcp = 1.0f / volumeData[i].voxelSize;
		volumeData[i].voxelDiag = sqrt( volumeData[i].voxelSize * volumeData[i].voxelSize * 3 );
		volumeData[i].voxelDiagRcp = 1.0f / volumeData[i].voxelDiag;
	}

	uint16_t levelOffset = 0;
	for(uint16_t i = clipmapCount; i < levelsCount; i++)
	{
		uint16_t resolution = volumeResolution / (uint16_t)pow(2.0f, (float)(i - clipmapCount + 1));

		volumesConfig[i].worldSize = volumesConfig[clipmapCount - 1].worldSize;
		volumesConfig[i].voxelSize = volumesConfig[i].worldSize / resolution;
		float halfWorldSize = volumesConfig[i].worldSize * 0.5f;
		volumesConfig[i].volumeBox.Extents = Vector3(halfWorldSize, halfWorldSize, halfWorldSize);

		volumeData[i].worldSize = volumesConfig[i].worldSize;
		volumeData[i].worldSizeRcp = 1.0f / volumesConfig[i].worldSize;
		volumeData[i].scaleHelper = (float)resolution / volumesConfig[i].worldSize;
		volumeData[i].volumeRes = resolution;
		volumeData[i].levelOffset = Vector2( (float)(volumeResolution * clipmapCount), (float)levelOffset );
		volumeData[i].levelOffsetTex = Vector2( volumeData[i].levelOffset.x / fullXRes, volumeData[i].levelOffset.y / fullYRes );
		volumeData[i].voxelSize = float(volumesConfig[i].worldSize) / resolution;
		volumeData[i].voxelSizeRcp = 1.0f / volumeData[i].voxelSize;
		volumeData[i].voxelDiag = sqrt( volumeData[i].voxelSize * volumeData[i].voxelSize * 3 );
		volumeData[i].voxelDiagRcp = 1.0f / volumeData[i].voxelDiag;

		levelOffset += resolution;
	}

	viewport.topLeftX = 0.0f;
	viewport.topLeftY = 0.0f;
	viewport.height = viewport.width = (float)volumeResolution;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
}

bool VoxelRenderer::initVoxelBuffers()
{
	// dumb MSAA target
	{
		RHI::TextureDesc dumbDesc = {};
		dumbDesc.dimension = RHI::TextureDimension::Tex2D;
		dumbDesc.width = volumeResolution;
		dumbDesc.height = volumeResolution;
		dumbDesc.depth = 1;
		dumbDesc.mipLevels = 1;
		dumbDesc.format = DXGI_FORMAT_R8_UNORM;
		dumbDesc.msaaSamples = AAquality;
		dumbDesc.msaaQuality = 0;
		dumbDesc.allowRTV = true;
		voxelizationDumb = GFX_DEVICE->CreateTexture(dumbDesc);
		if(!voxelizationDumb)
			return false;

		voxelizationDumbRTV = GFX_DEVICE->CreateRTV(voxelizationDumb, DXGI_FORMAT_R8_UNORM, 0, 0);
		if(!voxelizationDumbRTV)
			return false;
	}

	// emittance 3D volume
	uint32_t volumeWidth = volumeResolution * clipmapCount + volumeResolution / 2;
	uint32_t volumeHeight = volumeResolution;
	uint32_t volumeDepth = volumeResolution;
	{
		RHI::TextureDesc volumeDesc = {};
		volumeDesc.dimension = RHI::TextureDimension::Tex3D;
		volumeDesc.width = volumeWidth;
		volumeDesc.height = volumeHeight;
		volumeDesc.depth = volumeDepth;
		volumeDesc.mipLevels = 1;
		volumeDesc.format = DXGI_FORMAT_R32G32B32A32_TYPELESS;
		volumeDesc.allowSRV = true;
		volumeDesc.allowUAV = true;

		voxelEmittance = GFX_DEVICE->CreateTexture(volumeDesc);
		if(!voxelEmittance)
			return false;

		voxelEmittanceUAV = GFX_DEVICE->CreateUAV(voxelEmittance, DXGI_FORMAT_R32G32B32A32_UINT, 0);
		if(!voxelEmittanceUAV)
			return false;

		voxelEmittanceSRV = GFX_DEVICE->CreateSRV(voxelEmittance, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, UINT32_MAX);
		if(!voxelEmittanceSRV)
			return false;
	}

	// voxelLight0 3D volume
	{
		RHI::TextureDesc volumeDesc = {};
		volumeDesc.dimension = RHI::TextureDimension::Tex3D;
		volumeDesc.width = volumeWidth;
		volumeDesc.height = volumeHeight;
		volumeDesc.depth = volumeDepth;
		volumeDesc.mipLevels = 1;
		volumeDesc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		volumeDesc.allowSRV = true;
		volumeDesc.allowUAV = true;

		voxelLight0 = GFX_DEVICE->CreateTexture(volumeDesc);
		if(!voxelLight0)
			return false;

		voxelLight0UAV = GFX_DEVICE->CreateUAV(voxelLight0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
		if(!voxelLight0UAV)
			return false;

		voxelLight0SRV = GFX_DEVICE->CreateSRV(voxelLight0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, UINT32_MAX);
		if(!voxelLight0SRV)
			return false;
	}

	// voxelLight1 3D volume
	{
		RHI::TextureDesc volumeDesc = {};
		volumeDesc.dimension = RHI::TextureDimension::Tex3D;
		volumeDesc.width = volumeWidth;
		volumeDesc.height = volumeHeight;
		volumeDesc.depth = volumeDepth;
		volumeDesc.mipLevels = 1;
		volumeDesc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		volumeDesc.allowSRV = true;
		volumeDesc.allowUAV = true;

		voxelLight1 = GFX_DEVICE->CreateTexture(volumeDesc);
		if(!voxelLight1)
			return false;

		voxelLight1UAV = GFX_DEVICE->CreateUAV(voxelLight1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
		if(!voxelLight1UAV)
			return false;

		voxelLight1SRV = GFX_DEVICE->CreateSRV(voxelLight1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, UINT32_MAX);
		if(!voxelLight1SRV)
			return false;
	}

	// downsample temp 3D volume
	uint32_t downsampleRes = volumeResolution / 2 + 1;
	{
		RHI::TextureDesc volumeDesc = {};
		volumeDesc.dimension = RHI::TextureDimension::Tex3D;
		volumeDesc.width = downsampleRes;
		volumeDesc.height = downsampleRes;
		volumeDesc.depth = downsampleRes;
		volumeDesc.mipLevels = 1;
		volumeDesc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		volumeDesc.allowSRV = true;
		volumeDesc.allowUAV = true;

		voxelDownsampleTemp = GFX_DEVICE->CreateTexture(volumeDesc);
		if(!voxelDownsampleTemp)
			return false;

		voxelDownsampleTempUAV = GFX_DEVICE->CreateUAV(voxelDownsampleTemp, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
		if(!voxelDownsampleTempUAV)
			return false;

		voxelDownsampleTempSRV = GFX_DEVICE->CreateSRV(voxelDownsampleTemp, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, UINT32_MAX);
		if(!voxelDownsampleTempSRV)
			return false;
	}

	volumeDataBuffer = Buffer::CreateConstantBuffer(sizeof(VolumeData) * VCT_MAX_COUNT, true);
	volumeDataPrevBuffer = Buffer::CreateConstantBuffer(sizeof(VolumeData) * VCT_MAX_COUNT, true);

	volumeMatBuffer = Buffer::CreateConstantBuffer(sizeof(VolumeMatrix), true);
	levelBuffer = Buffer::CreateConstantBuffer(sizeof(uint32_t) * 4, true);

	volumeLightInfo = Buffer::CreateConstantBuffer(sizeof(uint32_t) * 4, true);
	volumeDownsampleBuffer = Buffer::CreateConstantBuffer(sizeof(VolumeDownsample), true);

	spotLightInjectBuffer = Buffer::CreateStructedBuffer(CASTER_SPOT_FRAME_MAX, sizeof(SpotVoxelBuffer), true);
	pointLightInjectBuffer = Buffer::CreateStructedBuffer(CASTER_POINT_FRAME_MAX, sizeof(PointVoxelBuffer), true);
	dirLightInjectBuffer = Buffer::CreateStructedBuffer(LIGHT_DIR_FRAME_MAX, sizeof(DirVoxelBuffer), true);

	volumeTraceDataBuffer = Buffer::CreateConstantBuffer(sizeof(VolumeTraceData), true);

	VolumeTraceData volumeTraceData;
	volumeTraceData.levelsCount = clipmapCount + mipmapCount;
	volumeTraceData.xVolumeSizeRcp = 1.0f / ((float)clipmapCount + 0.5f);
	volumeTraceData.maxLevel = volumeTraceData.levelsCount - 1;
	volumeTraceData.clipmapCount = clipmapCount;
	GFX_CMD->UpdateBuffer(volumeTraceDataBuffer, &volumeTraceData, sizeof(VolumeTraceData));

	voxelPropagateLight = new Compute( COMPUTE_VOXEL_PROPAGATE_LIGHT );
	voxelPropagateLight->AttachRWResource("targetLightVolume", voxelLight1UAV);
	voxelPropagateLight->AttachResource("emittanceVolume", voxelEmittanceSRV);
	voxelPropagateLight->AttachResource("sourceLightVolume", voxelLight0SRV);
	voxelPropagateLight->AttachConstantBuffer("volumeDataBuffer", volumeDataBuffer);
	voxelPropagateLight->AttachConstantBuffer("volumeTraceDataBuffer", volumeTraceDataBuffer);

	voxelDownsample[0] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "1#" );
	voxelDownsample[0]->AttachRWResource("downsampleVolumeRW", voxelDownsampleTempUAV);
	voxelDownsample[0]->AttachResource("emittanceVolume", voxelEmittanceSRV);
	voxelDownsample[0]->AttachConstantBuffer("volumeBuffer", volumeDataBuffer);
	voxelDownsample[0]->AttachConstantBuffer("downsampleBuffer", volumeDownsampleBuffer);

	voxelDownsampleMove[0] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "1#" );
	voxelDownsampleMove[0]->AttachRWResource("emittanceVolumeRW", voxelEmittanceUAV);
	voxelDownsampleMove[0]->AttachResource("downsampleVolume", voxelDownsampleTempSRV);
	voxelDownsampleMove[0]->AttachConstantBuffer("volumeBuffer", volumeDataBuffer);
	voxelDownsampleMove[0]->AttachConstantBuffer("downsampleBuffer", volumeDownsampleBuffer);

	voxelDownsample[1] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "2#" );
	voxelDownsample[1]->AttachRWResource("downsampleVolumeRW", voxelDownsampleTempUAV);
	voxelDownsample[1]->AttachResource("emittanceVolume", voxelEmittanceSRV);
	voxelDownsample[1]->AttachConstantBuffer("volumeBuffer", volumeDataBuffer);
	voxelDownsample[1]->AttachConstantBuffer("downsampleBuffer", volumeDownsampleBuffer);

	voxelDownsampleMove[1] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "2#" );
	voxelDownsampleMove[1]->AttachRWResource("emittanceVolumeRW", voxelEmittanceUAV);
	voxelDownsampleMove[1]->AttachResource("downsampleVolume", voxelDownsampleTempSRV);
	voxelDownsampleMove[1]->AttachConstantBuffer("volumeBuffer", volumeDataBuffer);
	voxelDownsampleMove[1]->AttachConstantBuffer("downsampleBuffer", volumeDownsampleBuffer);

	voxelDownsample[2] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "4#" );
	voxelDownsample[2]->AttachRWResource("downsampleVolumeRW", voxelDownsampleTempUAV);
	voxelDownsample[2]->AttachResource("emittanceVolume", voxelEmittanceSRV);
	voxelDownsample[2]->AttachConstantBuffer("volumeBuffer", volumeDataBuffer);
	voxelDownsample[2]->AttachConstantBuffer("downsampleBuffer", volumeDownsampleBuffer);

	voxelDownsampleMove[2] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "4#" );
	voxelDownsampleMove[2]->AttachRWResource("emittanceVolumeRW", voxelEmittanceUAV);
	voxelDownsampleMove[2]->AttachResource("downsampleVolume", voxelDownsampleTempSRV);
	voxelDownsampleMove[2]->AttachConstantBuffer("volumeBuffer", volumeDataBuffer);
	voxelDownsampleMove[2]->AttachConstantBuffer("downsampleBuffer", volumeDownsampleBuffer);

	voxelDownsample[3] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "8#" );
	voxelDownsample[3]->AttachRWResource("downsampleVolumeRW", voxelDownsampleTempUAV);
	voxelDownsample[3]->AttachResource("emittanceVolume", voxelEmittanceSRV);
	voxelDownsample[3]->AttachConstantBuffer("volumeBuffer", volumeDataBuffer);
	voxelDownsample[3]->AttachConstantBuffer("downsampleBuffer", volumeDownsampleBuffer);

	voxelDownsampleMove[3] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "8#" );
	voxelDownsampleMove[3]->AttachRWResource("emittanceVolumeRW", voxelEmittanceUAV);
	voxelDownsampleMove[3]->AttachResource("downsampleVolume", voxelDownsampleTempSRV);
	voxelDownsampleMove[3]->AttachConstantBuffer("volumeBuffer", volumeDataBuffer);
	voxelDownsampleMove[3]->AttachConstantBuffer("downsampleBuffer", volumeDownsampleBuffer);

	// first update to prevent uninit data
	updateBuffers();

	return true;
}

void VoxelRenderer::updateBuffers()
{
	// store previous volume
	swap(voxelLight0, voxelLight1);
	swap(voxelLight0SRV, voxelLight1SRV);
	swap(voxelLight0UAV, voxelLight1UAV);
	//swap(volumeDataPrevBuffer, volumeDataBuffer);
	
	GFX_CMD->UpdateBuffer(spotLightInjectBuffer.buf, spotVoxel_array.data(), spotVoxel_array.size() * sizeof(SpotVoxelBuffer));
	GFX_CMD->UpdateBuffer(pointLightInjectBuffer.buf, pointVoxel_array.data(), pointVoxel_array.size() * sizeof(PointVoxelBuffer));
	GFX_CMD->UpdateBuffer(dirLightInjectBuffer.buf, dirVoxel_array.data(), dirVoxel_array.size() * sizeof(DirVoxelBuffer));

	uint32_t lightCount[4] = {(uint32_t)spotVoxel_array.size(), (uint32_t)pointVoxel_array.size(), 
		(uint32_t)dirVoxel_array.size(), 0};
	GFX_CMD->UpdateBuffer(volumeLightInfo, lightCount, sizeof(uint32_t) * 4);

	GFX_CMD->UpdateBuffer(volumeDataBuffer, volumeData, sizeof(VolumeData) * (clipmapCount + mipmapCount));

	Vector3 camDirs[3];
	camDirs[0] = Vector3(1.0f, 0.0f, 0.0f);
	camDirs[1] = Vector3(0.0f, 1.0f, 0.0f);
	camDirs[2] = Vector3(0.0f, 0.0f, 1.0f);

	Vector3 camUps[3];
	camUps[0] = Vector3(0.0f, 1.0f, 0.0f);
	camUps[1] = Vector3(1.0f, 0.0f, 0.0f);
	camUps[2] = Vector3(0.0f, 1.0f, 0.0f);

	VolumeMatrix matrixBuffer;
	for(uint16_t level = 0; level < clipmapCount; level++)
	{
		auto& bbox = volumesConfig[level].volumeBox;

		Vector3 camPoses[3];
		camPoses[0] = Vector3(bbox.Center.x - bbox.Extents.x, bbox.Center.y, bbox.Center.z);
		camPoses[1] = Vector3(bbox.Center.x, bbox.Center.y - bbox.Extents.y, bbox.Center.z);
		camPoses[2] = Vector3(bbox.Center.x, bbox.Center.y, bbox.Center.z - bbox.Extents.z);
	
		for(uint8_t i = 0; i < 3; i++)
		{
			matrixBuffer.volumeVP[level][i] = XMMatrixLookToLH(camPoses[i], camDirs[i], camUps[i]);
			matrixBuffer.volumeVP[level][i] *= XMMatrixOrthographicLH(volumesConfig[level].worldSize, volumesConfig[level].worldSize, 0.0f, volumesConfig[level].worldSize);
			matrixBuffer.volumeVP[level][i] = XMMatrixTranspose(matrixBuffer.volumeVP[level][i]);
		}
	}

	for(uint16_t level = clipmapCount; level < clipmapCount + mipmapCount; level++)
		for(uint8_t i = 0; i < 3; i++)
			matrixBuffer.volumeVP[level][i] = matrixBuffer.volumeVP[clipmapCount - 1][i];

	GFX_CMD->UpdateBuffer(volumeMatBuffer, (void*)&matrixBuffer, sizeof(VolumeMatrix));
}

void VoxelRenderer::VoxelizeScene()
{
#ifdef NO_VOXEL_GI
	return;
#endif

	prepareMeshData();

	GFX_CMD->ClearUAVUint(voxelEmittanceUAV, 0, 0, 0, 0);

	GFX_CMD->SetRenderTargets(1, &voxelizationDumbRTV, nullptr);
	GFX_CMD->SetOMUnorderedAccessViews(1, 1, &voxelEmittanceUAV, nullptr);

	GFX_CMD->SetViewport(viewport);
	GFX_CMD->SetTopology(RHI::Topology::TriangleList);

	updateBuffers();

	GFX_CMD->SetPSConstantBuffers(4, 1, &volumeMatBuffer);
	GFX_CMD->SetGSConstantBuffers(4, 1, &volumeMatBuffer);

	GFX_CMD->SetPSConstantBuffers(5, 1, &volumeDataBuffer);
	GFX_CMD->SetGSConstantBuffers(5, 1, &volumeDataBuffer);

	GFX_CMD->SetPSConstantBuffers(6, 1, &levelBuffer);
	GFX_CMD->SetGSConstantBuffers(6, 1, &levelBuffer);

	GFX_CMD->SetPSConstantBuffers(7, 1, &volumeTraceDataBuffer);
	GFX_CMD->SetPSConstantBuffers(8, 1, &volumeDataPrevBuffer);

	GFX_CMD->SetPSConstantBuffers(9, 1, &volumeLightInfo);

	GFX_CMD->SetPSResource(9, render_mgr->shadowsRenderer->GetShadowBuffer());
	GFX_CMD->SetPSResource(10, voxelLight0SRV);

	//auto diffCubeSRV = render_mgr->GetDistEnvProb().diffCube;
	//Render::PSSetShaderResources(11, 1, nullptr);

	GFX_CMD->SetPSResource(12, spotLightInjectBuffer.srv);
	GFX_CMD->SetPSResource(13, pointLightInjectBuffer.srv);
	GFX_CMD->SetPSResource(14, dirLightInjectBuffer.srv);
	
	// draw
	for(uint8_t level = 0; level < clipmapCount; level++)
	{
		uint32_t levelData = (uint32_t)level;
		GFX_CMD->UpdateBuffer(levelBuffer, &levelData, sizeof(uint32_t));

		for(auto& currentInstancesGroup: meshInstanceGroups[level])
		{
			auto matrixData = matrixPerMesh[level].begin() + currentInstancesGroup.matrixStart;
			GFX_CMD->UpdateBuffer(instanceMatrixBuffer, matrixData, sizeof(StmMatrixBuffer) * currentInstancesGroup.instanceCount);

			GFX_CMD->SetVertexBuffer(0, currentInstancesGroup.meshData->vertex_buffer, currentInstancesGroup.meshData->vertex_size);
			GFX_CMD->SetIndexBuffer(currentInstancesGroup.meshData->index_buffer, true);

			currentInstancesGroup.meshData->material->SetMatrixBuffer(instanceMatrixBuffer, false); // TODO
			currentInstancesGroup.meshData->material->Set(TECHNIQUES::TECHNIQUE_VOXEL);

			GFX_CMD->DrawIndexedInstanced(currentInstancesGroup.meshData->index_count, currentInstancesGroup.instanceCount);
		}
	}

	GFX_CMD->SetRenderTargets(0, nullptr, nullptr);
}

bool VoxelRenderer::CompareMeshes(VCTRenderMesh& a, VCTRenderMesh& b)
{
	return a.meshHash < b.meshHash;
}

void VoxelRenderer::SwapMeshes(VCTRenderMesh* first, VCTRenderMesh* second, SArray<VCTRenderMesh, VCT_MESH_MAX_COUNT>* meshArr, 
	SArray<StmMatrixBuffer, VCT_MESH_MAX_COUNT>* matrixArr)
{
	swap((*matrixArr)[first->arrayID], (*matrixArr)[second->arrayID]);
	swap(first->arrayID, second->arrayID);
	swap(*first, *second);
}

void VoxelRenderer::prepareMeshData()
{
	for(uint8_t level = 0; level < clipmapCount; level++)
	{
		QSortSwap(meshesToRender[level].begin(), meshesToRender[level].end(), VoxelRenderer::CompareMeshes, 
			VoxelRenderer::SwapMeshes, &meshesToRender[level], &matrixPerMesh[level]);

		uint32_t currentHash = 0;
		uint32_t instancesCount = 0;
		VCTInstanceGroup* currentInstance = nullptr;
	
		for(uint32_t mesh_i = 0; mesh_i < meshesToRender[level].size(); mesh_i++)
		{
			if( meshesToRender[level][mesh_i].meshHash == currentHash && instancesCount < VCT_MESH_MAX_INSTANCE)
			{
				instancesCount++;
				continue;
			}

			if(currentInstance)
				currentInstance->instanceCount = instancesCount;

			currentInstance = meshInstanceGroups[level].push_back();
			currentInstance->meshData = &meshesToRender[level][mesh_i];
			currentInstance->matrixStart = mesh_i;

			currentHash = meshesToRender[level][mesh_i].meshHash;
			instancesCount = 1;
		}

		if(currentInstance)
			currentInstance->instanceCount = instancesCount;
	}
}

void VoxelRenderer::ProcessEmittance()
{
#ifdef NO_VOXEL_GI
	GFX_CMD->ClearUAVFloat(voxelLight1UAV, 0, 0, 0, 0);
	return;
#endif

	/*
	PERF_GPU_TIMESTAMP(_PROPAGATE);

	GFX_CMD->ClearUAVFloat(voxelLight1UAV, 0, 0, 0, 0);

	voxelPropagateLight->AttachRWResource("targetLightVolume", voxelLight1UAV);
	voxelPropagateLight->AttachResource("emittanceVolume", voxelEmittanceSRV);
	voxelPropagateLight->AttachResource("sourceLightVolume", voxelLight0SRV);

	voxelPropagateLight->Dispatch(injectGroupsCount[0], injectGroupsCount[1], injectGroupsCount[2]);
	*/

	PERF_GPU_TIMESTAMP(_VOXELDOWNSAMPLE);
	
	VolumeDownsample volumeDownsample;
	ZeroMemory(&volumeDownsample, sizeof(VolumeDownsample));

	uint32_t currentRes = volumeResolution / 2;
	
	uint32_t threadCount[3];
	threadCount[0] = currentRes / 8;
	threadCount[1] = threadCount[0] * 6;
	threadCount[2] = currentRes / 4;
	/*for(uint16_t level = 1; level < clipmapCount; level++)
	{
		GFX_CMD->ClearUAVFloat(voxelDownsampleTempUAV, 0, 0, 0, 0);

		const Vector3& prevCornerOffset = volumeData[level - 1].cornerOffset;
		const Vector3& currCornerOffset = volumeData[level].cornerOffset;

		Vector3 volumeOffset = prevCornerOffset - currCornerOffset;
		volumeOffset = volumeOffset * volumeData[level].scaleHelper;

		Vector3 volumeOffsetFloor = XMVectorTruncate(volumeOffset);
		Vector3 isShifted = volumeOffset - volumeOffsetFloor;

		volumeDownsample.isShifted.x = isShifted.x > 0.1f ? 1.0f : 0.0f;
		volumeDownsample.isShifted.y = isShifted.y > 0.1f ? 1.0f : 0.0f;
		volumeDownsample.isShifted.z = isShifted.z > 0.1f ? 1.0f : 0.0f;

		volumeDownsample.writeOffset = volumeOffsetFloor;
		volumeDownsample.writeOffset += volumeDownsample.isShifted;

		volumeDownsample.currentLevel = level;
		volumeDownsample.currentRes = currentRes;

		GFX_CMD->UpdateBuffer(volumeDownsampleBuffer, &volumeDownsample, sizeof(VolumeDownsample));

		// downsample
		voxelDownsample[3]->BindUAV(voxelDownsampleTempUAV);
		GFX_CMD->SetCSResource(0, voxelEmittanceSRV);

		voxelDownsample[3]->Dispatch(threadCount[0], threadCount[1], threadCount[2]);

		voxelDownsample[3]->UnbindUAV();
		GFX_CMD->SetCSResource(0, nullptr);

		// move data
		voxelDownsampleMove[3]->BindUAV(voxelEmittanceUAV);
		GFX_CMD->SetCSResource(0, voxelDownsampleTempSRV);

		voxelDownsampleMove[3]->Dispatch(threadCount[0], threadCount[1], threadCount[2]);

		voxelDownsampleMove[3]->UnbindUAV();
		GFX_CMD->SetCSResource(0, nullptr);
	}
	*/
	volumeDownsample.isShifted.x = 0;
	volumeDownsample.isShifted.y = 0;
	volumeDownsample.isShifted.z = 0;
	volumeDownsample.writeOffset.x = 0;
	volumeDownsample.writeOffset.y = 0;
	volumeDownsample.writeOffset.z = 0;
	
	for(uint16_t level = clipmapCount; level < clipmapCount + mipmapCount; level++)
	{
		uint32_t shaderId = 0;
		uint32_t temp = min(uint32_t(8), currentRes);
		while(temp >>= 1) ++shaderId;

		threadCount[0] = max<uint32_t>(uint32_t(1), currentRes / 8);
		threadCount[1] = threadCount[0] * 6;
		threadCount[2] = max<uint32_t>(uint32_t(1), currentRes / 4);

		GFX_CMD->ClearUAVFloat(voxelDownsampleTempUAV, 0, 0, 0, 0);
		
		volumeDownsample.currentLevel = level;
		volumeDownsample.currentRes = currentRes;

		GFX_CMD->UpdateBuffer(volumeDownsampleBuffer, &volumeDownsample, sizeof(VolumeDownsample));

		// downsample
		voxelDownsample[shaderId]->Dispatch(threadCount[0], threadCount[1], threadCount[2]);

		// move data
		voxelDownsampleMove[shaderId]->Dispatch(threadCount[0], threadCount[1], threadCount[2]);

		currentRes /= 2;
	}
}

void VoxelRenderer::RegMeshForVCT(GPUMeshBuffer& index, GPUMeshBuffer& vertex, MeshVertexFormat& format, Material* material, StmMatrixBuffer& matrixData, BoundingOrientedBox& bbox)
{
	if( !material )
		return;

	for(uint8_t level = 0; level < clipmapCount; level++)
	{
		if(meshesToRender[level].full())
			continue;

		// discard if in lower level
		if( level < clipmapCount - 1 )
		{
			/*if( level > 0)
			{
				if(	volumesConfig[level].volumeBox.Contains(bbox) == DISJOINT || volumesConfig[level - 1].volumeBox.Contains(bbox) == CONTAINS )
					continue;
			}
			else
			{*/
				if(	volumesConfig[level].volumeBox.Contains(bbox) == DISJOINT )
					continue;
			//}
		}
		else
		{
			if(	volumesConfig[level - 1].volumeBox.Contains(bbox) != DISJOINT )
				continue;
		}

		float meshSize = max<float>(max<float>(bbox.Extents.x, bbox.Extents.y), bbox.Extents.z) * 4.0f;
		if( meshSize < volumesConfig[level].voxelSize )
			continue;

		bool has_tq = false;
		auto queue = material->GetTechQueue(TECHNIQUES::TECHNIQUE_VOXEL, &has_tq);
		if(!has_tq)
			return;

		auto meshPtr = meshesToRender[level].push_back();
		auto matixPtr = matrixPerMesh[level].push_back();

		meshPtr->index_count = index.count;
		meshPtr->vertex_size = MeshLoader::GetVertexSize(format);
		meshPtr->index_buffer = index.buffer;
		meshPtr->vertex_buffer = vertex.buffer;
		meshPtr->material = material;
		meshPtr->arrayID = (uint32_t)meshesToRender[level].size() - 1;

		matixPtr->world = matrixData.world;
		matixPtr->norm = matrixData.norm;

		meshPtr->meshHash = calcMeshHash(meshPtr);
	}
}

void VoxelRenderer::CalcVolumeBox(Vector3& camPos, Vector3& camDir)
{
	Vector3 prevCornerOffset;
	for(uint8_t i = 0; i < clipmapCount; i++)
	{
		float halfWorldSize = volumesConfig[i].worldSize * 0.5f;
		float centerOffset = halfWorldSize - (volumesConfig[i].voxelSize * VCT_BACK_VOXEL_COUNT);

		Vector3 center = camPos + camDir * centerOffset;
		center /= volumesConfig[i].voxelSize;
		center = Vector3(XMVectorFloor(center)) * volumesConfig[i].voxelSize;
		volumesConfig[i].volumeBox.Center = center;

		const Vector3 prevCorner = volumesConfig[i].corner;

		volumesConfig[i].corner = center - Vector3(halfWorldSize, halfWorldSize, halfWorldSize);
		volumeData[i].cornerOffset = volumesConfig[i].corner;

		Vector3 cornerPrevOffset = prevCorner - volumesConfig[i].corner;
		cornerPrevOffset /= volumesConfig[i].voxelSize;

		volumeData[i].prevFrameOffset.x = floorf(cornerPrevOffset.x + 0.5f);
		volumeData[i].prevFrameOffset.y = floorf(cornerPrevOffset.y + 0.5f);
		volumeData[i].prevFrameOffset.z = floorf(cornerPrevOffset.z + 0.5f);

		Vector3 volumeOffset;
		if( i > 0 )
		{
			volumeOffset = prevCornerOffset - volumesConfig[i].corner;
			volumeOffset = volumeOffset * volumeData[i].scaleHelper;
		}
		else
		{
			volumeOffset = prevCornerOffset;
		}
		
		volumeData[i].volumeOffset = volumeOffset;

		prevCornerOffset = volumesConfig[i].corner;
	}

	uint16_t lastClipmap = clipmapCount - 1;
	for(uint16_t i = clipmapCount; i < clipmapCount + mipmapCount; i++)
	{
		volumesConfig[i].volumeBox.Center = volumesConfig[lastClipmap].volumeBox.Center;
		volumesConfig[i].corner = volumesConfig[lastClipmap].corner;

		volumeData[i].cornerOffset = volumeData[lastClipmap].cornerOffset;
		volumeData[i].volumeOffset.x = 0;
		volumeData[i].volumeOffset.y = 0;
		volumeData[i].volumeOffset.z = 0;
	}
}