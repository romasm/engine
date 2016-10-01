#include "stdafx.h"
#include "Common.h"
#include "DropMenu.h"
#include "Hud.h"

using namespace MyLittleFramework;

DropMenu::DropMenu(): HudWindow()
{
	m_font = nullptr;

	for(int i=0; i<MENU_ITEM_MAXNUM; i++)
	{
		items[i] = nullptr;
		submenus[i] = nullptr;
		dashes[i] = nullptr;
	}
	items_num = 0;
	submenus_num = 0;
	dashes_num = 0;
	m_menu_caller = nullptr;
}

bool DropMenu::Init(int ID, MenuProperties props, Hud* parent, HudWindow* menu_caller)
{
	m_menu_caller = menu_caller;

	if(!props.font)
		return false;

	m_menu_props.item_height = props.item_height;
	m_menu_props.dash_height = props.dash_height;
	m_menu_props.bg_color = props.bg_color;
	m_menu_props.bg_color_hover = props.bg_color_hover;
	m_menu_props.color = props.color;
	m_menu_props.color_hover = props.color_hover;
	m_menu_props.dash_color = props.dash_color;
	m_menu_props.info_color = props.info_color;
	m_menu_props.bird_color = props.bird_color;
	m_menu_props.font = props.font;

	WinProperties win_props;
	win_props.header = false;
	win_props.border = true;
	win_props.shadow = props.shadow;
	win_props.shadow_color = props.shadow_color;
	win_props.shadow_color_active = props.shadow_color;
	win_props.resizeableX = false;
	win_props.resizeableY = false;
	win_props.scrollable_x = false;
	win_props.scrollable_y = false;
	win_props.dragable = false;
	win_props.Xclose = false;
	win_props.client_pad = props.border_size;
	win_props.body_color = props.body_color;
	win_props.border_color = props.border_color;
	win_props.border_size = props.border_size;

	if(!HudWindow::Init(ID, win_props, parent))
		return false;

	SetSize(0, 0);

	return true;
}

int DropMenu::AddItem(bool checkable, wstring title, wstring addition, int EventID)
{
	int res = -1;

	MenuItem* item = new MenuItem();

	ButtonProperties button_props;
	button_props.border = false;
	button_props.material = EMPTY_BUTTON_MATERIAL;
	button_props.color = m_menu_props.color;
	button_props.color_hover = m_menu_props.color_hover;
	button_props.color_press = m_menu_props.color;
	button_props.bg_color = m_menu_props.bg_color;
	button_props.bg_color_hover = m_menu_props.bg_color_hover;
	button_props.bg_color_press = m_menu_props.bg_color;
	button_props.font = m_menu_props.font;
	button_props.text = title;
	button_props.text_offset.x = m_menu_props.item_height;
	button_props.text_offset.y = 0;

	item->button = new HudButton();
	if(!item->button->Init(items_num, this, button_props))
		return res;

	item->button->SetSize(m_rect.width, m_menu_props.item_height);
	item->button->SetPos(0, m_rect.height);
	if(!AddObject(item->button))
		return res;

	LabelProperties info_props;
	info_props.font = FONT_18PX_TEXT;
	info_props.background = false;
	info_props.border = false;
	info_props.static_text = false;
	info_props.text_color = m_menu_props.info_color;
	info_props.text_maxsize = MENU_INFO_MAXSYM;
	info_props.text = addition;
	info_props.text_offset.y = 2;

	item->info = new HudLabel();
	if(!item->info->Init(items_num+MENU_ITEM_INFO_NUM_OFFSET, this, info_props))
		return res;

	item->info->SetSize(MENU_INFO_WIDTH, m_menu_props.item_height);
	item->info->SetPos(MENU_INFO_PAD + MENU_INFO_WIDTH, m_rect.height, false, false, HUD_ALIGN_RIGHT);
	item->info->SetIgnoreEvents();
	if(!AddObject(item->info))
		return res;

	if(checkable)
	{
		RectProperties check_props;
		check_props.border_color = NULL_COLOR;
		check_props.border = true;
		check_props.bg_color = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		check_props.border_size = 0;
		check_props.material = MENU_BIRD_MATERIAL;

		item->checkbox = new HudRect();
		if(!item->checkbox->Init(items_num+MENU_ITEM_CHECK_NUM_OFFSET, this, check_props))
			return res;

		item->checkbox->SetSize(m_menu_props.item_height, m_menu_props.item_height);
		item->checkbox->SetPos(0, m_rect.height);
		item->checkbox->SetIgnoreEvents();
		if(!AddObject(item->checkbox))
			return res;
	}

	item->event_id = EventID;

	items[items_num] = item;
	res = items_num;
	items_num++;

	SetSize(m_rect.width, m_rect.height + m_menu_props.item_height);

	return res;
}

int DropMenu::AddSubMenu(wstring title, int SubMenuID)
{
	int res = -1;

	SubMenu* item = new SubMenu();

	ButtonProperties button_props;
	button_props.border = false;
	button_props.material = EMPTY_BUTTON_MATERIAL;
	button_props.color = m_menu_props.color;
	button_props.color_hover = m_menu_props.color_hover;
	button_props.color_press = m_menu_props.color_hover;
	button_props.bg_color = m_menu_props.bg_color;
	button_props.bg_color_hover = m_menu_props.bg_color_hover;
	button_props.bg_color_press = m_menu_props.bg_color_hover;
	button_props.font = m_menu_props.font;
	button_props.text = title;
	button_props.text_offset.x = m_menu_props.item_height;
	button_props.text_offset.y = 0;

	item->button = new HudButton();
	if(!item->button->Init(submenus_num+MENU_SUBM_NUM_OFFSET, this, button_props))
		return res;

	item->button->SetSize(m_rect.width, m_menu_props.item_height);
	item->button->SetPos(0, m_rect.height);
	if(!AddObject(item->button))
		return res;

	RectProperties arrow_props;
	arrow_props.border_color = m_menu_props.color;
	arrow_props.border = true;
	arrow_props.bg_color = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	arrow_props.border_size = 0;
	arrow_props.material = MENU_ARROW_MATERIAL;

	item->arrow = new HudRect();
	if(!item->arrow->Init(submenus_num+MENU_SUBM_ARROW_NUM_OFFSET, this, arrow_props))
		return res;

	item->arrow->SetSize(m_menu_props.item_height, m_menu_props.item_height);
	item->arrow->SetPos(m_menu_props.item_height, m_rect.height, false, false, HUD_ALIGN_RIGHT);
	item->arrow->SetIgnoreEvents();
	if(!AddObject(item->arrow))
		return res;

	item->menu_id = SubMenuID;

	submenus[submenus_num] = item;
	res = submenus_num;
	submenus_num++;

	SetSize(m_rect.width, m_rect.height + m_menu_props.item_height);

	return res;
}

bool DropMenu::AddDash()
{
	RectProperties	dash_props;
	dash_props.bg_color = m_menu_props.dash_color;
	dash_props.border = false;
	dash_props.border_size = 0;
	dash_props.material = Path::Get()->WString(HUD_LABEL_BG_MAT);

	HudRect* dash = new HudRect();
	dash->Init(100, this, dash_props);

	dash->SetPos(MENU_DASH_PAD, m_rect.height + (m_menu_props.dash_height - MENU_DASH_THICKNESS) / 2);
	dash->SetSize(m_rect.width - 2 * MENU_DASH_PAD, MENU_DASH_THICKNESS);

	if(!AddObject(dash))
		return false;

	dashes[dashes_num] = dash;
	dashes_num++;

	SetSize(m_rect.width, m_rect.height + m_menu_props.dash_height);

	return true;
}

bool DropMenu::SetChecked(bool check, int ID)
{
	if(ID>=items_num || ID<0)
		return false;
	if(!items[ID]->checkbox)
		return false;

	items[ID]->checked = check;

	if(check)
		items[ID]->checkbox->m_props.border_color = m_menu_props.bird_color;
	else
		items[ID]->checkbox->m_props.border_color = NULL_COLOR;
	items[ID]->checkbox->UpdateMatParams();

	return true;
}

bool DropMenu::SetEnabled(int ID)		// to do
{
	return true;
}

bool DropMenu::SetDisabled(int ID)		// to do
{
	return true;
}

HudEvent DropMenu::MousePressed(const MouseEventClick &arg, bool pressed)
{
	if(m_menu_caller)
		if(m_menu_caller->IsCollide(arg.x, arg.y))
		{
			HudEvent res = m_menu_caller->MousePressed(arg, pressed);
			//m_parent->SetFocus(WIN_ID, true);
			return res;
		}

	return HudWindow::MousePressed(arg, pressed);
}

HudEvent DropMenu::MouseMove(const MouseEvent &arg, bool check_collide)
{
	if(m_menu_caller)
		if(m_menu_caller->IsCollide(arg.x, arg.y))
		{
			HudEvent res = m_menu_caller->MouseMove(arg, true);
			//m_parent->SetFocus(WIN_ID, true);
			return res;
		}

	return HudWindow::MouseMove(arg, check_collide);
}

HudEvent DropMenu::LocalCallback(HudEvent e)
{
	HudEvent res;
	res = e;

	res = HudWindow::LocalCallback(e);

	switch (res.event_id)
	{
	case HUD_EVENT_MOUSE_DOWN_OUTOF:
		{
			Disable();
			res.object_id = HUD_ID_WIN_NULL;
			res.event_id = HUD_EVENT_MOUSE_DOWN;
			res = m_menu_caller->LocalCallback(res);
			eMouseKeyCodes btn;
			switch (res.key)
			{
			case KEY_RBUTTON:
				btn = eMouseKeyCodes::MOUSE_RIGHT;
				break;
			case KEY_MBUTTON:
				btn = eMouseKeyCodes::MOUSE_MIDDLE;
				break;
			case KEY_LBUTTON: default:
				btn = eMouseKeyCodes::MOUSE_LEFT;
				break;
			}
			MouseEventClick arg(btn, res.coords.x, res.coords.y);
			m_parent->MousePressed(arg, true, m_syswin);
		}
		break;
	case HUD_EVENT_BUTTON_HOVER:
		SubMenusClose();
		m_parent->SetFocus(WIN_ID, true);

		if(res.object_id < UINT(items_num) && res.object_id > 0)
		{
			if(!items[res.object_id]->checkbox)
				break;

			if(items[res.object_id]->checked)
				break;

			items[res.object_id]->checkbox->m_props.border_color = NULL_COLOR;
			items[res.object_id]->checkbox->UpdateMatParams();
		}
		else if(res.object_id < UINT(submenus_num+MENU_SUBM_NUM_OFFSET) && res.object_id >= MENU_SUBM_NUM_OFFSET)
		{
			int current = res.object_id-MENU_SUBM_NUM_OFFSET;

			QueuePointer<HudWindow>* submenu = m_parent->GetWindowById(submenus[current]->menu_id);
			if(!submenu)
				break;
			if(!submenu->current)
				break;
			((DropMenu*)submenu->current)->Enable();
			submenu->current->SetPos(m_rect.width + m_rect.left, submenus[current]->button->GetRect().top + m_rect.top);

			m_parent->SetFocus(submenu->current->GetID(), true);
		
			submenus[current]->button->m_props.bg_color = m_menu_props.bg_color_hover;
			submenus[current]->button->m_props.color = m_menu_props.color_hover;
			submenus[current]->button->UpdateMatParams();

			submenus[current]->arrow->m_props.border_color = m_menu_props.color_hover;
			submenus[current]->arrow->UpdateMatParams();
		}
		break;

	case HUD_EVENT_BUTTON_OUT:
		if(res.object_id < UINT(items_num) && res.object_id > 0)
		{
			if(!items[res.object_id]->checkbox)
				break;

			if(items[res.object_id]->checked)
				break;

			items[res.object_id]->checkbox->m_props.border_color = NULL_COLOR;
			items[res.object_id]->checkbox->UpdateMatParams();
		}
		break;

	case HUD_EVENT_BUTTON_PRESSED:
		m_parent->SetFocus(WIN_ID, true);
		if(res.object_id >= UINT(items_num) || res.object_id == 0)
			break;
		
		if(items[res.object_id]->checkbox)
		{
			if(items[res.object_id]->checked)
			{
				res.event_id = HUD_EVENT_MENU_UNCHECK;
				items[res.object_id]->checkbox->m_props.border_color = NULL_COLOR;
				items[res.object_id]->checked = false;
			}
			else
			{
				res.event_id = HUD_EVENT_MENU_CHECK;
				items[res.object_id]->checkbox->m_props.border_color = m_menu_props.bird_color;
				items[res.object_id]->checked = true;
			}
			items[res.object_id]->checkbox->UpdateMatParams();
		}
		else
		{
			Disable();
			HudEvent e;
			e.object_id = WIN_ID;
			e.event_id = HUD_EVENT_MENU_CLOSE;
			m_menu_caller->LocalCallback(e);
			res.event_id = HUD_EVENT_MENU_CLICK;
		}

		res.object_id = items[res.object_id]->event_id;
		break;

	}

	return res;
}

void DropMenu::SubMenusClose()
{
	for(int i=0; i<submenus_num; i++)
	{
		int current = i;

		QueuePointer<HudWindow>* submenu = m_parent->GetWindowById(submenus[current]->menu_id);
		if(!submenu)
			break;
		if(!submenu->current)
			break;

		((DropMenu*)submenu->current)->Disable();
			
		submenus[current]->button->m_props.bg_color = m_menu_props.bg_color;
		submenus[current]->button->m_props.color = m_menu_props.color;
		submenus[current]->button->UpdateMatParams();
		HudEvent e;
		e.event_id = HUD_EVENT_MOUSE_MOVE;
		e.coords.x = -10000;
		e.coords.y = -10000;
		submenus[current]->button->LocalCallback(e);

		submenus[current]->arrow->m_props.border_color = m_menu_props.color;
		submenus[current]->arrow->UpdateMatParams();
	}
}