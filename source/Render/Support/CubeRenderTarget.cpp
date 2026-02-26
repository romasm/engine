#include "stdafx.h"
#include "CubeRenderTarget.h"
#include "Render.h"

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
	mipsCount = hasMipChain ? GetLog2(resolution) : 1;

	const auto dimension = (arraySize > 1)
		? RHI::TextureDimension::CubeMapArray
		: RHI::TextureDimension::CubeMap;

	RHI::TextureDesc texDesc = {};
	texDesc.dimension    = dimension;
	texDesc.width        = resolution;
	texDesc.height       = resolution;
	texDesc.depth        = arraySize; // number of cubes
	texDesc.mipLevels    = mipsCount;
	texDesc.format       = fmt;
	texDesc.allowSRV     = true;
	texDesc.allowUAV     = true;
	texDesc.allowRTV     = hasMipChain;
	texDesc.generateMips = hasMipChain;
	faces = GFX_DEVICE->CreateTexture(texDesc);
	if(!faces) return false;

	SRV = GFX_DEVICE->CreateSRV(faces, fmt, 0, mipsCount);
	if(!SRV) return false;

	// Per-face UAVs (one per cube face per array element)
	const uint32_t totalFaces = 6 * arraySize;
	UAV = new RHI::GfxUAV*[totalFaces];
	for(uint32_t faceIndex = 0; faceIndex < totalFaces; faceIndex++)
	{
		UAV[faceIndex] = GFX_DEVICE->CreateUAV(faces, fmt, 0, faceIndex);
		if(!UAV[faceIndex]) return false;
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