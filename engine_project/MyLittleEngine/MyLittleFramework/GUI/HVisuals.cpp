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

RectData* HRectMgr::AddRect(int16_t winId)
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

	rectArr->data[ rectArr->ids[idx] ].id = idx;

	return &rectArr->data[ rectArr->ids[idx] ];
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