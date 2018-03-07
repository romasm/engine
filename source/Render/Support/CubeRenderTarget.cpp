#include "stdafx.h"
#include "CubeRenderTarget.h"

using namespace EngineCore;

CubeRenderTarget::CubeRenderTarget()
{
	for(int32_t i = 0; i < 6; i++)
		RTV[i] = nullptr;

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
	
	viewport.Width = (float)resolution;
	viewport.Height = (float)resolution;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

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
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
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
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		renderTargetViewDesc.Format = fmt;
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		renderTargetViewDesc.Texture2DArray.ArraySize = 1;
		renderTargetViewDesc.Texture2DArray.FirstArraySlice = i;
		renderTargetViewDesc.Texture2DArray.MipSlice = 0;
		if( FAILED(Render::CreateRenderTargetView(faces, &renderTargetViewDesc, &RTV[i])) )
			return false;
	}

	return true;
}

void CubeRenderTarget::Close()
{
	for(int32_t i = 0; i < 6; i++)
		_RELEASE(RTV[i]);

	_RELEASE(SRV);
	_RELEASE(faces);
}

void CubeRenderTarget::SetRenderTarget(uint32_t face)
{
	Render::OMSetRenderTargets(1, &(RTV[face]), nullptr);
	Render::RSSetViewports(1, &viewport);
}

void CubeRenderTarget::ClearCube(float red, float green, float blue, float alpha)
{
	for(int32_t i = 0; i < 6; i++)
		Render::ClearRenderTargetView(RTV[i], Vector4(red, green, blue, alpha));
}