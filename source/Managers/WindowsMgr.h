#pragma once
#include "stdafx.h"
#include "Window.h"
#include "LuaVM.h"
#include "HDefines.h"
#include "Arrays.h"

namespace EngineCore
{
	
#define MAX_GUI_WINDOWS 16

	class WindowsMgr
	{
	public:
		WindowsMgr();
		~WindowsMgr();
		
		inline static WindowsMgr* Get(){return m_instance;}
		
		inline unordered_map<HWND, int16_t>* GetMap(){return &windows_map;}
		
		int16_t GetWindowID(HWND hwnd);
		Window* GetWindowByHwnd(HWND hwnd);
		inline Window* GetWindowByID(int16_t id) {return &app_windows[id];}
		
		inline Window* GetMainWindow() {return &app_windows[main_window];}

		Window* AddWindow();
		void DeleteWindow(HWND hwnd);

		bool IsJustCreated(HWND hwnd) { return just_created >= 0 && windows_map.find(hwnd) == windows_map.end(); }

		bool Tick();

		static WindowsMgr* GetWindowsMgr(){return WindowsMgr::Get();}

		HCURSOR GetCursors(HCursors cursorid);

	private:
		static WindowsMgr *m_instance;

		void loadCursors();
		void closeAll();

		unordered_map<HCursors, HCURSOR> cursors;
		
		unordered_map<HWND, int16_t> windows_map;
		SArray< Window, MAX_GUI_WINDOWS > app_windows;
		SDeque< int16_t, MAX_GUI_WINDOWS > free_ids;

		SArray< HWND, MAX_GUI_WINDOWS > windows_to_delete;

		int16_t main_window;
		int16_t just_created;
	};
}