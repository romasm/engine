#pragma once
#include "stdafx.h"
#include "Window.h"
#include "LuaVM.h"
#include "Gui/HDefines.h"
#include "DataAlloc\Arrays.h"

namespace EngineCore
{

#define START_HWND HWND(-10000)

	class WindowsMgr
	{
	public:
		WindowsMgr();
		
		inline static WindowsMgr* Get(){return m_instance;}
		
		inline map<HWND, Window*>* GetMap(){return &app_windows;}
		Window* GetWindowByHwnd(HWND hwnd);
		inline Window* GetMainWindow(){return main_window;}

		void AddWindow(HWND hwnd, Window* win)
		{
			app_windows.insert(make_pair(hwnd, win));
			if(!main_window)
				main_window = win;
		}

		void RegJustCreatedWindow(Window* win) { just_created = win; }
		void ClearJustCreatedWindow() { just_created = nullptr; }
		bool IsJustCreated(HWND hwnd) { return just_created && app_windows.find(hwnd) == app_windows.end(); }

		void Close();
		bool Init();
		bool Frame();

		static WindowsMgr* GetWindowsMgr(){return WindowsMgr::Get();}

		HCURSOR GetCursors(HCursors cursorid);

	private:
		void LoadCursors();

		map<HCursors, HCURSOR> cursors;

		static WindowsMgr *m_instance;

		map<HWND, Window*> app_windows;
		DArray<HWND> windows_to_delete;

		Window* main_window;
		Window* just_created;
	};
}