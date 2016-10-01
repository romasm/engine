#pragma once
#include "stdafx.h"

#define DOUBLE_CLICK_INTERVAL 400.0f

namespace EngineCore
{
	enum HCursors
	{
		CURSOR_ARROW = 0,
		CURSOR_HAND = 1,
		CURSOR_HELP = 2,
		CURSOR_NO = 3,
		CURSOR_CROSSARROW = 4,
		CURSOR_DIAG1ARROW = 5,
		CURSOR_DIAG2ARROW = 6,
		CURSOR_VERTARROW = 7,
		CURSOR_HORZARROW = 8,
		CURSOR_STICK = 9,
		CURSOR_CROSS = 10
	};

	enum GuiEvents{
		GE_NULL                = 0x00,
		GE_ERROR               = 0x01,

		GE_UPDATE                = 0x02,
		GE_TICK                  = 0x03,
 
		GE_KEY_DOWN              = 0x04,
		GE_KEY_UP                = 0x05,
		GE_MOUSE_DOWN            = 0x06,
		GE_MOUSE_UP              = 0x07,
		GE_MOUSE_MOVE            = 0x08,
		GE_MOUSE_WHEEL           = 0x09,

		GE_DO_DENIED             = 0x0F,

		GE_FOCUS                 = 0x10,
		GE_UNFOCUS               = 0x11,

		GE_MOUSE_OUTOFWIN        = 0x15,

		GE_MOUSE_DBLCLICK        = 0x16,

		GE_MOUSE_DOWN_OUTOF      = 0x27,

		GE_ITEMS_DROPED          = 0x34,
		GE_ITEMS_DRAG_ENTER      = 0x35,
		GE_ITEMS_DRAG_LEAVE      = 0x36,
		GE_ITEMS_DRAG_MOVE       = 0x37,

		GE_MOUSE_HOVER            = 0x45,
		GE_MOUSE_OUT            = 0x46,

		GE_KILL		            = 0x4C,
		
	};
}