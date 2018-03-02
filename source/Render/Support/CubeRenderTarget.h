#pragma once

#include "stdafx.h"
#include "Render.h"
#include "Common.h"
#include "Compute.h"

namespace EngineCore
{

class CubeRenderTarget
{
public:
	CubeRenderTarget();

	bool Init(int32_t w, int32_t h, DXGI_FORMAT fmt, bool hasMipChain = false);
	void Close();

	void SetRenderTarget(uint32_t face);
	void ClearCube(float r = 0, float g = 0, float b = 0, float a = 0);

	ID3D11ShaderResourceView* GetShaderResourceView(uint32_t face);
	ID3D11UnorderedAccessView* GetUnorderedAccessView(uint32_t face);
	ID3D11RenderTargetView* GetRenderTargetView(uint32_t face);
	ID3D11Texture2D* GetTexture(uint32_t face);

	void SetMipmappingShader(string& shader, string& func)
	{
		_DELETE(mipShader);
		mipShader = new Compute(shader, func);
	}

	void GenerateMipmaps();
	inline uint32_t GetMipsCount() { return mipsCount; }

	ALIGNED_ALLOCATION

private:
	D3D11_VIEWPORT viewport;

	ID3D11Texture2D* faces[6];

	ID3D11RenderTargetView* RTV[6];
	ID3D11ShaderResourceView* SRV[6];
	ID3D11UnorderedAccessView* UAV[6];

	int32_t height;
	int32_t width;
	uint32_t mipsCount;

	Compute* mipShader;
};
}