#pragma once

#include "Window.h"

#include "HEntity.h"

using namespace EngineCore;

#define BG_SHADER PATH_SHADERS "gui/color"
#define BORDER_SHADER PATH_SHADERS "gui/rect"

class Hud
{
public:
	Hud();

	inline static Hud* Get(){return instance;}

	bool Init();
	void Update(bool force_update_gui, bool no_gui_gc);
	void Draw(HWND win);
	void Close();

	void UpdateEntities(Window* win);

	void DeactivateWindow(Window* win)
	{
		KeyState.alt = false;
		KeyState.ctrl = false;
		KeyState.shift = false;
		MousePressed(MouseEventClick(MOUSE_LEFT, cursor_pos.x, cursor_pos.y), false, win);
		MousePressed(MouseEventClick(MOUSE_RIGHT, cursor_pos.x, cursor_pos.y), false, win);
		MousePressed(MouseEventClick(MOUSE_MIDDLE, cursor_pos.x, cursor_pos.y), false, win);
	}
	
	bool KeyPressed(const KeyEvent &arg, bool pressed, Window* win);
	bool MousePressed(const MouseEventClick &arg, bool pressed, Window* win);
	bool MouseWheel(const MouseEventWheel &arg, Window* win);
	bool MouseMove(const MouseEvent &arg, Window* win);

	HEntityWraper GetEntityById(string id);

	HEntityWraper GetMainRoot() const
	{
		HWND find = WindowsMgr::Get()->GetMainWindow()->GetHWND();
		auto it = rootEnts.find(find);
		if(it != rootEnts.end()) return HEntityWraper(it->second.rootEnt);
		else return HEntityWraper();
	}
	HEntityWraper GetRootByWindow(Window* win) const
	{
		HWND find = win->GetHWND();
		auto it = rootEnts.find(find);
		if(it != rootEnts.end()) return HEntityWraper(it->second.rootEnt);
		else return HEntityWraper();
	}

	bool SetRootClass(HEntityWraper e, LuaRef cls)
	{
		return GET_HENTITY(e.ID)->SetLuaClass(cls);
	}

	void UpdateRootRect(HEntityWraper root);

	bool CreateRoot(Window* wnd);
	bool DestroyRoot(Window* wnd);

	Window* GetWindowByRoot(HEntityWraper e);

	POINT GetSystemCursorPos(){return system_cursor_pos;}
	void SetSystemCursorPos(POINT pos){system_cursor_pos = pos;}

	POINT GetCursorPos(){return cursor_pos;}

	void AddDropItems(wchar_t* filename)
	{
		dropedItems.push_back(WstringToString(wstring(filename)));
	}
	void FinishDropItems(POINT dropPoint, Window* win);
	DArray<string>* GetDropedItems() {return &dropedItems;}

	uint32_t GetDropedItemsCount() {return uint32_t(dropedItems.size());}
	string GetDropedItem(uint32_t i)
	{
		if(i > dropedItems.size())
			return string();
		return dropedItems[i];
	}

	void RegLuaClass();
	
	struct key_state
	{
		bool shift;
		bool ctrl;
		bool alt;

		key_state()
		{
			shift = false;
			ctrl = false;
			alt = false;
		}
	} KeyState;

	HEntityStorage* EntStorge;

	HDC screenContext;

protected:
	static Hud* instance;

	struct root
	{
		uint32_t rootEnt;
		Window* win;

		Image2D* border;
	};
	unordered_map<HWND, root> rootEnts;
	SArray<Window*, GUI_ENTITY_COUNT> winForRoots;

	POINT system_cursor_pos;
	POINT cursor_pos;

	DArray<string> dropedItems;
};