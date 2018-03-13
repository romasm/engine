#pragma once

#include "stdafx.h"
#include "Render.h"
#include "Common.h"

namespace EngineCore
{

class CubeRenderTarget
{
public:
	CubeRenderTarget();

	bool Init(int32_t resolution, DXGI_FORMAT fmt, bool hasMipChain = false);
	void Close();
	
	~CubeRenderTarget() { Close(); }

	void ClearCube(float r = 0, float g = 0, float b = 0, float a = 0);

	inline ID3D11ShaderResourceView* GetShaderResourceView() { return SRV; }
	inline ID3D11UnorderedAccessView* GetUnorderedAccessView(uint32_t face) { return UAV[face]; }
	
	inline void GenerateMips() { Render::GenerateMips(SRV); }
	inline uint32_t GetMipsCount() { return mipsCount; }
	inline int32_t GetResolution() { return resolution; }

private:
	ID3D11Texture2D* faces;
	ID3D11UnorderedAccessView* UAV[6];
	ID3D11ShaderResourceView* SRV;

	int32_t resolution;
	uint32_t mipsCount;
};
}