#include "stdafx.h"

#include "HVisuals.h"

using namespace EngineCore;

HVisuals* HVisuals::instance = nullptr;

HVisuals::HVisuals()
{
	if(instance)
	{
		ERR("Only one instance of HRectMgr is allowed!");
		return;
	}
	instance = this;

	rectsPerWindow.reserve(1);
}

HVisuals::~HVisuals()
{

}

uint16_t HVisuals::AddRect(uint16_t hwnd)
{
	SArray<RectData, MAX_RECTS_PER_WINDOW>* rectArr;

	auto win_id = winIDs.find(hwnd);
	if( win_id == winIDs.end() )
	{
		winIDs.insert(make_pair( hwnd, (uint16_t)rectsPerWindow.size() ));
		rectArr = &rectsPerWindow.push_back();
	}
	else
	{
		rectArr = &rectsPerWindow[win_id->second];
	}

	if(rectArr->full())
	{
		ERR("Rect array overflow!");
		return;
	}
	
	uint16_t rect_id = rectArr->size();
	auto rect = rectArr->push_back();
	
	return rect_id;
}

void HVisuals::DeleteRect(uint16_t winId, uint16_t id)
{

}