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

	computeHistoryStepBack = nullptr;
	computeHistoryStepForward = nullptr;

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

	_DELETE(computeHistoryStepBack);
	_DELETE(computeHistoryStepForward);
}

bool VolumePainter::Init(uint32_t width, uint32_t height, uint32_t depth, uint32_t historyBufferSizeMB)
{
	volumeResolutionX = width;
	volumeResolutionY = height;
	volumeResolutionZ = depth;
	
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

	computeHistoryStepBack = new Compute(COMPUTE_HISTORY_STEP_BACK);
	computeHistoryStepBack->AttachRWResource("volumeRW", volumeTextureUAV);
	computeHistoryStepBack->AttachRWResource("volumeDiff", volumeDifferenceUAV);
	computeHistoryStepBack->AttachConstantBuffer("volumeInfo", volumeInfoBuffer);
	
	computeHistoryStepForward = new Compute(COMPUTE_HISTORY_STEP_FORWARD);
	computeHistoryStepForward->AttachRWResource("volumeRW", volumeTextureUAV);
	computeHistoryStepForward->AttachRWResource("volumeDiff", volumeDifferenceUAV);
	computeHistoryStepForward->AttachConstantBuffer("volumeInfo", volumeInfoBuffer);
	
	history.create(HISTORY_LENGTH);
	historyMaxSize = (int64_t)historyBufferSizeMB * 1024 * 1024;
	historySize = 0;
	historyMark = 0;

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

	computeDrawBrush->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void VolumePainter::PushDifference(Vector3& minCorner, Vector3& maxCorner)
{
	Vector3 minCornerClamped = Vector3::Max(minCorner, Vector3(0, 0, 0));
	Vector3 maxCornerClamped = Vector3::Min(maxCorner, Vector3((float)volumeResolutionX, (float)volumeResolutionY, (float)volumeResolutionZ));

	Vector3 checkArea = maxCornerClamped - minCornerClamped;
	if (checkArea.x <= 0 || checkArea.y <= 0 || checkArea.z <= 0)
		return;
	
	// if historyMark not last, erase all history after historyMark
	while (historyMark < (int32_t)history.size())
	{
		VolumeDiff& dropedArea = history.back();

		historySize -= dropedArea.GetVoxelsSize();
		historySize = max((int64_t)0, historySize);

		dropedArea.Clear();
		history.erase(history.size() - 1);
	}
			
	VolumeDiff& volumeArea = history.push_back();
	volumeArea.Init(minCornerClamped, maxCornerClamped);

	historyMark = (int32_t)history.size();

	uint64_t voxelsSize = volumeArea.GetVoxelsSize();
	
	historySize += voxelsSize;
	while (historySize > historyMaxSize && history.size() > 1)
	{
		VolumeDiff& dropedArea = history.front();

		historySize -= dropedArea.GetVoxelsSize();
		historySize = max((int64_t)0, historySize);

		dropedArea.Clear();
		history.pop_front();
	}

	D3D11_BOX volumeBox = volumeArea.GetD3DBox();
	uint32_t rowPitch = volumeArea.resX * VOXEL_DATA_SIZE;
	uint32_t depthPitch = rowPitch * volumeArea.resY;
	
	if (FAILED(CONTEXT3->Map(volumeDifference, 0, D3D11_MAP_READ, 0, NULL)))
	{
		ERR("Cant map volume difference to CPU");
		return;
	}

	DEVICE3->ReadFromSubresource(volumeArea.data, rowPitch, depthPitch, volumeDifference, 0, &volumeBox);	
	CONTEXT3->Unmap(volumeDifference, 0);

	Render::ClearUnorderedAccessViewFloat(volumeDifferenceUAV, Vector4(0, 0, 0, 0));

	LOG("Difference pushed, history size = %i MB", int32_t(historySize / (1024 * 1024)));
}

void VolumePainter::HistoryStepBack() 
{
	if (history.empty() || historyMark == 0)
		return;

	// send diff to gpu
	VolumeDiff& volumeArea = history[historyMark - 1];

	D3D11_BOX volumeBox = volumeArea.GetD3DBox();
	uint32_t rowPitch = volumeArea.resX * VOXEL_DATA_SIZE;
	uint32_t depthPitch = rowPitch * volumeArea.resY;

	if (FAILED(CONTEXT3->Map(volumeDifference, 0, D3D11_MAP_WRITE, 0, NULL)))
	{
		ERR("Cant map volume difference to CPU");
		return;
	}

	DEVICE3->WriteToSubresource(volumeDifference, 0, &volumeBox, volumeArea.data, rowPitch, depthPitch);
	CONTEXT3->Unmap(volumeDifference, 0);

	// execute compute to do step back
	VolumeInfo volumeInfo;
	volumeInfo.minCorner = Vector3((float)volumeArea.minX, (float)volumeArea.minY, (float)volumeArea.minZ);
	volumeInfo.size = Vector3((float)volumeArea.resX, (float)volumeArea.resY, (float)volumeArea.resZ);
	Render::UpdateDynamicResource(volumeInfoBuffer, &volumeInfo, sizeof(VolumeInfo));

	uint32_t groupCountX = (uint32_t)ceil(volumeInfo.size.x / COPMUTE_TREADS_X);
	uint32_t groupCountY = (uint32_t)ceil(volumeInfo.size.y / COPMUTE_TREADS_Y);
	uint32_t groupCountZ = (uint32_t)ceil(volumeInfo.size.z / COPMUTE_TREADS_Z);

	computeHistoryStepBack->Dispatch(groupCountX, groupCountY, groupCountZ);

	historyMark--;

	LOG("History changed to %i", historyMark);
}

void VolumePainter::HistoryStepForward()
{
	if (historyMark == (int32_t)history.size())
		return;

	// execute compute to do step forward
	VolumeDiff& volumeArea = history[historyMark];

	VolumeInfo volumeInfo;
	volumeInfo.minCorner = Vector3((float)volumeArea.minX, (float)volumeArea.minY, (float)volumeArea.minZ);
	volumeInfo.size = Vector3((float)volumeArea.resX, (float)volumeArea.resY, (float)volumeArea.resZ);
	Render::UpdateDynamicResource(volumeInfoBuffer, &volumeInfo, sizeof(VolumeInfo));

	uint32_t groupCountX = (uint32_t)ceil(volumeInfo.size.x / COPMUTE_TREADS_X);
	uint32_t groupCountY = (uint32_t)ceil(volumeInfo.size.y / COPMUTE_TREADS_Y);
	uint32_t groupCountZ = (uint32_t)ceil(volumeInfo.size.z / COPMUTE_TREADS_Z);

	computeHistoryStepForward->Dispatch(groupCountX, groupCountY, groupCountZ);
	
	historyMark++;

	// if not last step - send next diff to gpu
	if (historyMark < (int32_t)history.size())
	{
		VolumeDiff& volumeAreaNext = history[historyMark];

		D3D11_BOX volumeBox = volumeAreaNext.GetD3DBox();
		uint32_t rowPitch = volumeAreaNext.resX * VOXEL_DATA_SIZE;
		uint32_t depthPitch = rowPitch * volumeAreaNext.resY;

		if (FAILED(CONTEXT3->Map(volumeDifference, 0, D3D11_MAP_WRITE, 0, NULL)))
		{
			ERR("Cant map volume difference to CPU");
			return;
		}

		DEVICE3->WriteToSubresource(volumeDifference, 0, &volumeBox, volumeAreaNext.data, rowPitch, depthPitch);
		CONTEXT3->Unmap(volumeDifference, 0);
	}

	LOG("History changed to %i", historyMark);
}