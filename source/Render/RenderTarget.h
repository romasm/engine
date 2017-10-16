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
	bool Init(int width, int height, ID3D11DepthStencilView* depthStencilView, int msaa_count = 1, int msaa_quality = 0);
	bool Init(RenderTarget* importDepthStencil);

	bool AddRT(DXGI_FORMAT RT_format, unsigned short mips = 1, bool UAV = false, bool forceMipRes = false);

	bool AddBackBufferRT(ID3D11Texture2D* p_BB);

	void Close();

	void SetRenderTarget(uint32_t rt_start = 0, uint32_t rt_end = 0);
	void ClearRenderTargets(bool clearDS = true){ClearRenderTargets(0,0,0,0, clearDS);}
	void ClearRenderTargets(float, float, float, float, bool clearDS = true);

	// Получаем текстуру RT в виде shader resource view
	ID3D11ShaderResourceView* GetShaderResourceView(int id);
	ID3D11UnorderedAccessView* GetUnorderedAccessView(int id);
	ID3D11RenderTargetView* GetRenderTargetView(int id);
	ID3D11DepthStencilView* GetDepthStencilView();
	ID3D11Texture2D* GetTexture(int id);

	// Call after Init, before AddRT!!!
	void SetMipmappingMaterial(string& mat)
	{
		if(!mip_mat) mip_mat = new string;
		*mip_mat = mat;
	}
	inline void SetMipmappingMaterial(char* mat){SetMipmappingMaterial(string(mat));}

	void GenerateMipmaps(class ScenePipeline* scene = nullptr);
	int GetMipsCountInFullChain(){return mip_count;}

	ALIGNED_ALLOCATION

	struct miplevel
	{
		ID3D11RenderTargetView** mip_RTV;
		ID3D11ShaderResourceView** mip_SRV;		
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

	D3D11_VIEWPORT m_viewport;

	ID3D11Texture2D* m_RTTexture[MAX_RENDERTARGETS];
	ID3D11Texture2D* m_DSTexture;
	ID3D11RenderTargetView* m_RTV[MAX_RENDERTARGETS];
	ID3D11ShaderResourceView* m_SRV[MAX_RENDERTARGETS];
	ID3D11UnorderedAccessView* m_UAV[MAX_RENDERTARGETS];
	ID3D11DepthStencilView* m_DSV;

	miplevel mipRes[MAX_RENDERTARGETS];

	unsigned short b_mips[MAX_RENDERTARGETS];

	POINT* mip_res;

	int t_height;
	int t_width;

	bool b_importedDS;

	bool msaa;
	uint32_t m_msaa_count;
	uint32_t m_msaa_quality;

	uint32_t RT_count;

	string* mip_mat;
	int mip_count;
	ScreenPlane* plane;
};
}