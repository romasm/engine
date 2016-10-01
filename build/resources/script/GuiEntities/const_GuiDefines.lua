SYSTEM_BORDER_SIZE = 7

SYSTEM_CURSORS = {
	ARROW = 0,
	HAND = 1,
	HELP = 2,
	NO = 3,
	CROSSARROW = 4,
	DIAG1ARROW = 5,
	DIAG2ARROW = 6,
	VERTARROW = 7,
	HORZARROW = 8,
	STICK = 9,
	CROSS = 10,
	NONE = 99,
}

GUI_FOCUS_MODE = {
    NONE = 0,
    NORMAL = 1,
    ONTOP = 2,
}

GUI_ALIGN = {
    LEFT = 0,
    RIGHT = 1,
    CENTER = 2,
    BOTH = 3,
}

GUI_VALIGN = {
    TOP = 0,
    BOTTOM = 1,
    MIDDLE = 2,
    BOTH = 3,
}

GUI_TEXTFIELD = {
    TEXT = 0,
    INT = 1,
    FLOAT = 2,
}

GUI_TEXTFIELD_NUMBERLIMITS = {
    MIN = -1000000000,
    MAX = 1000000000,
}

GUI_SLIDER_ORIENT = {
    HORZ = 0,
    VERT = 1,
}

GUI_SLIDER_MINSTEP = 0.1

ALT_SETS = {
    X = 0,
    Y = 20,
    TIME_SHOW = 1000,
    TIME_STOP = 500,
    TIME_FADEIN = 300,
}

GUI_COLORPICKER = {
    MIN = 1000,
    MAX = 15000,
    DIFF = 14000,
    OFFSET = 5,
}

GUI_EVENTS = {
    NULL                  = 0x00,
    ERROR                 = 0x01,

    UPDATE                = 0x02,
    TICK                  = 0x03,
 
    KEY_DOWN              = 0x04,
    KEY_UP                = 0x05,
    MOUSE_DOWN            = 0x06,
    MOUSE_UP              = 0x07,
    MOUSE_MOVE            = 0x08,
    MOUSE_WHEEL           = 0x09,

    BUTTON_PRESSED        = 0x0A,
    BUTTON_UNPRESSED      = 0x0B,
    BUTTON_MOVE           = 0x0C,
    BUTTON_HOVER          = 0x0D,
    BUTTON_OUT            = 0x0E,

    DO_DENIED             = 0x0F,

    FOCUS                 = 0x10,
    UNFOCUS               = 0x11,

    SLIDER_START_DRAG     = 0x12,
    SLIDER_END_DRAG       = 0x13,
    SLIDER_DRAG           = 0x14,

    MOUSE_OUTOFWIN        = 0x15, -- REMOVE

    MOUSE_DBLCLICK        = 0x16,

    TF_ACTIVATE           = 0x17,
    TF_SELECTED           = 0x18,
    TF_CURSORMOVE         = 0x19,
    TF_DEACTIVATE         = 0x1A,
    TF_EDITING            = 0x1B,

    CB_CHECKED            = 0x1C,
    CB_UNCHECKED          = 0x1D,
    CB_MOVE               = 0x1E, -- UNUSED
    CB_HOVER              = 0x1F,
    CB_OUT                = 0x20,

    BUTTON_FORCED_PRESS   = 0x21,
    BUTTON_FORCED_UNPRESS = 0x22,

    MENU_CHECK            = 0x23,
    MENU_UNCHECK          = 0x24,
    MENU_CLICK            = 0x25,
    MENU_CLOSE            = 0x26,

    MOUSE_DOWN_OUTOF      = 0x27, -- REMOVE

    WIN_START_MOVE            = 0x28,
    WIN_STOP_MOVE             = 0x29,
    WIN_MOVING                = 0x2A,
    WIN_START_RESIZE          = 0x2B,
    WIN_STOP_RESIZE           = 0x2C,
    WIN_RESIZING              = 0x2D,
    WIN_CLOSE             = 0x2E,
    WIN_SCROLL            = 0x2F,

    WIN_MAIN_RESIZE       = 0x30,

    GROUP_OPEN            = 0x31,
    GROUP_CLOSE           = 0x32,
    GROUP_UPDATE          = 0x33,

    ITEMS_DROPED          = 0x34,
    ITEMS_DRAG_ENTER      = 0x35,
    ITEMS_DRAG_LEAVE      = 0x36,
    ITEMS_DRAG_MOVE       = 0x37,

    ------- new
    CBGROUP_CHECK         = 0x38,

    COMBO_HOVER           = 0x39,
    COMBO_OUT             = 0x3A,
    COMBO_OPEN            = 0x3B,
    COMBO_CLOSE           = 0x3C,
    COMBO_SELECT          = 0x3D,

    SYS_RESIZE          = 0x3E,
    SYS_MOVE            = 0x3F,

    COLOR_PICKING       = 0x40,
    COLOR_PICKED       = 0x41,

    FF_SET                = 0x42,
    FF_RELOAD             = 0x43,
    FF_DELETE             = 0x44,

    MOUSE_HOVER             = 0x45,
    MOUSE_OUT             = 0x46,

    TEXTURE_SET                = 0x47,
    TEXTURE_RELOAD             = 0x48,
    TEXTURE_DELETE             = 0x49,

    WIN_SCROLL_WHEEL            = 0x4A,

    MENU_SUB_OPEN            = 0x4B,

    KILL                  = 0x4C,

}

KEYBOARD_CODES = {
		KEY_LBUTTON     = 0x00, -- Left mouse button
		KEY_RBUTTON     = 0x02, -- Right mouse button
		KEY_CANCEL      = 0x03, -- Control-break processing
		KEY_MBUTTON     = 0x04, -- Middle mouse button (three-button mouse)
		KEY_XBUTTON1    = 0x05, -- X1 mouse button
		KEY_XBUTTON2    = 0x06, -- X2 mouse button
		KEY_BACK        = 0x08, -- BACKSPACE key
		KEY_TAB         = 0x09, -- TAB key
		KEY_CLEAR       = 0x0C, -- CLEAR key
		KEY_RETURN      = 0x0D, -- ENTER key
		KEY_SHIFT       = 0x10, -- SHIFT key
		KEY_CONTROL     = 0x11, -- CTRL key
		KEY_ALT	        = 0x12, -- ALT key
		KEY_PAUSE       = 0x13, -- PAUSE key
		KEY_CAPITAL     = 0x14, -- CAPS LOCK key
		KEY_KANA        = 0x15, -- IME Kana mode
		KEY_HANGUEL     = 0x15, -- IME Hanguel mode
		KEY_HANGUL      = 0x15, -- IME Hangul mode
		KEY_JUNJA       = 0x17, -- IME Junja mode
		KEY_FINAL       = 0x18, -- IME final mode
		KEY_HANJA       = 0x19, -- IME Hanja mode
		KEY_KANJI       = 0x19, -- IME Kanji mode

		KEY_ESCAPE      = 0x1B,
		KEY_SPACE       = 0x20,
		KEY_PAGEUP      = 0x21,
		KEY_PAGEDOWN    = 0x22,
		KEY_END         = 0x23,
		KEY_HOME        = 0x24,
		KEY_LEFT        = 0x25, -- left arrow key
		KEY_UP          = 0x26, -- up arrow key
		KEY_RIGHT       = 0x27, -- right arrow key
		KEY_DOWN        = 0x28, -- down arrow key
		KEY_SELECT      = 0x29,
		KEY_EXE         = 0x2B, -- execute key
		KEY_SNAPSHOT    = 0x2C,
		KEY_INSERT      = 0x2D,
		KEY_DELETE      = 0x2E,
		KEY_HELP        = 0x2F,

		KEY_0           = 0x30,
		KEY_1           = 0x31,
		KEY_2           = 0x32,
		KEY_3           = 0x33,
		KEY_4           = 0x34,
		KEY_5           = 0x35,
		KEY_6           = 0x36,
		KEY_7           = 0x37,
		KEY_8           = 0x38,
		KEY_9           = 0x39,

		KEY_A           = 0x41,
		KEY_B           = 0x42,
		KEY_C           = 0x43,
		KEY_D           = 0x44,
		KEY_E           = 0x45,
		KEY_F           = 0x46,
		KEY_G           = 0x47,
		KEY_H           = 0x48,
		KEY_I           = 0x49,
		KEY_J           = 0x4A,
		KEY_K           = 0x4B,
		KEY_L           = 0x4C,
		KEY_M           = 0x4D,
		KEY_N           = 0x4E,
		KEY_O           = 0x4F,
		KEY_P           = 0x50,
		KEY_Q           = 0x51,
		KEY_R           = 0x52,
		KEY_S           = 0x53,
		KEY_T           = 0x54,
		KEY_U           = 0x55,
		KEY_V           = 0x56,
		KEY_W           = 0x57,
		KEY_X           = 0x58,
		KEY_Y           = 0x59,
		KEY_Z           = 0x5A,

		KEY_WINLEFT     = 0x5B,
		KEY_WINRIGHT    = 0x5C,
		KEY_APPS        = 0x5D,

		KEY_NUMPAD0     = 0x60,
		KEY_NUMPAD1     = 0x61,
		KEY_NUMPAD2     = 0x62,
		KEY_NUMPAD3     = 0x63,
		KEY_NUMPAD4     = 0x64,
		KEY_NUMPAD5     = 0x65,
		KEY_NUMPAD6     = 0x66,
		KEY_NUMPAD7     = 0x67,
		KEY_NUMPAD8     = 0x68,
		KEY_NUMPAD9     = 0x69,

		KEY_MULTIPLY    = 0x6A,
		KEY_ADD         = 0x6B,
		KEY_SEPARATOR   = 0x6C,
		KEY_SUBTRACT    = 0x6D,
		KEY_DECIMAL     = 0x6E,
		KEY_DIVIDE      = 0x6F,

		KEY_F1          = 0x70,
		KEY_F2          = 0x71,
		KEY_F3          = 0x72,
		KEY_F4          = 0x73,
		KEY_F5          = 0x74,
		KEY_F6          = 0x75,
		KEY_F7          = 0x76,
		KEY_F8          = 0x77,
		KEY_F9          = 0x78,
		KEY_F10         = 0x79,
		KEY_F11         = 0x7A,
		KEY_F12         = 0x7B,
		KEY_F13         = 0x7C,
		KEY_F14         = 0x7D,
		KEY_F15         = 0x7E,
		KEY_F16         = 0x7F,
		KEY_F17         = 0x80,
		KEY_F18         = 0x81,
		KEY_F19         = 0x82,
		KEY_F20         = 0x83,
		KEY_F21         = 0x84,
		KEY_F22         = 0x85,
		KEY_F23         = 0x86,
		KEY_F24         = 0x87,

		KEY_NUMLOCK     = 0x90,
		KEY_SCROLL      = 0x91,

		KEY_LSHIFT      = 0xA0,
		KEY_RSHIFT      = 0xA1,
		KEY_LCONTROL    = 0xA2,
		KEY_RCONTROL    = 0xA3,
		KEY_LALT	    = 0xA4,
		KEY_RALT        = 0xA5,

		KEY_PLUS        = 0xBB, -- '+'
		KEY_COMMA       = 0xBC, -- ','
		KEY_MINUS       = 0xBD, -- '-'
		KEY_PERIOD      = 0xBE, -- '.'

		KEY_EXPONENT    = 0xDC, -- '^'

		KEY_ATTN        = 0xF6,
		KEY_CRSEL       = 0xF7,
		KEY_EXSEL       = 0xF8,
		KEY_EREOF       = 0xF9,
		KEY_PLAY        = 0xFA,
		KEY_ZOOM        = 0xFB,
		KEY_NONAME      = 0xFC,
		KEY_PA1         = 0xFD,
		KEY_OEMCLEAR    = 0xFE,
}