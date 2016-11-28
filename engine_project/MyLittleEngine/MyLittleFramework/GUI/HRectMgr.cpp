#include "stdafx.h"

#include "HRectMgr.h"

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

	rectsPerWindow.reserve(1);
	winIDs.reserve(10);
}

HRectMgr::~HRectMgr()
{

}

uint16_t HRectMgr::AddRect(HWND hwnd)
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

	// add rect
}

void HRectMgr::DeleteRect(uint16_t id)
{

}