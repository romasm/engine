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
	volumeResolutionX = depth;

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
	volumeDesc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
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
	volumeUAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
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

	computeImportTexture = new Compute(COMPUTE_IMPORT_TEXTURE);
	computeImportTexture->AttachRWResource("volumeRW", volumeTextureUAV);

	//computeDrawBrush = new Compute(COMPUTE_DRAW_BRUSH);
}

bool VolumePainter::ImportTexture(string textureName)
{	
	auto copyCallback = [this](uint32_t id, bool status) -> void
	{
		uint32_t groupCountX = (uint32_t)ceil(float(volumeResolutionX) / COPMUTE_TREADS_X);
		uint32_t groupCountY = (uint32_t)ceil(float(volumeResolutionX) / COPMUTE_TREADS_Y);
		uint32_t groupCountZ = (uint32_t)ceil(float(volumeResolutionX) / COPMUTE_TREADS_Z);

		computeImportTexture->AttachResource("volumeTexture", TexMgr::GetResourcePtr(id));

		computeImportTexture->Dispatch(groupCountX, groupCountY, groupCountZ);

		TexMgr::Get()->DeleteResource(id);
	};

	TexMgr::Get()->GetResource(textureName, false, copyCallback);
}