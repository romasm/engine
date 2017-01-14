#include "stdafx.h"

#include "ShadowsRenderer.h"
#include "RenderMgrs.h"
#include "Render.h"
#include "Utils\Profiler.h"

using namespace EngineCore;

ShadowsRenderer::ShadowsRenderer(SceneRenderMgr* rndm)
{
	render_mgr = rndm;

	shadowsBuffer = nullptr;
	shadowsBufferSRV = nullptr;
	for(uint8_t i = 0; i < SHADOWS_BUF_SIZE; i++)
		shadowsBufferDSV[i] = nullptr;

	castersIdx.resize(ENTITY_COUNT);
	castersIdx.assign(ENTITY_COUNT);

	cascadeShadowRes = SHADOWS_DIR_RES;

	if(!initShadowBuffer())
		ERR("Failed init shadows buffer");

	shadows_sizes[0].size = SHADOWS_LOD_0_SIZE;
	shadows_sizes[1].size = SHADOWS_LOD_1_SIZE;
	shadows_sizes[2].size = SHADOWS_LOD_2_SIZE;
	shadows_sizes[3].size = SHADOWS_LOD_3_SIZE;
	shadows_sizes[4].size = SHADOWS_LOD_4_SIZE;
	shadows_sizes[5].size = SHADOWS_LOD_5_SIZE;
	shadows_sizes[0].res = SHADOWS_MAXRES;
	shadows_sizes[1].res = SHADOWS_MAXRES / 2;
	shadows_sizes[2].res = SHADOWS_MAXRES / 4;
	shadows_sizes[3].res = SHADOWS_MAXRES / 8;
	shadows_sizes[4].res = SHADOWS_MAXRES / 16;
	shadows_sizes[5].res = SHADOWS_MAXRES / 32;
}

ShadowsRenderer::~ShadowsRenderer()
{
	render_mgr = nullptr;

	_RELEASE(shadowsBufferSRV);
	for(int i=0; i<SHADOWS_BUF_SIZE; i++)
		_RELEASE(shadowsBufferDSV[i]);
	_RELEASE(shadowsBuffer);
}

void ShadowsRenderer::ClearPerFrame()
{
	for(auto& it: shadowmap_array)
		castersIdx[it.id] = ENTITY_COUNT;		
	shadowmap_array.clear();
}

bool ShadowsRenderer::initShadowBuffer()
{
	D3D11_TEXTURE2D_DESC shadowBufferDesc;
	ZeroMemory(&shadowBufferDesc, sizeof(shadowBufferDesc));
	shadowBufferDesc.Width = SHADOWS_BUF_RES;
	shadowBufferDesc.Height = SHADOWS_BUF_RES;
	shadowBufferDesc.MipLevels = 1;
	shadowBufferDesc.ArraySize = SHADOWS_BUF_SIZE;
	shadowBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS; //DXGI_FORMAT_D32_FLOAT
	shadowBufferDesc.SampleDesc.Count = 1;
	shadowBufferDesc.SampleDesc.Quality = 0;
	shadowBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	shadowBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowBufferDesc.CPUAccessFlags = 0;
	shadowBufferDesc.MiscFlags = 0;
	if( FAILED(Render::CreateTexture2D(&shadowBufferDesc, NULL, &shadowsBuffer)) )
		return false;
	
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2DArray.MipLevels = -1;	
	shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;	
	shaderResourceViewDesc.Texture2DArray.ArraySize = SHADOWS_BUF_SIZE;	
	if( FAILED(Render::CreateShaderResourceView(shadowsBuffer, &shaderResourceViewDesc, &shadowsBufferSRV)) )
		return false;
	
	for(uint8_t i = 0; i < SHADOWS_BUF_SIZE; i++)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
		depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		depthStencilViewDesc.Texture2DArray.MipSlice = 0;
		depthStencilViewDesc.Texture2DArray.ArraySize = 1;
		depthStencilViewDesc.Texture2DArray.FirstArraySlice = i;
		if( FAILED(Render::CreateDepthStencilView(shadowsBuffer, &depthStencilViewDesc, &shadowsBufferDSV[i])) )
			return false;
	}

	return true;
}

bool ShadowsRenderer::CompareShadows(ShadowMap& first, ShadowMap& second)
{
	return first.size < second.size;
}

void ShadowsRenderer::SwapShadows(ShadowMap* first, ShadowMap* second, SArray<ShadowMap, SHADOWMAPS_COUNT>* arr)
{
	uint32_t fpos = (uint32_t(first) - uint32_t(arr->begin())) / sizeof(ShadowMap);
	uint32_t spos = (uint32_t(second) - uint32_t(arr->begin())) / sizeof(ShadowMap);

	bool second_prev = false;
	bool second_next = false;

	if(first->prev != ENTITY_COUNT)
	{
		if(first->prev == spos)
			second_prev = true;
		(*arr)[first->prev].next = spos;
	}
	
	if(first->next != ENTITY_COUNT)
	{
		if(first->next == spos)
			second_next = true;
		(*arr)[first->next].prev = spos;
	}
	
	if(second->prev != ENTITY_COUNT)
	{
		if(second_next)
			first->next = fpos;
		else
			(*arr)[second->prev].next = fpos;
	}

	if(second->next != ENTITY_COUNT)
	{
		if(second_prev)
			first->prev = fpos;
		else
			(*arr)[second->next].prev = fpos;
	}

	swap(*first, *second);
}

void ShadowsRenderer::PlaceShadowMaps()
{
	SArray<POINT, SHADOWS_STRINGS_NUM> strings;
	POINT z;
	z.x = 0;
	z.y = 0;
	strings.push_back(z);

	uint8_t control_size_id = 0;
	for(uint16_t i=0; i < shadowmap_array.size(); i++)
	{
		auto& e = shadowmap_array[i];
		while(e.size < shadows_sizes[control_size_id].size && control_size_id < SHADOWS_STRINGS_NUM-1)
		{
			strings.push_back(strings[control_size_id]);
			control_size_id++;
		}

		auto& p = strings[control_size_id];

		shadowmap_array[i].res = float(shadows_sizes[control_size_id].res);
		shadowmap_array[i].x = float(p.x);
		shadowmap_array[i].y = float(p.y % SHADOWS_BUF_RES);
		shadowmap_array[i].dsv = uint8_t(p.y / SHADOWS_BUF_RES);
		
		p.x += shadows_sizes[control_size_id].res;
		if(p.x >= SHADOWS_BUF_RES)
		{
			p.y += shadows_sizes[control_size_id].res;

			if(control_size_id > 0)
			{
				auto* curr = &strings[control_size_id];
				auto* prev = &strings[control_size_id - 1];
				p.x = prev->x;

				uint8_t cur_size_id = control_size_id;
				while(cur_size_id > 0 && curr->y - prev->y >= shadows_sizes[cur_size_id - 1].res)
				{
					curr->x = -1;
					curr->y = -1;
					cur_size_id--;
					prev->y += shadows_sizes[cur_size_id].res;
					curr = prev;
					if(cur_size_id <= 0)
						break;
					prev = &strings[cur_size_id - 1];
				}

				if(cur_size_id > 0)
					curr->x = prev->x;
				else
					curr->x = 0;

				for(uint8_t j = 1; j < SHADOWS_STRINGS_NUM; j++)
					if(strings[j].x < 0)
						strings[j] = strings[j-1];
			}
			else
				p.x = 0;
		}
	}

	if(strings[strings.size()-1].y < SHADOWS_BUF_RES * SHADOWS_BUF_SIZE / 4 && shadows_sizes[0].res < SHADOWS_MAXRES)
	{
		LOG("Shadows buffer seams free! Target shadow resolution upscales.");
		for(uint8_t k = 0; k < 6; k++)
			shadows_sizes[k].res = min(shadows_sizes[k].res * 2, SHADOWS_MAXRES / uint16_t(pow(2, k)));
		cascadeShadowRes = shadows_sizes[0].res;

		PlaceShadowMaps();
		return;
	}
	else if(strings[strings.size()-1].y >= SHADOWS_BUF_RES * SHADOWS_BUF_SIZE && strings[strings.size()-1].x > 0 && shadows_sizes[0].res > SHADOWS_MINRES)
	{
		WRN("Shadows buffer overflow! Target shadow resolution downscales.");
		for(uint8_t k = 0; k < 6; k++)
			shadows_sizes[k].res = max(shadows_sizes[k].res / 2, SHADOWS_MINRES);
		cascadeShadowRes = shadows_sizes[0].res;

		PlaceShadowMaps();
		return;
	}
}

void ShadowsRenderer::ResolveShadowMaps()
{
	QSortSwap(shadowmap_array.begin(), shadowmap_array.end(), ShadowsRenderer::CompareShadows, ShadowsRenderer::SwapShadows, &shadowmap_array);

	for(int i=0; i < shadowmap_array.size(); i++)
		if(shadowmap_array[i].prev == ENTITY_COUNT)
			castersIdx[shadowmap_array[i].id] = i;

	for(int i=0; i<SHADOWS_BUF_SIZE; i++)
		Render::ClearDepthStencilView(shadowsBufferDSV[i], D3D11_CLEAR_DEPTH, 1.0f, 0);

	PlaceShadowMaps();
}

bool ShadowsRenderer::RegShadowMap(uint id,  float size)
{
	uint32_t subId = (uint32_t)shadowmap_array.size();
	if(subId >= SHADOWMAPS_COUNT)
		return false;

	ShadowMap shm;
	shm.id = id;
	shm.size = size;
	shm.next = ENTITY_COUNT;
	if(castersIdx[id] == ENTITY_COUNT)
	{
		shm.prev = ENTITY_COUNT;
		castersIdx[id] = subId;
	}
	else
	{
		uint32_t prev = castersIdx[id];
		uint32_t* next = &shadowmap_array[prev].next;
		while (*next != ENTITY_COUNT)
		{
			prev = *next;
			next = &shadowmap_array[prev].next;
		}
		*next = subId;
		shm.prev = prev;
	}
	
	shadowmap_array.push_back(shm);
	
	return true;
}

void ShadowsRenderer::RenderShadow(uint id, uchar num, ShadowRenderMgr* shadow_mgr, ID3D11Buffer* vp)
{
	ShadowMap shm = shadowmap_array[castersIdx[id]];
	for(uint i=0; i<num; i++)
		shm = shadowmap_array[shm.next];

	Render::OMSetRenderTargets(0, nullptr, shadowsBufferDSV[min(shm.dsv, SHADOWS_BUF_SIZE-1)]);
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = shm.x;
	viewport.TopLeftY = shm.y;
	viewport.Height = viewport.Width = shm.res;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	Render::RSSetViewports(1, &viewport);

	Render::VSSetConstantBuffers(0, 1, &vp);

	shadow_mgr->DrawOpaque();
	shadow_mgr->DrawAlphatest();

	/*if(shadow_mgr->IsTranparentShadows())
	{
		// transparent render target
		shadow_mgr->DrawTransparent();
	}*/

	Render::OMSetRenderTargets(0, nullptr, nullptr);
}