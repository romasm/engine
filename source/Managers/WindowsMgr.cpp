#include "stdafx.h"
#include "WindowsMgr.h"
#include "Common.h"
#include "Render.h"

using namespace EngineCore;

WindowsMgr *WindowsMgr::m_instance = nullptr;

WindowsMgr::WindowsMgr()
{
	if(!m_instance)
	{
		m_instance = this;
		main_window = -1;
		just_created = -1;
		loadCursors();

		windows_map.reserve(MAX_GUI_WINDOWS);

		for(uint16_t i = 0; i < MAX_GUI_WINDOWS; i++)
			free_ids.push_back(i);
	}
	else
		ERR("Only one instance of WindowsMgr is allowed!");
}

WindowsMgr::~WindowsMgr()
{
	closeAll();
	m_instance = nullptr;
}

void WindowsMgr::closeAll()
{
	for(auto& it : windows_map)
	{
		app_windows[it.second].Close();
		app_windows[it.second] = Window();
		free_ids.push_back(it.second);
	}
	windows_map.clear();

	if(just_created >= 0)
	{
		app_windows[just_created].Close();
		app_windows[just_created] = Window();
		free_ids.push_back(just_created);
		just_created = -1;
	}

	main_window = -1;
}

bool WindowsMgr::Tick()
{
	MSG msg;
	while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if( just_created > -1 )
		app_windows[just_created].AfterRunEvent();
	
	windows_to_delete.clear();

	for(auto& it: windows_map)
	{
		app_windows[it.second].AfterRunEvent();

		if( app_windows[it.second].IsExit() )
		{
			if( app_windows[it.second].IsMain() )
			{
				closeAll();
				return false;
			}

			windows_to_delete.push_back(it.first);
		}
	}

	for(auto& hwnd: windows_to_delete)
		DeleteWindow(hwnd);

	return true;
}

int16_t WindowsMgr::GetWindowID(HWND hwnd)
{
	auto it = windows_map.find(hwnd);
	if(it == windows_map.end())
	{
		if( just_created >= 0 )
			return just_created;
		return -1;
	}
	return it->second;
}

Window* WindowsMgr::GetWindowByHwnd(HWND hwnd)
{
	auto id = GetWindowID(hwnd);
	if( id < 0 )
		return nullptr;
	return GetWindowByID(id);
}

void WindowsMgr::loadCursors()
{
	cursors[CURSOR_ARROW] = LoadCursor(0, IDC_ARROW);
	cursors[CURSOR_HAND] = LoadCursor(0, IDC_HAND);
	cursors[CURSOR_HELP] = LoadCursor(0, IDC_HELP);
	cursors[CURSOR_NO] = LoadCursor(0, IDC_NO);
	cursors[CURSOR_CROSSARROW] = LoadCursor(0, IDC_SIZEALL);
	cursors[CURSOR_DIAG1ARROW] = LoadCursor(0, IDC_SIZENESW);
	cursors[CURSOR_DIAG2ARROW] = LoadCursor(0, IDC_SIZENWSE);
	cursors[CURSOR_VERTARROW] = LoadCursor(0, IDC_SIZENS);
	cursors[CURSOR_HORZARROW] = LoadCursor(0, IDC_SIZEWE);
	cursors[CURSOR_STICK] = LoadCursor(0, IDC_IBEAM);
	cursors[CURSOR_CROSS] = LoadCursor(0, IDC_CROSS);
}

HCURSOR WindowsMgr::GetCursors(HCursors cursorid)
{
	auto it = cursors.find(cursorid);
	if(it == cursors.end())
		return cursors[CURSOR_ARROW];
	return it->second;
}

Window* WindowsMgr::AddWindow()
{
	if(free_ids.empty())
		return nullptr;

	int16_t idx = free_ids.front();
	free_ids.pop_front();
	
	app_windows[idx] = Window();
	just_created = idx;
	
	if( !app_windows[idx].Create(idx, main_window < 0) )
	{
		ERR("Unable to create window");
		return nullptr;
	}

	if( main_window < 0 )
		main_window = idx;

	auto hwnd = app_windows[idx].GetHWND();
	windows_map.insert(make_pair(hwnd, idx));

	just_created = -1;

	if ( !app_windows[idx].CreateSwapChain() )
	{
		ERR("Unable to create swap chain for window");
		DeleteWindow(hwnd);
		return nullptr;
	}

	return &app_windows[idx];
}

void WindowsMgr::DeleteWindow(HWND hwnd)
{
	int16_t winId = -1;
	auto it = windows_map.find(hwnd);
	if(it == windows_map.end())
	{
		winId = just_created;
		just_created = -1;
	}
	else
	{
		winId = it->second;
		windows_map.erase(it);
	}

	if(winId < 0)
		return;

	app_windows[winId].Close();
	app_windows[winId] = Window();

	free_ids.push_back(winId);

	if( main_window == winId )
		main_window = -1;
}