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

	inline RHI::GfxSRV* GetShaderResourceView() { return SRV; }
	inline RHI::GfxUAV* GetUnorderedAccessView(uint32_t face, uint32_t arrayID = 0) { return UAV[6 * arrayID + face]; }

	void GenerateMips();
	inline uint32_t GetMipsCount() { return mipsCount; }
	inline int32_t GetResolution() { return resolution; }

private:
	RHI::GfxTexture* faces;
	RHI::GfxUAV** UAV;
	RHI::GfxSRV* SRV;

	int32_t resolution;
	uint32_t mipsCount;
	uint32_t arraySize;
};
}