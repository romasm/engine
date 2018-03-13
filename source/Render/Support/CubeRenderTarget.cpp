#include "stdafx.h"
#include "CubeRenderTarget.h"

using namespace EngineCore;

CubeRenderTarget::CubeRenderTarget()
{
	for(int32_t i = 0; i < 6; i++)
		UAV[i] = nullptr;

	faces = nullptr;
	SRV = nullptr;
	resolution = 0;
	mipsCount = 0;
}

bool CubeRenderTarget::Init(int32_t res, DXGI_FORMAT fmt, bool hasMipChain)
{
	if(faces)
		Close();

	resolution = res;
	
	if(hasMipChain)
		mipsCount = GetLog2(resolution);
	else
		mipsCount = 1;

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = resolution;
	textureDesc.Height = resolution;
	textureDesc.MipLevels = mipsCount;
	textureDesc.ArraySize = 6;
	textureDesc.Format = fmt;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = (hasMipChain ? D3D11_BIND_RENDER_TARGET : 0) | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = (hasMipChain ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0) | D3D11_RESOURCE_MISC_TEXTURECUBE;
	if( FAILED(Render::CreateTexture2D(&textureDesc, NULL, &faces)) )
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = fmt;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	shaderResourceViewDesc.TextureCube.MipLevels = mipsCount;
	shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
	if( FAILED(Render::CreateShaderResourceView(faces, &shaderResourceViewDesc, &SRV)) )
		return false;

	for(int32_t i = 0; i < 6; i++)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVdesc;
		ZeroMemory( &UAVdesc, sizeof(UAVdesc));
		UAVdesc.Format = fmt;
		UAVdesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		UAVdesc.Texture2DArray.MipSlice = 0;
		UAVdesc.Texture2DArray.ArraySize = 1;
		UAVdesc.Texture2DArray.FirstArraySlice = i;
		if( FAILED(Render::CreateUnorderedAccessView(faces, &UAVdesc, &UAV[i])) )
			return false;
	}

	return true;
}

void CubeRenderTarget::Close()
{
	for(int32_t i = 0; i < 6; i++)
		_RELEASE(UAV[i]);

	_RELEASE(SRV);
	_RELEASE(faces);
}

void CubeRenderTarget::ClearCube(float red, float green, float blue, float alpha)
{
	for(int32_t i = 0; i < 6; i++)
		Render::ClearUnorderedAccessViewFloat(UAV[i], Vector4(red, green, blue, alpha));
}