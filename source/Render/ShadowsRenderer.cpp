#include "stdafx.h"

#include "ShadowsRenderer.h"
#include "RenderMgrs.h"
#include "Render.h"
#include "RHI\GfxDevice.h"
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

	_DELETE(shadowsBufferSRV);
	for(int i=0; i<SHADOWS_BUF_SIZE; i++)
		_DELETE(shadowsBufferDSV[i]);
	_DELETE(shadowsBuffer);
}

void ShadowsRenderer::ClearPerFrame()
{
	for(auto& it: shadowmap_array)
		castersIdx[it.id] = ENTITY_COUNT;		
	shadowmap_array.clear();
}

bool ShadowsRenderer::initShadowBuffer()
{
	using namespace RHI;

	TextureDesc texDesc = {};
	texDesc.dimension   = TextureDimension::Tex2DArray;
	texDesc.width       = SHADOWS_BUF_RES;
	texDesc.height      = SHADOWS_BUF_RES;
	texDesc.depth       = SHADOWS_BUF_SIZE;
	texDesc.mipLevels   = 1;
	texDesc.format      = DXGI_FORMAT_R32_TYPELESS;
	texDesc.allowDSV    = true;
	texDesc.allowSRV    = true;
	texDesc.msaaSamples = 1;

	shadowsBuffer = GFX_DEVICE->CreateTexture(texDesc);
	if(!shadowsBuffer)
		return false;

	shadowsBufferSRV = GFX_DEVICE->CreateSRV(shadowsBuffer, DXGI_FORMAT_R32_FLOAT);
	if(!shadowsBufferSRV)
		return false;

	for(uint8_t i = 0; i < SHADOWS_BUF_SIZE; i++)
	{
		shadowsBufferDSV[i] = GFX_DEVICE->CreateDSV(shadowsBuffer, DXGI_FORMAT_D32_FLOAT, 0, i);
		if(!shadowsBufferDSV[i])
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
	uint32_t fpos = (uint32_t)(uint64_t(first) - uint64_t(arr->begin())) / sizeof(ShadowMap);
	uint32_t spos = (uint32_t)(uint64_t(second) - uint64_t(arr->begin())) / sizeof(ShadowMap);

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
			shadows_sizes[k].res = max<uint16_t>(shadows_sizes[k].res / 2, SHADOWS_MINRES);
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
		GFX_CMD->ClearDepthStencil(shadowsBufferDSV[i], 1.0f, 0);

	PlaceShadowMaps();
}

bool ShadowsRenderer::RegShadowMap(uint32_t id,  float size)
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

void ShadowsRenderer::RenderShadow(uint32_t id, uint8_t num, ShadowRenderMgr* shadow_mgr, RHI::GfxBuffer* vp)
{
	Render::SetTopology(IA_TOPOLOGY::TRISLIST);

	ShadowMap shm = shadowmap_array[castersIdx[id]];
	for(uint32_t i=0; i<num; i++)
		shm = shadowmap_array[shm.next];

	GFX_CMD->SetRenderTargets(0, nullptr, shadowsBufferDSV[std::min<uint8_t>(shm.dsv, SHADOWS_BUF_SIZE-1)]);
	RHI::GfxViewport viewport;
	viewport.topLeftX = shm.x;
	viewport.topLeftY = shm.y;
	viewport.height = viewport.width = shm.res;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	GFX_CMD->SetViewport(viewport);

	GFX_CMD->SetVSConstantBuffers(0, 1, &vp);

	shadow_mgr->DrawOpaque();

	if(shadow_mgr->IsTranparentShadows())
	{
		// TODO: transparent render target
		shadow_mgr->DrawTransparent();
	}

	GFX_CMD->SetRenderTargets(0, nullptr, nullptr);
}