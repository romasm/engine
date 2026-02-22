#include "stdafx.h"
#include "CubeRenderTarget.h"
#include "RHI\DX11\DX11Types.h"

using namespace EngineCore;

CubeRenderTarget::CubeRenderTarget()
{
	UAV = nullptr;
	faces = nullptr;
	SRV = nullptr;
	resolution = 0;
	mipsCount = 0;
	arraySize = 0;
}

bool CubeRenderTarget::Init(int32_t res, DXGI_FORMAT fmt, bool hasMipChain, uint32_t arrayCount)
{
	if(faces)
		Close();

	arraySize = max((uint32_t)1, arrayCount);

	resolution = res;

	if(hasMipChain)
		mipsCount = GetLog2(resolution);
	else
		mipsCount = 1;

	// Create cubemap texture via raw DX11 (cubemap misc flag not handled by RHI CreateTexture)
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = resolution;
	textureDesc.Height = resolution;
	textureDesc.MipLevels = mipsCount;
	textureDesc.ArraySize = 6 * arraySize;
	textureDesc.Format = fmt;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = (hasMipChain ? D3D11_BIND_RENDER_TARGET : 0) | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = (hasMipChain ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0) | D3D11_RESOURCE_MISC_TEXTURECUBE;
	ID3D11Texture2D* rawTexture = nullptr;
	if( FAILED(Render::Device()->CreateTexture2D(&textureDesc, NULL, &rawTexture)) )
		return false;
	auto* wrappedTexture = new RHI::DX11::DX11Texture();
	wrappedTexture->texture2D = rawTexture;
	wrappedTexture->width = resolution;
	wrappedTexture->height = resolution;
	wrappedTexture->depth = 6 * arraySize;
	wrappedTexture->mipLevels = mipsCount;
	wrappedTexture->format = fmt;
	faces = wrappedTexture;

	// SRV: cubemap or cubemap array (raw DX11 creation + wrap)
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = fmt;
	shaderResourceViewDesc.ViewDimension = arraySize > 1 ? D3D11_SRV_DIMENSION_TEXTURECUBEARRAY : D3D11_SRV_DIMENSION_TEXTURECUBE;
	shaderResourceViewDesc.TextureCube.MipLevels = mipsCount;
	shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;

	shaderResourceViewDesc.TextureCubeArray.MipLevels = mipsCount;
	shaderResourceViewDesc.TextureCubeArray.MostDetailedMip = 0;
	shaderResourceViewDesc.TextureCubeArray.First2DArrayFace = 0;
	shaderResourceViewDesc.TextureCubeArray.NumCubes = arraySize;

	{
		ID3D11ShaderResourceView* rawSRV = nullptr;
		if( FAILED(Render::Device()->CreateShaderResourceView(rawTexture, &shaderResourceViewDesc, &rawSRV)) )
			return false;
		auto* wrapped = new RHI::DX11::DX11SRV();
		wrapped->view = rawSRV;
		SRV = wrapped;
	}

	// Per-face UAVs: Texture2DArray per-slice (raw DX11 creation + wrap in DX11UAV)
	UAV = new RHI::GfxUAV*[6 * arraySize];

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVdesc;
	ZeroMemory(&UAVdesc, sizeof(UAVdesc));
	UAVdesc.Format = fmt;
	UAVdesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	UAVdesc.Texture2DArray.MipSlice = 0;
	UAVdesc.Texture2DArray.ArraySize = 1;

	for (uint32_t k = 0; k < arraySize; k++)
	{
		for (int32_t i = 0; i < 6; i++)
		{
			UAVdesc.Texture2DArray.FirstArraySlice = k * 6 + i;
			ID3D11UnorderedAccessView* rawUAV = nullptr;
			if (FAILED(Render::Device()->CreateUnorderedAccessView(rawTexture, &UAVdesc, &rawUAV)))
				return false;
			auto* wrappedUAV = new RHI::DX11::DX11UAV();
			wrappedUAV->view = rawUAV;
			UAV[UAVdesc.Texture2DArray.FirstArraySlice] = wrappedUAV;
		}
	}

	return true;
}

void CubeRenderTarget::Close()
{
	if (UAV)
	{
		for (uint32_t i = 0; i < 6 * arraySize; i++)
			_DELETE(UAV[i]);
		_DELETE_ARRAY(UAV);
	}

	_DELETE(SRV);
	_DELETE(faces);
}

void CubeRenderTarget::GenerateMips()
{
	if(SRV)
		GFX_CMD->GenerateMips(SRV);
}

void CubeRenderTarget::ClearCube(uint32_t arrayID, float red, float green, float blue, float alpha)
{
	const uint32_t arrayID2D = arrayID * 6;
	for(uint32_t i = arrayID2D; i < arrayID2D + 6; i++)
		GFX_CMD->ClearUAVFloat(UAV[i], red, green, blue, alpha);
}

void CubeRenderTarget::ClearCubeArray(float red, float green, float blue, float alpha)
{
	for (uint32_t i = 0; i < arraySize * 6; i++)
		GFX_CMD->ClearUAVFloat(UAV[i], red, green, blue, alpha);
}