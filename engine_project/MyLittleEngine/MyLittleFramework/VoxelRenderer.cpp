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
	voxelDownsampleTemp = nullptr;
	voxelDownsampleTempUAV = nullptr;
	voxelDownsampleTempSRV = nullptr;
	
	volumeMatBuffer = nullptr;
	volumeDataBuffer = nullptr;
	levelBuffer = nullptr;

	volumeLightInfo = nullptr;
	volumeDownsampleBuffer = nullptr;

	voxelInjectLight = nullptr;
	voxelDownsample = nullptr;
	voxelDownsampleMove = nullptr;

	volumeResolution = 0;
	clipmapCount = 0;
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
	_RELEASE(voxelSceneUAV);
	_RELEASE(voxelSceneSRV);
	_RELEASE(voxelScene);
	_RELEASE(voxelSceneColor0UAV);
	_RELEASE(voxelSceneColor0SRV);
	_RELEASE(voxelSceneColor0);
	_RELEASE(voxelSceneColor1UAV);
	_RELEASE(voxelSceneColor1SRV);
	_RELEASE(voxelSceneColor1);
	_RELEASE(voxelSceneNormalUAV);
	_RELEASE(voxelSceneNormalSRV);
	_RELEASE(voxelSceneNormal);
	_RELEASE(voxelEmittanceUAV);
	_RELEASE(voxelEmittanceSRV);
	_RELEASE(voxelEmittance);
	_RELEASE(voxelDownsampleTempUAV);
	_RELEASE(voxelDownsampleTempSRV);
	_RELEASE(voxelDownsampleTemp);
	
	_RELEASE(volumeMatBuffer);
	_RELEASE(volumeDataBuffer);
	_RELEASE(levelBuffer);

	_RELEASE(volumeLightInfo);
	_RELEASE(volumeDownsampleBuffer);

	_DELETE(voxelInjectLight);
	_DELETE(voxelDownsample);
	_DELETE(voxelDownsampleMove);

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
	volumeResolution = VCT_VOLUME_RES;
	clipmapCount = VCT_CLIPMAP_COUNT;
	volumeSize = VCT_VOLUME_SIZE;
	AAquality = VCT_SUBSAMPLES;

	volumesConfig.destroy();
	volumesConfig.create(clipmapCount);
	volumesConfig.resize(clipmapCount);

	for(uint8_t i = 0; i < clipmapCount; i++)
	{
		volumesConfig[i].worldSize = volumeSize * pow(2.0f, (float)i);
		volumesConfig[i].voxelSize = volumesConfig[i].worldSize / volumeResolution;
		float halfWorldSize = volumesConfig[i].worldSize * 0.5f;
		volumesConfig[i].volumeBox.Extents = XMFLOAT3(halfWorldSize, halfWorldSize, halfWorldSize);

		volumeData[i].worldSize = volumesConfig[i].worldSize;
		volumeData[i].worldSizeRcp = 1.0f / volumesConfig[i].worldSize;
		volumeData[i].scaleHelper = (float)volumeResolution / volumesConfig[i].worldSize;
		volumeData[i].volumeRes = volumeResolution;
		volumeData[i].voxelSize = float(volumesConfig[i].worldSize) / volumeResolution;
		volumeData[i].voxelSizeRcp = 1.0f / volumeData[i].voxelSize;
		volumeData[i].voxelDiag = sqrt( volumeData[i].voxelSize * volumeData[i].voxelSize * 3 );
		volumeData[i].voxelDiagRcp = 1.0f / volumeData[i].voxelDiag;
		volumeData[i].maxLevel = clipmapCount - 1;
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

	// visibility
	D3D11_TEXTURE3D_DESC volumeDesc;
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = volumeResolution * clipmapCount;			// x - level
	volumeDesc.Height = volumeResolution * 6;					// y - face
	volumeDesc.Depth = volumeResolution;
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
	volumeUAVDesc.Texture3D.WSize = volumeResolution;
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
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelSceneColor0)) )
		return false;
	if( FAILED(Render::CreateUnorderedAccessView(voxelSceneColor0, &volumeUAVDesc, &voxelSceneColor0UAV)) )
		return false;
	if( FAILED(Render::CreateShaderResourceView(voxelSceneColor0, &volumeSRVDesc, &voxelSceneColor0SRV)) )
		return false;

	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelSceneColor1)) )
		return false;
	if( FAILED(Render::CreateUnorderedAccessView(voxelSceneColor1, &volumeUAVDesc, &voxelSceneColor1UAV)) )
		return false;
	if( FAILED(Render::CreateShaderResourceView(voxelSceneColor1, &volumeSRVDesc, &voxelSceneColor1SRV)) )
		return false;

	// normal
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelSceneNormal)) )
		return false;
	if( FAILED(Render::CreateUnorderedAccessView(voxelSceneNormal, &volumeUAVDesc, &voxelSceneNormalUAV)) )
		return false;
	if( FAILED(Render::CreateShaderResourceView(voxelSceneNormal, &volumeSRVDesc, &voxelSceneNormalSRV)) )
		return false;
	
	// emittance
	volumeDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelEmittance)) )
		return false;

	volumeUAVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	if( FAILED(Render::CreateUnorderedAccessView(voxelEmittance, &volumeUAVDesc, &voxelEmittanceUAV)) )
		return false;

	volumeSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	if( FAILED(Render::CreateShaderResourceView(voxelEmittance, &volumeSRVDesc, &voxelEmittanceSRV)) )
		return false;

	// downsample
	uint32_t downsampleRes = volumeResolution / 2 + 1;

	volumeDesc.Width = downsampleRes;
	volumeDesc.Height = downsampleRes * 6;
	volumeDesc.Depth = downsampleRes;
	if( FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &voxelDownsampleTemp)) )
		return false;

	volumeUAVDesc.Texture3D.WSize = downsampleRes;
	if( FAILED(Render::CreateUnorderedAccessView(voxelDownsampleTemp, &volumeUAVDesc, &voxelDownsampleTempUAV)) )
		return false;

	if( FAILED(Render::CreateShaderResourceView(voxelDownsampleTemp, &volumeSRVDesc, &voxelDownsampleTempSRV)) )
		return false;

	volumeDataBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeData) * VCT_CLIPMAP_COUNT_MAX, true);
	volumeMatBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeMatrix), true);
	levelBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(uint32_t) * 4, true);

	volumeLightInfo = Buffer::CreateConstantBuffer(DEVICE, sizeof(uint32_t) * 4, true);
	volumeDownsampleBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeDownsample), true);

	spotLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, SPOT_VOXEL_FRAME_MAX, sizeof(SpotVoxelBuffer), true);
	pointLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, POINT_VOXEL_FRAME_MAX, sizeof(PointVoxelBuffer), true);
	dirLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, LIGHT_DIR_FRAME_MAX, sizeof(DirVoxelBuffer), true);

	voxelInjectLight = new Compute( COMPUTE_VOXEL_INJECT_LIGHT );
	voxelDownsample = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE );
	voxelDownsampleMove = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE );

	return true;
}

void VoxelRenderer::updateBuffers()
{
	Render::UpdateDynamicResource(volumeDataBuffer, volumeData, sizeof(VolumeData) * clipmapCount);
	
	XMVECTOR camDirs[3];
	camDirs[0] = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	camDirs[1] = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	camDirs[2] = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	XMVECTOR camUps[3];
	camUps[0] = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	camUps[1] = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	camUps[2] = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	VolumeMatrix matrixBuffer;
	for(uint8_t level = 0; level < clipmapCount; level++)
	{
		auto& bbox = volumesConfig[level].volumeBox;

		XMVECTOR camPoses[3];
		camPoses[0] = XMVectorSet(bbox.Center.x - bbox.Extents.x, bbox.Center.y, bbox.Center.z, 1.0f);
		camPoses[1] = XMVectorSet(bbox.Center.x, bbox.Center.y - bbox.Extents.y, bbox.Center.z, 1.0f);
		camPoses[2] = XMVectorSet(bbox.Center.x, bbox.Center.y, bbox.Center.z - bbox.Extents.z, 1.0f);
	
		for(uint8_t i = 0; i < 3; i++)
		{
			matrixBuffer.volumeVP[level][i] = XMMatrixLookToLH(camPoses[i], camDirs[i], camUps[i]);
			matrixBuffer.volumeVP[level][i] *= XMMatrixOrthographicLH(volumesConfig[level].worldSize, volumesConfig[level].worldSize, 0.0f, volumesConfig[level].worldSize);
			matrixBuffer.volumeVP[level][i] = XMMatrixTranspose(matrixBuffer.volumeVP[level][i]);
		}
	}

	Render::UpdateDynamicResource(volumeMatBuffer, (void*)&matrixBuffer, sizeof(VolumeMatrix));
}

void VoxelRenderer::VoxelizeScene()
{
	prepareMeshData();
	
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

	Render::RSSetViewports(1, &viewport);
	Render::SetTopology(IA_TOPOLOGY::TRISLIST);

	updateBuffers();

	Render::PSSetConstantBuffers(4, 1, &volumeMatBuffer);
	Render::GSSetConstantBuffers(4, 1, &volumeMatBuffer);

	Render::PSSetConstantBuffers(5, 1, &volumeDataBuffer);
	Render::GSSetConstantBuffers(5, 1, &volumeDataBuffer);
	
	Render::PSSetConstantBuffers(6, 1, &levelBuffer);
	Render::GSSetConstantBuffers(6, 1, &levelBuffer);

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

			currentInstancesGroup.meshData->material->SetMatrixBuffer(instanceMatrixBuffer);
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
	PERF_GPU_TIMESTAMP(_LIGHTINJECT);

	Render::ClearUnorderedAccessViewFloat(voxelEmittanceUAV, XMFLOAT4(0,0,0,0));

	Render::UpdateDynamicResource(spotLightInjectBuffer.buf, spotVoxel_array.data(), spotVoxel_array.size() * sizeof(SpotVoxelBuffer));
	Render::UpdateDynamicResource(pointLightInjectBuffer.buf, pointVoxel_array.data(), pointVoxel_array.size() * sizeof(PointVoxelBuffer));
	Render::UpdateDynamicResource(dirLightInjectBuffer.buf, dirVoxel_array.data(), dirVoxel_array.size() * sizeof(DirVoxelBuffer));

	uint32_t lightCount[4] = {(uint32_t)spotVoxel_array.size(), (uint32_t)pointVoxel_array.size(), 
		(uint32_t)dirVoxel_array.size(), 0};
	Render::UpdateDynamicResource(volumeLightInfo, lightCount, sizeof(uint32_t) * 4);

	voxelInjectLight->BindUAV(voxelEmittanceUAV);

	auto shadowsBufferSRV = render_mgr->shadowsRenderer->GetShadowBuffer();
	Render::CSSetShaderResources(0, 1, &shadowsBufferSRV);

	Render::CSSetShaderResources(1, 1, &voxelSceneSRV);
	Render::CSSetShaderResources(2, 1, &voxelSceneColor0SRV);
	Render::CSSetShaderResources(3, 1, &voxelSceneColor1SRV);
	Render::CSSetShaderResources(4, 1, &voxelSceneNormalSRV);

	Render::CSSetShaderResources(5, 1, &spotLightInjectBuffer.srv);
	Render::CSSetShaderResources(6, 1, &pointLightInjectBuffer.srv);
	Render::CSSetShaderResources(7, 1, &dirLightInjectBuffer.srv);

	Render::CSSetConstantBuffers(0, 1, &volumeDataBuffer);
	Render::CSSetConstantBuffers(1, 1, &volumeLightInfo);

	voxelInjectLight->Dispatch( 32 * clipmapCount, 32 * 6, 32 );
	voxelInjectLight->UnbindUAV();
	
	
	/*PERF_GPU_TIMESTAMP(_VOXELDOWNSAMPLE);
	
	VolumeDownsample volumeDownsample;
	uint32_t currentRes = volumeResolution / 2;
	ID3D11ShaderResourceView* null_srv = nullptr;
	
	Render::CSSetConstantBuffers(0, 1, &volumeDataBuffer);
	Render::CSSetConstantBuffers(1, 1, &volumeDownsampleBuffer);
	
	for(uint32_t level = 1; level < clipmapCount; level++)
	{
		Render::ClearUnorderedAccessViewFloat(voxelDownsampleTempUAV, XMFLOAT4(0,0,0,0));

		XMFLOAT3& prevCornerOffset = volumeData[level - 1].cornerOffset;
		XMFLOAT3& currCornerOffset = volumeData[level].cornerOffset;
		XMVECTOR volumeOffset = XMVectorSet(prevCornerOffset.x - currCornerOffset.x, 
										prevCornerOffset.y - currCornerOffset.y,
										prevCornerOffset.z - currCornerOffset.z, 0.0f);
		volumeOffset = volumeOffset * volumeData[level].scaleHelper;
		XMVECTOR volumeOffsetFloor = XMVectorFloor(volumeOffset);
		XMVECTOR isShifted = volumeOffset - volumeOffsetFloor;

		volumeDownsample.isShifted[0] = XMVectorGetX(isShifted) > 0.1f ? 1 : 0;
		volumeDownsample.isShifted[1] = XMVectorGetY(isShifted) > 0.1f ? 1 : 0;
		volumeDownsample.isShifted[2] = XMVectorGetZ(isShifted) > 0.1f ? 1 : 0;
		
		XMStoreFloat3(&volumeDownsample.volumeOffset, volumeOffsetFloor);
		volumeDownsample.currentLevel = level;
		volumeDownsample.currentRes = currentRes;
		volumeDownsample.currentResMore = currentRes + 1;

		Render::UpdateDynamicResource(volumeDownsampleBuffer, &volumeDownsample, sizeof(VolumeDownsample));

		uint32_t processRes[3];
		for(uint8_t k = 0; k < 3; k++)
			processRes[k] = currentRes + (volumeDownsample.isShifted[k] ? 1 : 0);

		// downsample
		voxelDownsample->BindUAV(voxelDownsampleTempUAV);
		Render::CSSetShaderResources(0, 1, &voxelEmittanceSRV);
				
		voxelDownsample->Dispatch( processRes[0], processRes[1] * 6, processRes[2] );

		voxelDownsample->UnbindUAV();
		Render::CSSetShaderResources(0, 1, &null_srv);

		// move data
		voxelDownsampleMove->BindUAV(voxelEmittanceUAV);
		Render::CSSetShaderResources(0, 1, &voxelDownsampleTempSRV);

		voxelDownsampleMove->Dispatch( processRes[0], processRes[1] * 6, processRes[2] );

		voxelDownsampleMove->UnbindUAV();
		Render::CSSetShaderResources(0, 1, &null_srv);
	}*/
}

void VoxelRenderer::RegMeshForVCT(uint32_t& index_count, uint32_t&& vertex_size, ID3D11Buffer* index_buffer, ID3D11Buffer* vertex_buffer, Material* material, StmMatrixBuffer& matrixData, BoundingOrientedBox& bbox)
{
	for(uint8_t level = 0; level < clipmapCount; level++)
	{
		/*if( level < clipmapCount - 1 )
		{
			if( level > 0)
			{
				if(	volumesConfig[level].volumeBox.Contains(bbox) == DISJOINT || volumesConfig[level - 1].volumeBox.Contains(bbox) != DISJOINT )
					continue;
			}
			else
			{
				if(	volumesConfig[level].volumeBox.Contains(bbox) == DISJOINT )
					continue;
			}
		}
		else
		{
			if(	volumesConfig[level - 1].volumeBox.Contains(bbox) != DISJOINT )
				continue;
		}*/
		float meshSize = max(max(bbox.Extents.x, bbox.Extents.y), bbox.Extents.z) * 2;
		if( meshSize < volumesConfig[level].voxelSize )
			continue;

		if(	volumesConfig[level].volumeBox.Contains(bbox) == DISJOINT )
			continue;

		if(meshesToRender[level].full())
			return;

		bool has_tq = false;
		auto queue = material->GetTechQueue(TECHNIQUES::TECHNIQUE_VOXEL, &has_tq);
		if(!has_tq)
			return;

		auto meshPtr = meshesToRender[level].push_back();
		auto matixPtr = matrixPerMesh[level].push_back();

		meshPtr->index_count = index_count;
		meshPtr->vertex_size = vertex_size;
		meshPtr->index_buffer = index_buffer;
		meshPtr->vertex_buffer = vertex_buffer;
		meshPtr->material = material;
		meshPtr->arrayID = (uint32_t)meshesToRender[level].size() - 1;

		matixPtr->world = matrixData.world;
		matixPtr->norm = matrixData.norm;

		meshPtr->meshHash = calcMeshHash(meshPtr);
	}
}

void VoxelRenderer::CalcVolumeBox(XMVECTOR& camPos, XMVECTOR& camDir)
{
	XMVECTOR prevCornerOffset = XMVectorZero();
	for(uint8_t i = 0; i < clipmapCount; i++)
	{
		float halfWorldSize = volumesConfig[i].worldSize * 0.5f;
		float centerOffset = halfWorldSize - (volumesConfig[i].voxelSize * VCT_BACK_VOXEL_COUNT);

		XMVECTOR center = camPos + camDir * centerOffset;
		center = XMVectorFloor(center / volumesConfig[i].voxelSize) * volumesConfig[i].voxelSize;
		XMStoreFloat3(&volumesConfig[i].volumeBox.Center, center);

		XMVECTOR corner = center - XMVectorSet(halfWorldSize, halfWorldSize, halfWorldSize, 0.0f);
		XMStoreFloat3(&volumesConfig[i].corner, corner);

		volumeData[i].cornerOffset = volumesConfig[i].corner;

		XMVECTOR volumeOffset;
		if( i > 0 )
		{
			volumeOffset = prevCornerOffset - corner;
			volumeOffset = volumeOffset * volumeData[i].scaleHelper;
		}
		else
		{
			volumeOffset = prevCornerOffset;
		}
		
		XMStoreFloat3(&volumeData[i].volumeOffset, volumeOffset);

		prevCornerOffset = corner;
	}
}