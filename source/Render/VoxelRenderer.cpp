#include "stdafx.h"

#include "VoxelRenderer.h"
#include "RenderMgrs.h"
#include "Render.h"
#include "Utils\Profiler.h"

using namespace EngineCore;

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

	instanceMatrixBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(StmMatrixBuffer) * VCT_MESH_MAX_INSTANCE, true);

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

	_RELEASE(voxelizationDumbRTV);
	_RELEASE(voxelizationDumb);
	_RELEASE(voxelEmittanceUAV);
	_RELEASE(voxelEmittanceSRV);
	_RELEASE(voxelEmittance);
	_RELEASE(voxelLight0UAV);
	_RELEASE(voxelLight0SRV);
	_RELEASE(voxelLight0);
	_RELEASE(voxelLight1UAV);
	_RELEASE(voxelLight1SRV);
	_RELEASE(voxelLight1);
	_RELEASE(voxelDownsampleTempUAV);
	_RELEASE(voxelDownsampleTempSRV);
	_RELEASE(voxelDownsampleTemp);
	
	_RELEASE(volumeMatBuffer);
	_RELEASE(volumeDataBuffer);
	_RELEASE(volumeTraceDataBuffer);
	_RELEASE(levelBuffer);

	_RELEASE(volumeLightInfo);
	_RELEASE(volumeDownsampleBuffer);

	_DELETE(voxelPropagateLight);

	for(auto i = 0; i < 4; i++)
	{
		_DELETE(voxelDownsample[i]);
		_DELETE(voxelDownsampleMove[i]);
	}

	_RELEASE(instanceMatrixBuffer);
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
	float fullYRes = (float)(volumeResolution * 6);
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

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Height = viewport.Width = (float)volumeResolution;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

bool VoxelRenderer::initVoxelBuffers()
{
	// dumb MSAA target
	D3D11_TEXTURE2D_DESC dumbDesc;
	ZeroMemory(&dumbDesc, sizeof(dumbDesc));
	dumbDesc.Width = volumeResolution;
	dumbDesc.Height = volumeResolution;
	dumbDesc.MipLevels = 1;
	dumbDesc.ArraySize = 1;
	dumbDesc.Format = DXGI_FORMAT_R8_UNORM;
	dumbDesc.SampleDesc.Count = AAquality;
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

	// emittance
	D3D11_TEXTURE3D_DESC volumeDesc;
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = volumeResolution * clipmapCount + volumeResolution / 2;	// x - level
	volumeDesc.Height = volumeResolution * 6;					// y - face
	volumeDesc.Depth = volumeResolution;
	volumeDesc.MipLevels = 1;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	volumeDesc.Format = DXGI_FORMAT_R32G32B32A32_TYPELESS;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelEmittance)) )
		return false;

	D3D11_UNORDERED_ACCESS_VIEW_DESC volumeUAVDesc;
	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = volumeResolution;
	volumeUAVDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
	if( FAILED(Render::CreateUnorderedAccessView(voxelEmittance, &volumeUAVDesc, &voxelEmittanceUAV)) )
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC volumeSRVDesc;
	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	volumeSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if( FAILED(Render::CreateShaderResourceView(voxelEmittance, &volumeSRVDesc, &voxelEmittanceSRV)) )
		return false;

	volumeDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelLight0)) )
		return false;
	volumeUAVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if( FAILED(Render::CreateUnorderedAccessView(voxelLight0, &volumeUAVDesc, &voxelLight0UAV)) )
		return false;
	volumeSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if( FAILED(Render::CreateShaderResourceView(voxelLight0, &volumeSRVDesc, &voxelLight0SRV)) )
		return false;

	volumeDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelLight1)) )
		return false;
	volumeUAVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if( FAILED(Render::CreateUnorderedAccessView(voxelLight1, &volumeUAVDesc, &voxelLight1UAV)) )
		return false;
	volumeSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if( FAILED(Render::CreateShaderResourceView(voxelLight1, &volumeSRVDesc, &voxelLight1SRV)) )
		return false;

	// downsample
	uint32_t downsampleRes = volumeResolution / 2 + 1;

	volumeDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	volumeDesc.Width = downsampleRes;
	volumeDesc.Height = downsampleRes * 6;
	volumeDesc.Depth = downsampleRes;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelDownsampleTemp)) )
		return false;

	volumeUAVDesc.Texture3D.WSize = downsampleRes;
	volumeUAVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if( FAILED(Render::CreateUnorderedAccessView(voxelDownsampleTemp, &volumeUAVDesc, &voxelDownsampleTempUAV)) )
		return false;

	if( FAILED(Render::CreateShaderResourceView(voxelDownsampleTemp, &volumeSRVDesc, &voxelDownsampleTempSRV)) )
		return false;

	volumeDataBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeData) * VCT_MAX_COUNT, true);

	volumeMatBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeMatrix), true);
	levelBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(uint32_t) * 4, true);

	volumeLightInfo = Buffer::CreateConstantBuffer(DEVICE, sizeof(uint32_t) * 4, true);
	volumeDownsampleBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeDownsample), true);

	spotLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, SPOT_VOXEL_FRAME_MAX, sizeof(SpotVoxelBuffer), true);
	pointLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, POINT_VOXEL_FRAME_MAX, sizeof(PointVoxelBuffer), true);
	dirLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, LIGHT_DIR_FRAME_MAX, sizeof(DirVoxelBuffer), true);

	voxelPropagateLight = new Compute( COMPUTE_VOXEL_INJECT_LIGHT );

	voxelDownsample[0] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "1" );
	voxelDownsampleMove[0] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "1" );
	voxelDownsample[1] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "2" );
	voxelDownsampleMove[1] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "2" );
	voxelDownsample[2] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "4" );
	voxelDownsampleMove[2] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "4" );
	voxelDownsample[3] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "8" );
	voxelDownsampleMove[3] = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "8" );

	volumeTraceDataBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeTraceData), true);

	VolumeTraceData volumeTraceData;
	volumeTraceData.levelsCount = clipmapCount + mipmapCount;
	volumeTraceData.xVolumeSizeRcp = 1.0f / ( (float)clipmapCount + 0.5f );
	volumeTraceData.maxLevel = volumeTraceData.levelsCount - 1;
	volumeTraceData.clipmapCount = clipmapCount;
	Render::UpdateDynamicResource(volumeTraceDataBuffer, &volumeTraceData, sizeof(VolumeTraceData));

	return true;
}

void VoxelRenderer::updateBuffers()
{
	// store previous volume
	swap(voxelLight0, voxelLight1);
	swap(voxelLight0SRV, voxelLight1SRV);
	swap(voxelLight0UAV, voxelLight1UAV);
	
	Render::UpdateDynamicResource(spotLightInjectBuffer.buf, spotVoxel_array.data(), spotVoxel_array.size() * sizeof(SpotVoxelBuffer));
	Render::UpdateDynamicResource(pointLightInjectBuffer.buf, pointVoxel_array.data(), pointVoxel_array.size() * sizeof(PointVoxelBuffer));
	Render::UpdateDynamicResource(dirLightInjectBuffer.buf, dirVoxel_array.data(), dirVoxel_array.size() * sizeof(DirVoxelBuffer));

	uint32_t lightCount[4] = {(uint32_t)spotVoxel_array.size(), (uint32_t)pointVoxel_array.size(), 
		(uint32_t)dirVoxel_array.size(), 0};
	Render::UpdateDynamicResource(volumeLightInfo, lightCount, sizeof(uint32_t) * 4);
	
	Render::UpdateDynamicResource(volumeDataBuffer, volumeData, sizeof(VolumeData) * (clipmapCount + mipmapCount));

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

	Render::UpdateDynamicResource(volumeMatBuffer, (void*)&matrixBuffer, sizeof(VolumeMatrix));
}

void VoxelRenderer::VoxelizeScene()
{
	prepareMeshData();

	Render::ClearUnorderedAccessViewUint(voxelEmittanceUAV, Vector4(0,0,0,0));
	
	Render::OMSetRenderTargetsAndUnorderedAccessViews(1, &voxelizationDumbRTV, nullptr, 1, 1, &voxelEmittanceUAV, nullptr);

	Render::RSSetViewports(1, &viewport);
	Render::SetTopology(IA_TOPOLOGY::TRISLIST);

	updateBuffers();

	Render::PSSetConstantBuffers(4, 1, &volumeMatBuffer);
	Render::GSSetConstantBuffers(4, 1, &volumeMatBuffer);

	Render::PSSetConstantBuffers(5, 1, &volumeDataBuffer);
	Render::GSSetConstantBuffers(5, 1, &volumeDataBuffer);
	
	Render::PSSetConstantBuffers(6, 1, &levelBuffer);
	Render::GSSetConstantBuffers(6, 1, &levelBuffer);

	Render::PSSetConstantBuffers(7, 1, &volumeLightInfo);

	auto shadowsBufferSRV = render_mgr->shadowsRenderer->GetShadowBuffer();
	Render::PSSetShaderResources(9, 1, &shadowsBufferSRV);

	Render::PSSetShaderResources(10, 1, &spotLightInjectBuffer.srv);
	Render::PSSetShaderResources(11, 1, &pointLightInjectBuffer.srv);
	Render::CSSetShaderResources(12, 1, &dirLightInjectBuffer.srv);
	
	// draw
	const unsigned int offset = 0;
	for(uint8_t level = 0; level < clipmapCount; level++)
	{
		uint32_t levelData = (uint32_t)level;
		Render::UpdateDynamicResource(levelBuffer, &levelData, sizeof(uint32_t));

		for(auto& currentInstancesGroup: meshInstanceGroups[level])
		{
			auto matrixData = matrixPerMesh[level].begin() + currentInstancesGroup.matrixStart;
			Render::UpdateDynamicResource(instanceMatrixBuffer, matrixData, sizeof(StmMatrixBuffer) * currentInstancesGroup.instanceCount);

			Render::Context()->IASetVertexBuffers(0, 1, &(currentInstancesGroup.meshData->vertex_buffer), 
				&(currentInstancesGroup.meshData->vertex_size), &offset);
			Render::Context()->IASetIndexBuffer(currentInstancesGroup.meshData->index_buffer, DXGI_FORMAT_R32_UINT, 0);

			currentInstancesGroup.meshData->material->SetMatrixBuffer(instanceMatrixBuffer, false); // TODO
			currentInstancesGroup.meshData->material->Set(TECHNIQUES::TECHNIQUE_VOXEL);
		
			Render::Context()->DrawIndexedInstanced(currentInstancesGroup.meshData->index_count, currentInstancesGroup.instanceCount, 0, 0, 0);
		}
	}

	Render::OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 0, nullptr, nullptr);
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
	PERF_GPU_TIMESTAMP(_PROPAGATE);


	Render::ClearUnorderedAccessViewFloat(voxelLight1UAV, Vector4(0,0,0,0));

	voxelPropagateLight->BindUAV(voxelLight1UAV);

	Render::CSSetShaderResources(0, 1, &voxelEmittanceSRV);
	Render::CSSetShaderResources(1, 1, &voxelLight0SRV);
	
	Render::CSSetConstantBuffers(0, 1, &volumeDataBuffer);

	voxelPropagateLight->Dispatch(injectGroupsCount[0], injectGroupsCount[1], injectGroupsCount[2]);
	voxelPropagateLight->UnbindUAV();
	
	PERF_GPU_TIMESTAMP(_VOXELDOWNSAMPLE);
	
	VolumeDownsample volumeDownsample;
	ZeroMemory(&volumeDownsample, sizeof(VolumeDownsample));

	uint32_t currentRes = volumeResolution / 2;
	ID3D11ShaderResourceView* null_srv = nullptr;
	
	Render::CSSetConstantBuffers(0, 1, &volumeDataBuffer);
	Render::CSSetConstantBuffers(1, 1, &volumeDownsampleBuffer);
	
	uint32_t threadCount[3];
	threadCount[0] = currentRes / 8;
	threadCount[1] = threadCount[0] * 6;
	threadCount[2] = currentRes / 4;
	/*for(uint16_t level = 1; level < clipmapCount; level++)
	{
		Render::ClearUnorderedAccessViewFloat(voxelDownsampleTempUAV, Vector4(0,0,0,0));

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

		Render::UpdateDynamicResource(volumeDownsampleBuffer, &volumeDownsample, sizeof(VolumeDownsample));
		
		// downsample
		voxelDownsample[3]->BindUAV(voxelDownsampleTempUAV);
		Render::CSSetShaderResources(0, 1, &voxelEmittanceSRV);
				
		voxelDownsample[3]->Dispatch(threadCount[0], threadCount[1], threadCount[2]);

		voxelDownsample[3]->UnbindUAV();
		Render::CSSetShaderResources(0, 1, &null_srv);

		// move data
		voxelDownsampleMove[3]->BindUAV(voxelEmittanceUAV);
		Render::CSSetShaderResources(0, 1, &voxelDownsampleTempSRV);

		voxelDownsampleMove[3]->Dispatch(threadCount[0], threadCount[1], threadCount[2]);

		voxelDownsampleMove[3]->UnbindUAV();
		Render::CSSetShaderResources(0, 1, &null_srv);
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

		Render::ClearUnorderedAccessViewFloat(voxelDownsampleTempUAV, Vector4(0,0,0,0));
		
		volumeDownsample.currentLevel = level;
		volumeDownsample.currentRes = currentRes;

		Render::UpdateDynamicResource(volumeDownsampleBuffer, &volumeDownsample, sizeof(VolumeDownsample));

		// downsample
		voxelDownsample[shaderId]->BindUAV(voxelDownsampleTempUAV);
		Render::CSSetShaderResources(0, 1, &voxelEmittanceSRV);

		voxelDownsample[shaderId]->Dispatch(threadCount[0], threadCount[1], threadCount[2]);

		voxelDownsample[shaderId]->UnbindUAV();
		Render::CSSetShaderResources(0, 1, &null_srv);

		// move data
		voxelDownsampleMove[shaderId]->BindUAV(voxelEmittanceUAV);
		Render::CSSetShaderResources(0, 1, &voxelDownsampleTempSRV);

		voxelDownsampleMove[shaderId]->Dispatch(threadCount[0], threadCount[1], threadCount[2]);

		voxelDownsampleMove[shaderId]->UnbindUAV();
		Render::CSSetShaderResources(0, 1, &null_srv);

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

		volumeData[i].prevFrameOffsetX = (int32_t)floorf(cornerPrevOffset.x + 0.5f);
		volumeData[i].prevFrameOffsetY = (int32_t)floorf(cornerPrevOffset.y + 0.5f);
		volumeData[i].prevFrameOffsetZ = (int32_t)floorf(cornerPrevOffset.z + 0.5f);

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