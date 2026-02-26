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
	_DELETE(volumeTextureUAV);
	_DELETE(volumeTextureSRV);
	_DELETE(volumeTexture);

	_DELETE(volumeDifferenceUAV);
	_DELETE(volumeDifference);

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
	
	RHI::TextureDesc volumeDesc = {};
	volumeDesc.dimension = RHI::TextureDimension::Tex3D;
	volumeDesc.width = volumeResolutionX;
	volumeDesc.height = volumeResolutionY;
	volumeDesc.depth = volumeResolutionZ;
	volumeDesc.mipLevels = 1;
	volumeDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	volumeDesc.allowSRV = true;
	volumeDesc.allowUAV = true;
	volumeDesc.allowRTV = false;
	volumeDesc.allowDSV = false;
	volumeDesc.generateMips = false;
	volumeDesc.msaaSamples = 1;
	volumeDesc.msaaQuality = 0;
	volumeDesc.initialData = nullptr;

	volumeTexture = GFX_DEVICE->CreateTexture(volumeDesc);
	if (!volumeTexture)
	{
		ERR("VolumePainter -> volume texture CreateTexture failed");
		return false;
	}

	volumeDesc.format = DXGI_FORMAT_R16G16B16A16_SNORM;
	volumeDesc.allowSRV = false;
	volumeDifference = GFX_DEVICE->CreateTexture(volumeDesc);
	if (!volumeDifference)
	{
		ERR("VolumePainter -> volume difference CreateTexture failed");
		return false;
	}

	volumeTextureUAV = GFX_DEVICE->CreateUAV(volumeTexture, DXGI_FORMAT_R8G8B8A8_UNORM);
	if (!volumeTextureUAV)
	{
		ERR("VolumePainter -> volume texture CreateUAV failed");
		return false;
	}

	volumeDifferenceUAV = GFX_DEVICE->CreateUAV(volumeDifference, DXGI_FORMAT_R16G16B16A16_SNORM);
	if (!volumeDifferenceUAV)
	{
		ERR("VolumePainter -> volume difference CreateUAV failed");
		return false;
	}

	volumeTextureSRV = GFX_DEVICE->CreateSRV(volumeTexture, DXGI_FORMAT_R8G8B8A8_UNORM);
	if (!volumeTextureSRV)
	{
		ERR("VolumePainter -> volume texture CreateSRV failed");
		return false;
	}

	volumeInfoBuffer = Buffer::CreateConstantBuffer(sizeof(VolumeInfo), true);

	brushInfoBuffer = Buffer::CreateConstantBuffer(sizeof(BrushInfo), true);

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
		GFX_CMD->UpdateBuffer(volumeInfoBuffer, &volumeInfo, sizeof(VolumeInfo));

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

void VolumePainter::DrawBrush(Vector3& prevPosition, Vector3& position, float radius, Vector4& colorOpacity, float hardness, bool erase)
{
	BrushInfo brushInfo;
	brushInfo.position = position;
	brushInfo.prevPosition = prevPosition;
	brushInfo.radius = radius;
	brushInfo.colorOpacity = colorOpacity;
	brushInfo.hardness = min(hardness, 0.999f);
	brushInfo.erase = erase ? 1.0f : 0.0f;
	GFX_CMD->UpdateBuffer(brushInfoBuffer, &brushInfo, sizeof(BrushInfo));

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
	GFX_CMD->UpdateBuffer(volumeInfoBuffer, &volumeInfo, sizeof(VolumeInfo));

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

	RHI::GfxBox volumeBox = volumeArea.GetBox();
	uint32_t rowPitch = volumeArea.resX * VOXEL_DIFF_SIZE;
	uint32_t depthPitch = rowPitch * volumeArea.resY;

	if (FAILED(GFX_DEVICE->ReadTextureRegion(volumeDifference, 0, volumeBox, volumeArea.data, rowPitch, depthPitch)))
	{
		ERR("Cant read volume difference to CPU");
		return;
	}

	GFX_CMD->ClearUAVFloat(volumeDifferenceUAV, 0, 0, 0, 0);

	LOG("Difference pushed, history size = %i MB", int32_t(historySize / (1024 * 1024)));
}

void VolumePainter::HistoryStepBack() 
{
	if (history.empty() || historyMark == 0)
		return;

	// send diff to gpu
	VolumeDiff& volumeArea = history[historyMark - 1];

	RHI::GfxBox volumeBox = volumeArea.GetBox();
	uint32_t rowPitch = volumeArea.resX * VOXEL_DIFF_SIZE;
	uint32_t depthPitch = rowPitch * volumeArea.resY;

	if (FAILED(GFX_DEVICE->WriteTextureRegion(volumeDifference, 0, volumeBox, volumeArea.data, rowPitch, depthPitch)))
	{
		ERR("Cant write volume difference to GPU");
		return;
	}

	// execute compute to do step back
	VolumeInfo volumeInfo;
	volumeInfo.minCorner = Vector3((float)volumeArea.minX, (float)volumeArea.minY, (float)volumeArea.minZ);
	volumeInfo.size = Vector3((float)volumeArea.resX, (float)volumeArea.resY, (float)volumeArea.resZ);
	GFX_CMD->UpdateBuffer(volumeInfoBuffer, &volumeInfo, sizeof(VolumeInfo));

	uint32_t groupCountX = (uint32_t)ceil(volumeInfo.size.x / COPMUTE_TREADS_X);
	uint32_t groupCountY = (uint32_t)ceil(volumeInfo.size.y / COPMUTE_TREADS_Y);
	uint32_t groupCountZ = (uint32_t)ceil(volumeInfo.size.z / COPMUTE_TREADS_Z);

	computeHistoryStepBack->Dispatch(groupCountX, groupCountY, groupCountZ);

	GFX_CMD->ClearUAVFloat(volumeDifferenceUAV, 0, 0, 0, 0);

	historyMark--;

	LOG("History changed to %i", historyMark);
}

void VolumePainter::HistoryStepForward()
{
	if (historyMark == (int32_t)history.size())
		return;

	// send diff to gpu
	VolumeDiff& volumeArea = history[historyMark];

	RHI::GfxBox volumeBox = volumeArea.GetBox();
	uint32_t rowPitch = volumeArea.resX * VOXEL_DIFF_SIZE;
	uint32_t depthPitch = rowPitch * volumeArea.resY;

	if (FAILED(GFX_DEVICE->WriteTextureRegion(volumeDifference, 0, volumeBox, volumeArea.data, rowPitch, depthPitch)))
	{
		ERR("Cant write volume difference to GPU");
		return;
	}

	// execute compute to do step forward
	VolumeInfo volumeInfo;
	volumeInfo.minCorner = Vector3((float)volumeArea.minX, (float)volumeArea.minY, (float)volumeArea.minZ);
	volumeInfo.size = Vector3((float)volumeArea.resX, (float)volumeArea.resY, (float)volumeArea.resZ);
	GFX_CMD->UpdateBuffer(volumeInfoBuffer, &volumeInfo, sizeof(VolumeInfo));

	uint32_t groupCountX = (uint32_t)ceil(volumeInfo.size.x / COPMUTE_TREADS_X);
	uint32_t groupCountY = (uint32_t)ceil(volumeInfo.size.y / COPMUTE_TREADS_Y);
	uint32_t groupCountZ = (uint32_t)ceil(volumeInfo.size.z / COPMUTE_TREADS_Z);

	computeHistoryStepForward->Dispatch(groupCountX, groupCountY, groupCountZ);

	GFX_CMD->ClearUAVFloat(volumeDifferenceUAV, 0, 0, 0, 0);
	
	historyMark++;

	LOG("History changed to %i", historyMark);
}