#pragma once

#include "stdafx.h"
#include "HudWindow.h"
#include "Hud.h"

using namespace MyLittleFramework;

#define MENU_DASH_PAD 1
#define MENU_DASH_THICKNESS 1
#define MENU_INFO_PAD 5
#define MENU_INFO_WIDTH 50
#define MENU_INFO_MAXSYM 32

#define MENU_ITEM_MAXNUM 32
#define MENU_ITEM_INFO_NUM_OFFSET 1000
#define MENU_ITEM_CHECK_NUM_OFFSET 2000
#define MENU_SUBM_NUM_OFFSET 3000
#define MENU_SUBM_ARROW_NUM_OFFSET 4000

struct MenuProperties
{
	XMFLOAT4 body_color;
	bool shadow;
	XMFLOAT4 shadow_color;

	XMFLOAT4 border_color;
	int border_size;

	wchar_t* font;
	int item_height;
	int dash_height;

	XMFLOAT4 bg_color;
	XMFLOAT4 bg_color_hover;
	XMFLOAT4 color;
	XMFLOAT4 color_hover;
	XMFLOAT4 dash_color;
	XMFLOAT4 info_color;
	XMFLOAT4 bird_color;

	MenuProperties()
	{
		body_color = XMFLOAT4(0.15f,0.15f,0.15f,1.0f);
		shadow = false;
		shadow_color = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
		border_color = XMFLOAT4(0.0f,0.0f,0.0f,1.0f);
		border_size = 1;
		font = nullptr;
		item_height = 24;
		dash_height = 4;

		bg_color = XMFLOAT4(0.15f,0.15f,0.15f,1.0f);
		bg_color_hover = XMFLOAT4(0.07f, 0.07f, 0.07f, 1.0f);
		color = BUTTON_COLORSET_01_HOVER;
		color_hover = BUTTON_COLORSET_02;
		dash_color = XMFLOAT4(0.4f,0.4f,0.4f,1.0f);
		info_color = XMFLOAT4(0.6f,0.6f,0.6f,1.0f);
		bird_color = BUTTON_COLORSET_01_HOVER;
	}
};

class DropMenu: public HudWindow
{
	struct MenuItem
	{
		HudButton* button;
		HudLabel* info;
		HudRect* checkbox;
		int event_id;
		bool checked;

		MenuItem()
		{
			button = nullptr;
			info = nullptr;
			checkbox = nullptr;
			event_id = 0;
			checked = false;
		}
	};

	struct SubMenu
	{
		HudButton* button;
		HudRect* arrow;
		int menu_id;

		SubMenu()
		{
			button = nullptr;
			arrow = nullptr;
			menu_id = 0;
		}
	};

public:
	DropMenu();

	HudEvent LocalCallback(HudEvent e);
	bool Init(int ID, MenuProperties props, Hud* parent, HudWindow* menu_caller);

	int AddItem(bool checkable, wstring title, wstring addition, int EventID);
	int AddSubMenu(wstring title, int SubMenuID);
	bool AddDash();

	bool SetChecked(bool check, int ID);
	bool SetEnabled(int ID);
	bool SetDisabled(int ID);

	HudEvent MousePressed(const MouseEventClick &arg, bool pressed);
	HudEvent MouseMove(const MouseEvent &arg, bool check_collide);

	void SubMenusClose();

	void Enable()
	{
		HudWindow::Enable();
		m_parent->SetFocus(nullptr);
		m_parent->SetFocus(WIN_ID, true);
	}
	void Disable()
	{
		SubMenusClose();
		m_parent->SetFocus(nullptr);
		HudWindow::Disable();
	}

	HudWindow* m_menu_caller;

private:
	struct MenuAddProps
	{
		int item_height;
		int dash_height;
		XMFLOAT4 bg_color;
		XMFLOAT4 bg_color_hover;
		XMFLOAT4 color;
		XMFLOAT4 color_hover;
		XMFLOAT4 dash_color;
		XMFLOAT4 info_color;
		XMFLOAT4 bird_color;
		wchar_t* font;

		MenuAddProps()
		{
			item_height = 20;
			dash_height = 3;
			bg_color = XMFLOAT4(0.1f,0.1f,0.1f,1.0f);
			bg_color_hover = XMFLOAT4(0.2f,0.2f,0.2f,1.0f);
			color = XMFLOAT4(0.9f,0.9f,0.9f,1.0f);
			color_hover = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
			dash_color = XMFLOAT4(0.4f,0.4f,0.4f,1.0f);
			info_color = XMFLOAT4(0.6f,0.6f,0.6f,1.0f);
			bird_color = XMFLOAT4(0.0f,0.0f,0.0f,1.0f);
			font = nullptr;
		}

	} m_menu_props;

	Font* m_font;

	MenuItem* items[MENU_ITEM_MAXNUM];
	int items_num;
	SubMenu* submenus[MENU_ITEM_MAXNUM];
	int submenus_num;
	HudRect* dashes[MENU_ITEM_MAXNUM];
	int dashes_num;
};