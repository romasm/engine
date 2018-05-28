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

	bool Init(int32_t resolution, DXGI_FORMAT fmt, bool hasMipChain = false, uint32_t arrayCount = 1);
	void Close();
	
	~CubeRenderTarget() { Close(); }

	void ClearCube(uint32_t arrayID = 0, float r = 0, float g = 0, float b = 0, float a = 0);
	void ClearCubeArray(float r = 0, float g = 0, float b = 0, float a = 0);

	inline ID3D11ShaderResourceView* GetShaderResourceView() { return SRV; }
	inline ID3D11UnorderedAccessView* GetUnorderedAccessView(uint32_t face, uint32_t arrayID = 0) { return UAV[6 * arrayID + face]; }
	
	inline void GenerateMips() { Render::GenerateMips(SRV); }
	inline uint32_t GetMipsCount() { return mipsCount; }
	inline int32_t GetResolution() { return resolution; }

private:
	ID3D11Texture2D* faces;
	ID3D11UnorderedAccessView** UAV;
	ID3D11ShaderResourceView* SRV;

	int32_t resolution;
	uint32_t mipsCount;
	uint32_t arraySize;
};
}