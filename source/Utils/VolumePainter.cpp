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

	computeImportTexture = nullptr;
	computeDrawBrush = nullptr;
}

VolumePainter::~VolumePainter()
{
	_RELEASE(volumeTextureUAV);
	_RELEASE(volumeTextureSRV);
	_RELEASE(volumeTexture);

	_DELETE(computeImportTexture);
	_DELETE(computeDrawBrush);
}

bool VolumePainter::Init(uint32_t width, uint32_t height, uint32_t depth)
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
		ERR("VolumePainter -> CreateTexture3D failed");
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
		ERR("VolumePainter -> CreateUnorderedAccessView failed");
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
		ERR("VolumePainter -> CreateShaderResourceView failed");
		return false;
	}

	volumeInfoBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(VolumeInfo), true);
	
	brushInfoBuffer = Buffer::CreateConstantBuffer(DEVICE, sizeof(BrushInfo), true);

	computeImportTexture = new Compute(COMPUTE_IMPORT_TEXTURE);
	computeImportTexture->AttachRWResource("volumeRW", volumeTextureUAV);
	computeImportTexture->AttachConstantBuffer("volumeInfo", volumeInfoBuffer);

	computeDrawBrush = new Compute(COMPUTE_DRAW_BRUSH);
	computeDrawBrush->AttachRWResource("volumeRW", volumeTextureUAV);
	computeDrawBrush->AttachConstantBuffer("volumeInfo", volumeInfoBuffer);
	computeDrawBrush->AttachConstantBuffer("brushInfo", brushInfoBuffer);

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

void VolumePainter::DrawBrush(Vector3& position, float radius)
{
	BrushInfo brushInfo;
	brushInfo.position = position;
	brushInfo.radius = radius;
	Render::UpdateDynamicResource(brushInfoBuffer, &brushInfo, sizeof(BrushInfo));

	Vector3 minCorner = position;
	minCorner -= Vector3(radius);
	minCorner.x = floorf(minCorner.x);
	minCorner.y = floorf(minCorner.y);
	minCorner.z = floorf(minCorner.z);

	if (minCorner.x >= float(volumeResolutionX) || minCorner.y >= float(volumeResolutionY) || minCorner.z >= float(volumeResolutionZ))
		return;

	Vector3 boxSize = Vector3(floorf(radius * 2.0f));

	Vector3 negativeCorner;
	Vector3::Min(minCorner, Vector3(0.0), negativeCorner);
	boxSize = boxSize + negativeCorner;
	if (boxSize.x <= 0 || boxSize.y <= 0 || boxSize.z <= 0)
		return;

	Vector3::Max(minCorner, Vector3(0.0), minCorner);
	Vector3 maxCorner = minCorner + boxSize;

	Vector3 positiveCorner = maxCorner - Vector3(float(volumeResolutionX), float(volumeResolutionY), float(volumeResolutionZ));
	Vector3::Max(positiveCorner, Vector3(0.0), positiveCorner);
	boxSize = boxSize - positiveCorner;
	if (boxSize.x <= 0 || boxSize.y <= 0 || boxSize.z <= 0)
		return;

	VolumeInfo volumeInfo;
	volumeInfo.minCorner = minCorner;
	volumeInfo.size = boxSize;
	Render::UpdateDynamicResource(volumeInfoBuffer, &volumeInfo, sizeof(VolumeInfo));

	uint32_t groupCountX = (uint32_t)ceil(boxSize.x / COPMUTE_TREADS_X);
	uint32_t groupCountY = (uint32_t)ceil(boxSize.y / COPMUTE_TREADS_Y);
	uint32_t groupCountZ = (uint32_t)ceil(boxSize.z / COPMUTE_TREADS_Z);

	LOG("%u %u %u = %i %i %i", groupCountX, groupCountY, groupCountZ, int32_t(minCorner.x), int32_t(minCorner.y), int32_t(minCorner.z));

	computeDrawBrush->Dispatch(groupCountX, groupCountY, groupCountZ);
}