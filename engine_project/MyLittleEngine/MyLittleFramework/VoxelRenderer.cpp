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

	volumeBuffer = nullptr;
	volumeInfo = nullptr;
	volumeDownsampleBuffer = nullptr;
	voxelInjectLight = nullptr;
	voxelDownsample = nullptr;
	voxelDownsampleMove = nullptr;

	if(!initVoxelBuffers())
		ERR("Failed init voxel buffers");

	instanceMatrixBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(StmMatrixBuffer) * VCT_MESH_MAX_INSTANCE, true);
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
	
	_RELEASE(volumeBuffer);
	_RELEASE(volumeInfo);
	_RELEASE(volumeDownsampleBuffer);
	_DELETE(voxelInjectLight);
	_DELETE(voxelDownsample);
	_DELETE(voxelDownsampleMove);

	_RELEASE(instanceMatrixBuffer);
}

void VoxelRenderer::ClearPerFrame()
{
	meshesToRender.resize(0);
	matrixPerMesh.resize(0);
	meshInstanceGroups.resize(0);

	spotVoxel_array.resize(0);
	pointVoxel_array.resize(0);
	dirVoxel_array.resize(0);
}

bool VoxelRenderer::initVoxelBuffers()
{
	// dumb MSAA target
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
	volumeDesc.Width = VOXEL_VOLUME_RES * VOXEL_VOLUME_CLIPMAP_COUNT;	// x - level
	volumeDesc.Height = VOXEL_VOLUME_RES * 6;							// y - face
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
	uint32_t downsampleRes = VOXEL_VOLUME_RES / 2;

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

	volumeBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeData), true);
	volumeInfo = Buffer::CreateConstantBuffer(DEVICE, sizeof(uint32_t) * 4, true);
	volumeDownsampleBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(uint32_t) * 4, true);

	spotLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, SPOT_VOXEL_FRAME_MAX, sizeof(SpotVoxelBuffer), true);
	pointLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, POINT_VOXEL_FRAME_MAX, sizeof(PointVoxelBuffer), true);
	dirLightInjectBuffer = Buffer::CreateStructedBuffer(DEVICE, LIGHT_DIR_FRAME_MAX, sizeof(DirVoxelBuffer), true);

	voxelInjectLight = new Compute( COMPUTE_VOXEL_INJECT_LIGHT );
	voxelDownsample = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE );
	voxelDownsampleMove = new Compute( COMPUTE_VOXEL_DOWNSAMPLE_MOVE );

	return true;
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
	
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Height = viewport.Width = (float)VOXEL_VOLUME_RES;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	Render::RSSetViewports(1, &viewport);

	Render::SetTopology(IA_TOPOLOGY::TRISLIST);

	VolumeData constBuffer;
	constBuffer.cornerOffset.x = bigVolume.Center.x - bigVolume.Extents.x;
	constBuffer.cornerOffset.y = bigVolume.Center.y - bigVolume.Extents.y;
	constBuffer.cornerOffset.z = bigVolume.Center.z - bigVolume.Extents.z;
	constBuffer.worldSize = VOXEL_VOLUME_SIZE;
	constBuffer.scaleHelper = (float)VOXEL_VOLUME_RES / VOXEL_VOLUME_SIZE;
	constBuffer.volumeRes = VOXEL_VOLUME_RES;
	constBuffer.volumeDoubleRes = VOXEL_VOLUME_RES * 2;
	constBuffer.voxelSize = float(VOXEL_VOLUME_SIZE) / VOXEL_VOLUME_RES;
	constBuffer.voxelDiag = sqrt( constBuffer.voxelSize * constBuffer.voxelSize * 3 );

	// todo
	XMVECTOR camPoses[3];
	//camPoses[0] = XMVectorSet(0.0f, VOXEL_VOLUME_SIZE * 0.5f, VOXEL_VOLUME_SIZE * 0.5f, 1.0f);
	//camPoses[1] = XMVectorSet(VOXEL_VOLUME_SIZE * 0.5f, 0.0f, VOXEL_VOLUME_SIZE * 0.5f, 1.0f);
	//camPoses[2] = XMVectorSet(VOXEL_VOLUME_SIZE * 0.5f, VOXEL_VOLUME_SIZE * 0.5f, 0.0f, 1.0f);
	camPoses[0] = XMVectorSet(bigVolume.Center.x, bigVolume.Center.y, bigVolume.Center.z, 1.0f) + 
		XMVectorSet(0.0f, bigVolume.Extents.y, bigVolume.Extents.z, 0.0f);
	camPoses[1] = XMVectorSet(bigVolume.Center.x, bigVolume.Center.y, bigVolume.Center.z, 1.0f) + 
		XMVectorSet(bigVolume.Extents.x, 0.0f, bigVolume.Extents.z, 0.0f);
	camPoses[2] = XMVectorSet(bigVolume.Center.x, bigVolume.Center.y, bigVolume.Center.z, 1.0f) + 
		XMVectorSet(bigVolume.Extents.x, bigVolume.Extents.y, 0.0f, 0.0f);
	
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

	// draw
	const unsigned int offset = 0;
	for(auto& currentInstancesGroup: meshInstanceGroups)
	{
		auto matrixData = matrixPerMesh.begin() + currentInstancesGroup.matrixStart;
		Render::UpdateDynamicResource(instanceMatrixBuffer, matrixData, sizeof(StmMatrixBuffer) * currentInstancesGroup.instanceCount);

		Render::Context()->IASetVertexBuffers(0, 1, &(currentInstancesGroup.meshData->vertex_buffer), 
			&(currentInstancesGroup.meshData->vertex_size), &offset);
		Render::Context()->IASetIndexBuffer(currentInstancesGroup.meshData->index_buffer, DXGI_FORMAT_R32_UINT, 0);

		currentInstancesGroup.meshData->material->SetMatrixBuffer(instanceMatrixBuffer);
		currentInstancesGroup.meshData->material->Set(TECHNIQUES::TECHNIQUE_VOXEL);
		
		Render::Context()->DrawIndexedInstanced(currentInstancesGroup.meshData->index_count, currentInstancesGroup.instanceCount, 0, 0, 0);
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
	QSortSwap(meshesToRender.begin(), meshesToRender.end(), VoxelRenderer::CompareMeshes, 
		VoxelRenderer::SwapMeshes, &meshesToRender, &matrixPerMesh);

	uint32_t currentHash = 0;
	uint32_t instancesCount = 0;
	VCTInstanceGroup* currentInstance = nullptr;
	
	for(uint32_t mesh_i = 0; mesh_i < meshesToRender.size(); mesh_i++)
	{
		if( meshesToRender[mesh_i].meshHash == currentHash && instancesCount < VCT_MESH_MAX_INSTANCE)
		{
			instancesCount++;
			continue;
		}

		if(currentInstance)
			currentInstance->instanceCount = instancesCount;

		currentInstance = meshInstanceGroups.push_back();
		currentInstance->meshData = &meshesToRender[mesh_i];
		currentInstance->matrixStart = mesh_i;

		currentHash = meshesToRender[mesh_i].meshHash;
		instancesCount = 1;
	}

	if(currentInstance)
		currentInstance->instanceCount = instancesCount;
}

void VoxelRenderer::ProcessEmittance()
{
	PERF_GPU_TIMESTAMP(_LIGHTINJECT);

	Render::ClearUnorderedAccessViewFloat(voxelEmittanceUAV, XMFLOAT4(0,0,0,0));

	Render::UpdateDynamicResource(spotLightInjectBuffer.buf, spotVoxel_array.data(), spotVoxel_array.size() * sizeof(SpotVoxelBuffer));
	Render::UpdateDynamicResource(pointLightInjectBuffer.buf, pointVoxel_array.data(), pointVoxel_array.size() * sizeof(SpotVoxelBuffer));
	Render::UpdateDynamicResource(dirLightInjectBuffer.buf, dirVoxel_array.data(), dirVoxel_array.size() * sizeof(SpotVoxelBuffer));

	uint32_t lightCount[4] = {(uint32_t)spotVoxel_array.size(), (uint32_t)pointVoxel_array.size(), 
		(uint32_t)dirVoxel_array.size(), 0};
	Render::UpdateDynamicResource(volumeInfo, lightCount, sizeof(uint32_t) * 4);

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

	Render::CSSetConstantBuffers(0, 1, &volumeBuffer);
	Render::CSSetConstantBuffers(1, 1, &volumeInfo);

	voxelInjectLight->Dispatch( 8, 8, 8 );
	voxelInjectLight->UnbindUAV();
	
	PERF_GPU_TIMESTAMP(_VOXELDOWNSAMPLE);
	
	uint32_t downsampleBuffer[4] = {0, 0, 0, 0};
	uint32_t currentRes = VOXEL_VOLUME_RES / 2;
	ID3D11ShaderResourceView* null_srv = nullptr;

	Render::CSSetConstantBuffers(0, 1, &volumeBuffer);
	Render::CSSetConstantBuffers(1, 1, &volumeDownsampleBuffer);
	
	for(uint32_t level = 1; level < VOXEL_VOLUME_CLIPMAP_COUNT; level++)
	{
		Render::ClearUnorderedAccessViewFloat(voxelDownsampleTempUAV, XMFLOAT4(0,0,0,0));

		downsampleBuffer[0] = level;
		downsampleBuffer[1] = currentRes;
		Render::UpdateDynamicResource(volumeDownsampleBuffer, downsampleBuffer, sizeof(uint32_t) * 4);

		// downsample
		voxelDownsample->BindUAV(voxelDownsampleTempUAV);
		Render::CSSetShaderResources(0, 1, &voxelEmittanceSRV);
				
		voxelDownsample->Dispatch( currentRes, currentRes * 6, currentRes );

		voxelDownsample->UnbindUAV();
		Render::CSSetShaderResources(0, 1, &null_srv);

		// move data
		voxelDownsampleMove->BindUAV(voxelEmittanceUAV);
		Render::CSSetShaderResources(0, 1, &voxelDownsampleTempSRV);

		voxelDownsampleMove->Dispatch( currentRes, currentRes * 6, currentRes );

		voxelDownsampleMove->UnbindUAV();
		Render::CSSetShaderResources(0, 1, &null_srv);

		currentRes /= 2;
	}

}

void VoxelRenderer::RegMeshForVCT(uint32_t& index_count, uint32_t&& vertex_size, ID3D11Buffer* index_buffer, ID3D11Buffer* vertex_buffer, Material* material, StmMatrixBuffer& matrixData)
{
	if(meshesToRender.full())
		return;

	bool has_tq = false;
	auto queue = material->GetTechQueue(TECHNIQUES::TECHNIQUE_VOXEL, &has_tq);
	if(!has_tq)
		return;

	auto meshPtr = meshesToRender.push_back();
	auto matixPtr = matrixPerMesh.push_back();

	meshPtr->index_count = index_count;
	meshPtr->vertex_size = vertex_size;
	meshPtr->index_buffer = index_buffer;
	meshPtr->vertex_buffer = vertex_buffer;
	meshPtr->material = material;
	meshPtr->arrayID = (uint32_t)meshesToRender.size() - 1;

	matixPtr->world = matrixData.world;
	matixPtr->norm = matrixData.norm;

	meshPtr->meshHash = calcMeshHash(meshPtr);
}

void VoxelRenderer::CalcVolumeBox(XMVECTOR& camPos)
{
	XMStoreFloat3(&bigVolume.Center, camPos);
	bigVolume.Extents = XMFLOAT3(5.0f, 5.0f, 5.0f);
}