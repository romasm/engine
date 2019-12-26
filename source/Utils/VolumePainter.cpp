#include "stdafx.h"

#include "VolumePainter.h"
#include "Render.h"
#include "TexMgr.h"

using namespace EngineCore;

VolumePainter::VolumePainter()
{
	volumeTexture = nullptr;
	volumeTextureUAV = nullptr;
	volumeTextureSRV = nullptr;

	volumeDifference = nullptr;
	volumeDifferenceUAV = nullptr;

	computeImportTexture = nullptr;
	computeDrawBrush = nullptr;

	//volumeData = nullptr;

	historySize = 0;
	historyMaxSize = 0;
}

VolumePainter::~VolumePainter()
{
	_RELEASE(volumeTextureUAV);
	_RELEASE(volumeTextureSRV);
	_RELEASE(volumeTexture);

	_RELEASE(volumeDifferenceUAV);
	_RELEASE(volumeDifference);

	_DELETE(computeImportTexture);
	_DELETE(computeDrawBrush);

	//_DELETE_ARRAY(volumeData);
}

bool VolumePainter::Init(uint32_t width, uint32_t height, uint32_t depth, uint32_t historyBufferSize)
{
	volumeResolutionX = width;
	volumeResolutionY = height;
	volumeResolutionZ = depth;

	//int32_t volumeSize = volumeResolutionX * volumeResolutionY * volumeResolutionZ * VOXEL_DATA_SIZE;
	//volumeData = new uint8_t[volumeSize];

	D3D11_TEXTURE3D_DESC volumeDesc;
	ZeroMemory(&volumeDesc, sizeof(volumeDesc));
	volumeDesc.Width = volumeResolutionX;
	volumeDesc.Height = volumeResolutionY;
	volumeDesc.Depth = volumeResolutionZ;
	volumeDesc.MipLevels = 1;
	volumeDesc.Usage = D3D11_USAGE_DEFAULT;
	volumeDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	volumeDesc.CPUAccessFlags = 0;
	volumeDesc.MiscFlags = 0;
	volumeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	if (FAILED(Render::CreateTexture3D(&volumeDesc, NULL, &volumeTexture)))
	{
		ERR("VolumePainter -> volume texture CreateTexture3D failed");
		return false;
	}

	volumeDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	volumeDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	if (FAILED(DEVICE3->CreateTexture3D(&volumeDesc, NULL, &volumeDifference)))
	{
		ERR("VolumePainter -> volume difference CreateTexture3D failed");
		return false;
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC volumeUAVDesc;
	ZeroMemory(&volumeUAVDesc, sizeof(volumeUAVDesc));
	volumeUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	volumeUAVDesc.Texture3D.MipSlice = 0;
	volumeUAVDesc.Texture3D.WSize = volumeResolutionZ;
	volumeUAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	if (FAILED(Render::CreateUnorderedAccessView(volumeTexture, &volumeUAVDesc, &volumeTextureUAV)))
	{
		ERR("VolumePainter -> volume texture CreateUnorderedAccessView failed");
		return false;
	}

	if (FAILED(Render::CreateUnorderedAccessView(volumeDifference, &volumeUAVDesc, &volumeDifferenceUAV)))
	{
		ERR("VolumePainter -> volume difference CreateUnorderedAccessView failed");
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC volumeSRVDesc;
	ZeroMemory(&volumeSRVDesc, sizeof(volumeSRVDesc));
	volumeSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	volumeSRVDesc.Texture3D.MipLevels = -1;
	volumeSRVDesc.Texture3D.MostDetailedMip = 0;
	volumeSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	if (FAILED(Render::CreateShaderResourceView(volumeTexture, &volumeSRVDesc, &volumeTextureSRV)))
	{
		ERR("VolumePainter -> volume texture CreateShaderResourceView failed");
		return false;
	}

	volumeInfoBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeInfo), true);
	
	brushInfoBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(BrushInfo), true);

	computeImportTexture = new Compute(COMPUTE_IMPORT_TEXTURE);
	computeImportTexture->AttachRWResource("volumeRW", volumeTextureUAV);
	computeImportTexture->AttachRWResource("volumeDiff", volumeDifferenceUAV);
	computeImportTexture->AttachConstantBuffer("volumeInfo", volumeInfoBuffer);

	computeDrawBrush = new Compute(COMPUTE_DRAW_BRUSH);
	computeDrawBrush->AttachRWResource("volumeRW", volumeTextureUAV);
	computeDrawBrush->AttachRWResource("volumeDiff", volumeDifferenceUAV);
	computeDrawBrush->AttachConstantBuffer("volumeInfo", volumeInfoBuffer);
	computeDrawBrush->AttachConstantBuffer("brushInfo", brushInfoBuffer);

	history.create(HISTORY_LENGTH);
	historyMaxSize = historyBufferSize;
	historySize = 0;

	return true;
}

void VolumePainter::ImportTexture(string textureName)
{	
	auto copyCallback = [this](uint32_t id, bool status) -> void
	{
		uint32_t groupCountX = (uint32_t)ceil(float(volumeResolutionX) / COPMUTE_TREADS_X);
		uint32_t groupCountY = (uint32_t)ceil(float(volumeResolutionY) / COPMUTE_TREADS_Y);
		uint32_t groupCountZ = (uint32_t)ceil(float(volumeResolutionZ) / COPMUTE_TREADS_Z);

		VolumeInfo volumeInfo;
		volumeInfo.minCorner = Vector3(0, 0, 0);
		volumeInfo.sizeInv = Vector3(1.0f / volumeResolutionX, 1.0f / volumeResolutionY, 1.0f / volumeResolutionZ);
		Render::UpdateDynamicResource(volumeInfoBuffer, &volumeInfo, sizeof(VolumeInfo));

		computeImportTexture->AttachResource("volumeTexture", TexMgr::GetResourcePtr(id));

		computeImportTexture->Dispatch(groupCountX, groupCountY, groupCountZ);

		TexMgr::Get()->DeleteResource(id);
	};

	TexMgr::Get()->GetResource(textureName, false, copyCallback);
}

void VolumePainter::ExportTexture(string textureName, int32_t packingType, int32_t storageType)
{
	// TODO: rerender texture

	TexLoader::SaveTexture(textureName, volumeTextureSRV);
}

void VolumePainter::DrawBrush(Vector3& prevPosition, Vector3& position, float radius, Vector4& colorOpacity, float hardness)
{
	BrushInfo brushInfo;
	brushInfo.position = position;
	brushInfo.prevPosition = prevPosition;
	brushInfo.radius = radius;
	brushInfo.colorOpacity = colorOpacity;
	brushInfo.hardness = min(hardness, 0.999f);
	Render::UpdateDynamicResource(brushInfoBuffer, &brushInfo, sizeof(BrushInfo));

	Vector3 minCorner = Vector3::Min(position, prevPosition);
	minCorner -= Vector3(radius);
	minCorner.x = floorf(minCorner.x);
	minCorner.y = floorf(minCorner.y);
	minCorner.z = floorf(minCorner.z);

	if (minCorner.x >= float(volumeResolutionX) || minCorner.y >= float(volumeResolutionY) || minCorner.z >= float(volumeResolutionZ))
		return;

	Vector3 maxCorner = Vector3::Max(position, prevPosition);
	maxCorner += Vector3(radius);
	maxCorner.x = ceilf(maxCorner.x);
	maxCorner.y = ceilf(maxCorner.y);
	maxCorner.z = ceilf(maxCorner.z);

	if (maxCorner.x <= 0 || maxCorner.y <= 0 || maxCorner.z <= 0)
		return;

	minCorner = Vector3::Max(minCorner, Vector3(0.0));
	maxCorner = Vector3::Min(maxCorner, Vector3(float(volumeResolutionX), float(volumeResolutionY), float(volumeResolutionZ)));

	Vector3 boxSize = maxCorner - minCorner;
	if (boxSize.x <= 0 || boxSize.y <= 0 || boxSize.z <= 0)
		return;

	VolumeInfo volumeInfo;
	volumeInfo.minCorner = minCorner;
	volumeInfo.size = boxSize;
	Render::UpdateDynamicResource(volumeInfoBuffer, &volumeInfo, sizeof(VolumeInfo));

	uint32_t groupCountX = (uint32_t)ceil(boxSize.x / COPMUTE_TREADS_X);
	uint32_t groupCountY = (uint32_t)ceil(boxSize.y / COPMUTE_TREADS_Y);
	uint32_t groupCountZ = (uint32_t)ceil(boxSize.z / COPMUTE_TREADS_Z);

	//LOG("%u %u %u = %i %i %i", groupCountX, groupCountY, groupCountZ, int32_t(minCorner.x), int32_t(minCorner.y), int32_t(minCorner.z));

	computeDrawBrush->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void VolumePainter::PushDifference(Vector3& minCorner, Vector3& maxCorner)
{
	VolumeDiff& volumeArea = history.push_back();
	volumeArea.Init(minCorner, maxCorner);

	int32_t voxelsSize = volumeArea.GetVoxelsSize();

	historySize += voxelsSize;
	if (historySize > historyMaxSize && history.size() > 1)
	{
		VolumeDiff& dropedArea = history.front();

		historySize -= dropedArea.GetVoxelsSize();

		dropedArea.Clear();
		history.pop_front();
	}

	//CONTEXT->CopyResource(volumeDifferenceCopy, volumeDifference);

	if (FAILED(CONTEXT3->Map(volumeDifference, 0, D3D11_MAP_WRITE, 0, NULL/*&mappedVolume*/)))
	{
		ERR("Cant map volume difference to CPU");
		return;
	}

	//DEVICE3->ReadFromSubresource();

	//StoreDifference(volumeArea, (uint8_t*)mappedVolume.pData);
	
	CONTEXT3->Unmap(volumeDifference, 0);

	Render::ClearUnorderedAccessViewFloat(volumeDifferenceUAV, Vector4(0, 0, 0, 0));
}

void VolumePainter::StoreDifference(VolumeDiff& area, uint8_t* difference)
{
	int32_t offsetSource = area.minZ * (volumeResolutionX * volumeResolutionY) + area.minY * volumeResolutionX + area.minX;
	int32_t offsetArea = 0;

	int32_t copySize = area.resX * VOXEL_DATA_SIZE;

	for (int32_t z = area.minZ; z <= area.maxZ; z++)
	{
		int32_t offsetSourceZ = offsetSource + z * (volumeResolutionX * volumeResolutionY);

		for (int32_t y = area.minY; y <= area.maxY; y++)
		{
			int32_t offsetSourceZY = offsetSourceZ + y * volumeResolutionX;

			memcpy(area.data + offsetArea * VOXEL_DATA_SIZE, difference + offsetSourceZY * VOXEL_DATA_SIZE, copySize);

			offsetArea += area.resX;
		}
	}
}