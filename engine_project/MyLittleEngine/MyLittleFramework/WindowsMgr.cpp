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
		main_window = nullptr;
		just_created = nullptr;
		windows_to_delete.reserve(10);
	}
	else
		ERR("Only one instance of WindowsMgr is allowed!");
}

void WindowsMgr::Close()
{
	for(auto& it : app_windows)
		_CLOSE(it.second);
	app_windows.erase(app_windows.begin(), app_windows.end());

	m_instance = nullptr;
}

bool WindowsMgr::Init()
{
	LoadCursors();
	return true;
}

bool WindowsMgr::Frame()
{
	MSG msg;
	while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(just_created)
		just_created->AfterRunEvent();
	
	windows_to_delete.clear();

	for(auto& it : app_windows)
	{
		it.second->AfterRunEvent();
		
		// если окно неактивно - завершаем кадр
		//if (!it.second->IsActive())
		//	return true;

		if (it.second->IsExit())
		{
			if(it.second->IsMain())
				return false;

			_CLOSE(it.second);
			windows_to_delete.push_back(it.first);
		}
	}

	for(auto& hwnd: windows_to_delete)
		app_windows.erase(hwnd);

	return true;
}

Window* WindowsMgr::GetWindowByHwnd(HWND hwnd)
{
	auto it = app_windows.find(hwnd);
	if(it == app_windows.end())
	{
		if(just_created)
			return just_created;
		return nullptr;
	}
	return it->second;
}

void WindowsMgr::LoadCursors()
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