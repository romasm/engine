#pragma once

#include "stdafx.h"
#include "Render.h"
#include "Common.h"
#include "ScreenPlane.h"

#define MAX_RENDERTARGETS 8

namespace EngineCore
{

class RenderTarget
{
	friend class ScenePipeline;
	friend class GaussianBlur;
public:
	RenderTarget();

	bool Init(int width, int height, DXGI_FORMAT depthStencil = DXGI_FORMAT_UNKNOWN, int msaa_count = 1, int msaa_quality = 0);
	bool Init(int width, int height, RHI::GfxDSV* depthStencilView, int msaa_count = 1, int msaa_quality = 0);
	bool Init(RenderTarget* importDepthStencil);

	bool AddRT(DXGI_FORMAT RT_format, unsigned short mips = 1, bool UAV = false, bool forceMipRes = false);

	bool AddBackBufferRT(RHI::GfxTexture* p_BB);

	void Close();

	void SetRenderTarget(uint32_t rt_start = 0, uint32_t rt_end = 0);
	void ClearRenderTargets(bool clearDS = true){ClearRenderTargets(0,0,0,0, clearDS);}
	void ClearRenderTargets(float, float, float, float, bool clearDS = true);

	// DX12 resource state transitions (no-ops on DX11)
	void TransitionToRenderTarget();
	void TransitionToShaderResource();
	void TransitionToUnorderedAccess();
	void TransitionDepthToRead();

	// �������� �������� RT � ���� shader resource view
	RHI::GfxSRV* GetShaderResourceView(int id);
	RHI::GfxUAV* GetUnorderedAccessView(int id);
	RHI::GfxRTV* GetRenderTargetView(int id);
	RHI::GfxDSV* GetDepthStencilView();
	RHI::GfxTexture* GetTexture(int id);

	// Call after Init, before AddRT!!!
	void SetMipmappingMaterial(const string& mat)
	{
		if(!mip_mat) mip_mat = new string;
		*mip_mat = mat;
	}
	inline void SetMipmappingMaterial(const char* mat){SetMipmappingMaterial(string(mat));}

	void GenerateMipmaps(class ScenePipeline* scene = nullptr);
	int GetMipsCountInFullChain(){return mip_count;}

	ALIGNED_ALLOCATION

	struct miplevel
	{
		RHI::GfxRTV** mip_RTV;
		RHI::GfxSRV** mip_SRV;
		unsigned short mipCount;

		miplevel()
		{
			mip_RTV = nullptr;
			mip_SRV = nullptr;
			mipCount = 0;
		}
	};

private:
	void initVP();

	RHI::GfxViewport m_viewport;

	RHI::GfxTexture* m_RTTexture[MAX_RENDERTARGETS];
	RHI::GfxTexture* m_DSTexture;
	RHI::GfxRTV* m_RTV[MAX_RENDERTARGETS];
	RHI::GfxSRV* m_SRV[MAX_RENDERTARGETS];
	RHI::GfxUAV* m_UAV[MAX_RENDERTARGETS];
	RHI::GfxDSV* m_DSV;

	miplevel mipRes[MAX_RENDERTARGETS];

	unsigned short b_mips[MAX_RENDERTARGETS];

	POINT* mip_res;

	int t_height;
	int t_width;

	bool b_importedDS;

	RHI::ResourceState m_currentRTState[MAX_RENDERTARGETS];
	RHI::ResourceState m_currentDSState;

	bool msaa;
	uint32_t m_msaa_count;
	uint32_t m_msaa_quality;

	uint32_t RT_count;

	string* mip_mat;
	int mip_count;
	ScreenPlane* plane;
};
}