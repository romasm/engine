#include "stdafx.h"

#include "HVisuals.h"

using namespace EngineCore;

HRectMgr* HRectMgr::instance = nullptr;

HRectMgr::HRectMgr()
{
	if(instance)
	{
		ERR("Only one instance of HRectMgr is allowed!");
		return;
	}
	instance = this;

	rectsPerWindow.resize(MAX_GUI_WINDOWS);
	rectsPerWindow.assign(nullptr);
}

HRectMgr::~HRectMgr()
{
	for(auto& i: rectsPerWindow)
		_DELETE(i);
}

void HRectMgr::allocWindow(int16_t winId)
{
	if(rectsPerWindow[winId])
		return;

	rectsPerWindow[winId] = new RectsPerWin;
	rectsPerWindow[winId]->ids.resize(MAX_RECTS_PER_WINDOW);
	rectsPerWindow[winId]->ids.assign(MAX_RECTS_PER_WINDOW);
	for(uint16_t i = 0; i < MAX_RECTS_PER_WINDOW; i++)
		rectsPerWindow[winId]->free_ids.push_back(i);
}

void HRectMgr::updateGPUData(Window* win, int16_t winId, uint16_t arrId)
{
	auto& data = rectsPerWindow[winId]->data[arrId];
	auto& dataGPU = rectsPerWindow[winId]->dataGPU[arrId];

	dataGPU.rect.x = - 1.0f + 2.0f * float(data.rect.l) / win->GetWidth();
	dataGPU.rect.y = 1.0f - 2.0f * float(data.rect.t) / win->GetHeight();
	dataGPU.rect.z = 2.0f * float(data.rect.w) / win->GetWidth();
	dataGPU.rect.w = 2.0f * float(data.rect.h) / win->GetHeight();

	rectsPerWindow[winId]->dataGPU[arrId].depth.x = float(rectsPerWindow[winId]->data[arrId].depth) / DEPTH_LAYER_MAX;
}

bool HRectMgr::CompareRects(RectData& a, RectData& b)
{
	return a.rectHash < b.rectHash;
}

void HRectMgr::SwapRects(RectData* first, RectData* second, SArray<uint16_t, MAX_RECTS_PER_WINDOW>* idsArr, 
	SArray<RectInstData, MAX_RECTS_PER_WINDOW>* gpuArr)
{
	swap((*gpuArr)[first->arrId], (*gpuArr)[second->arrId]);
	swap(first->arrId, second->arrId);
	swap((*idsArr)[first->id], (*idsArr)[second->id]);
	swap(*first, *second);
}

void HRectMgr::sortRects()
{
	for(int16_t winId = 0; winId < MAX_GUI_WINDOWS; winId++)
	{
		QSortSwap(rectsPerWindow[winId]->data.begin(), rectsPerWindow[winId]->data.end(), HRectMgr::CompareRects, 
			HRectMgr::SwapRects, &rectsPerWindow[winId]->ids, &rectsPerWindow[winId]->dataGPU);
	}
}

RectData* HRectMgr::AddRect(int16_t winId, SimpleShaderInst* shInst)
{
	if( winId < 0 )
		return nullptr;

	if(!rectsPerWindow[winId])
		allocWindow(winId);

	auto rectArr = rectsPerWindow[winId];
	
	if(rectArr->data.full())
	{
		ERR("Rect array overflow!");
		return nullptr;
	}
	
	uint16_t idx = rectArr->free_ids.front();
	rectArr->free_ids.pop_front();
	
	rectArr->ids[idx] = rectArr->data.size();

	rectArr->data.push_back();
	rectArr->dataGPU.push_back();

	rectArr->data[ rectArr->ids[idx] ].id = idx;
	rectArr->data[ rectArr->ids[idx] ].arrId = rectArr->ids[idx];
	rectArr->data[ rectArr->ids[idx] ].shaderInst = shInst;

	rectArr->data[ rectArr->ids[idx] ].rectHash = /*todo*/;
	
	return &rectArr->data[ rectArr->ids[idx] ];
}

bool HRectMgr::SetRect(int16_t winId, uint16_t id, MLRECT& rect)
{
	if( winId < 0 )
		return false;
	auto rectArr = rectsPerWindow[winId];
	if(!rectArr)
		return false;

	auto arrId = rectArr->ids[id];
	if(arrId == MAX_RECTS_PER_WINDOW)
		return false;

	rectArr->data[arrId].rect = rect;
	
	auto win = WindowsMgr::Get()->GetWindowByID(winId);
	updateGPUData(win, winId, arrId);
	return true;
}

void HRectMgr::DeleteRects(int16_t winId, uint16_t id)
{
	if( winId < 0 )
		return;
	auto rectArr = rectsPerWindow[winId];
	if(!rectArr)
		return;

	auto arrId = rectArr->ids[id];
	if(arrId == MAX_RECTS_PER_WINDOW)
		return;

	rectArr->data[arrId] = RectData();
	rectArr->dataGPU[arrId] = RectInstData();

	rectArr->ids[id] = MAX_RECTS_PER_WINDOW;
	rectArr->free_ids.push_back(id);

	if(rectArr->data.empty())
		_DELETE(rectsPerWindow[winId]);
}

RectData* HRectMgr::GetRect(int16_t winId, uint16_t id)
{
	if( winId < 0 )
		return nullptr;
	auto rectArr = rectsPerWindow[winId];
	if(!rectArr)
		return nullptr;

	auto arrId = rectArr->ids[id];
	if(arrId == MAX_RECTS_PER_WINDOW)
		return nullptr;

	return &rectArr->data[arrId];
}