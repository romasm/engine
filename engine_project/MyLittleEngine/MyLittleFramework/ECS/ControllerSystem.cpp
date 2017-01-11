#include "stdafx.h"
#include "ControllerSystem.h"
#include "..\World.h"

using namespace EngineCore;

unordered_map<string, uint32_t> ControllerSystem::keyboardMap;
unordered_map<string, uint32_t> ControllerSystem::mouseMap;

ControllerSystem::ControllerSystem(BaseWorld* w)
{
	world = w;
	scriptSys = w->GetScriptSystem();

	components.reserve(4);
	keyMaps.reserve(4);

	if(keyboardMap.empty() || mouseMap.empty())
		FillControlMap();
};

void ControllerSystem::RawInput(RawInputData& data)
{
	if(components.size() == 0)
		return;

	switch (data.type)
	{
	case USER_DEVICES::KEYBOARD:
		if(data.pressed)
		{
			if(lastKeyboardState.isPressed[data.key])
				return;
			lastKeyboardState.isPressed[data.key] = true;
		}
		else
		{
			lastKeyboardState.isPressed[data.key] = false;
		}

		for(auto& i: components)
		{
			if( !world->IsEntityNeedProcess(i.second.get_entity()) )
				continue;

			if(!i.second.active)
				continue;

			auto func = i.second.funcMap->keyboardEvents[data.key];
			if(func)
				(*func)(i.second.classInstanceRef, data.key, data.pressed, 0, 0, 0);
		}
		break;

	case USER_DEVICES::MOUSE:
		if((data.key & RI_MOUSE_LEFT_BUTTON_DOWN) > 0)
			SendMouseEvent(MouseEvents::LEFT, true, 0);
		else if((data.key & RI_MOUSE_LEFT_BUTTON_UP) > 0)
			SendMouseEvent(MouseEvents::LEFT, false, 0);

		if((data.key & RI_MOUSE_RIGHT_BUTTON_DOWN) > 0)
			SendMouseEvent(MouseEvents::RIGHT, true, 0);
		else if((data.key & RI_MOUSE_RIGHT_BUTTON_UP) > 0)
			SendMouseEvent(MouseEvents::RIGHT, false, 0);

		if((data.key & RI_MOUSE_MIDDLE_BUTTON_DOWN) > 0)
			SendMouseEvent(MouseEvents::MIDDLE, true, 0);
		else if((data.key & RI_MOUSE_MIDDLE_BUTTON_UP) > 0)
			SendMouseEvent(MouseEvents::MIDDLE, false, 0);

		if((data.key & RI_MOUSE_WHEEL) > 0)
			SendMouseEvent(MouseEvents::WHEEL, false, data.deltaZ);

		if(data.deltaX != 0)
			SendMouseEvent(MouseEvents::MOVE_X, false, data.deltaX);
		if(data.deltaY != 0)
			SendMouseEvent(MouseEvents::MOVE_Y, false, data.deltaY);
		break;
	}
}
		
void ControllerSystem::SendMouseEvent(MouseEvents me, bool pressed, int32_t d)
{
	uint32_t key = eKeyCodes::KEY_MAX;
	switch(me)
	{
	case MouseEvents::LEFT:
		key = eKeyCodes::KEY_LBUTTON;
		break;
	case MouseEvents::RIGHT:
		key = eKeyCodes::KEY_RBUTTON;
		break;
	case MouseEvents::MIDDLE:
		key = eKeyCodes::KEY_MBUTTON;
		break;
	}

	for(auto& i: components)
	{
		if( !world->IsEntityNeedProcess(i.second.get_entity()) )
			continue;

		if(!i.second.active)
			continue;

		auto func = i.second.funcMap->mouseEvents[me];
		if(func)
			(*func)(i.second.classInstanceRef, key, pressed, d, d, d);
	}
}

void ControllerSystem::AddComponent(Entity e, string keyMapName)
{
	auto scriptComp = scriptSys->GetComponent(e);
	if(!scriptComp)
	{
		ERR("Can\'t add controller component %s, script component needed!", keyMapName.c_str());
		return;
	}

	Controller cntr;
	cntr.classInstanceRef = scriptComp->classInstanceRef;
	cntr.keyMapName = keyMapName;
	cntr.funcMap = new FuncMap;

	if( !AttachLuaFuncs(e, cntr, *scriptComp) )
	{
		_DELETE(cntr.funcMap);
		return;
	}

	components.insert(make_pair(e.index(), cntr));
}

#define GET_COMPONENT(res) auto& it = components.find(e.index());\
	if(it == components.end())	return res;\
	auto& comp = it->second;

bool ControllerSystem::IsActive(Entity e)
{
	GET_COMPONENT(false)
	return comp.active;
}

bool ControllerSystem::SetActive(Entity e, bool active)
{
	GET_COMPONENT(false)
	comp.active = active;
	return true;
}

#ifdef _DEV
void ControllerSystem::UpdateLuaFuncs()
{
	for(auto& comp: components)
	{
		if(!comp.second.funcMap)
			return;

		Entity e = comp.second.get_entity();

		auto scriptComp = scriptSys->GetComponent(e);
		if(!scriptComp)
		{
			ERR("Can\'t update controller component %s, script component needed!", comp.second.keyMapName.c_str());
			continue;
		}

		AttachLuaFuncs(e, comp.second, *scriptComp);
	}
}
#endif

bool ControllerSystem::AttachLuaFuncs(Entity e, Controller& comp, ScriptComponent& script)
{
	auto keyMap = GetKeyMap(comp.keyMapName);
	if(!keyMap)
	{
		ERR("Can\'t get key map %s!", comp.keyMapName.c_str());
		return false;
	}

	for(uint16_t i = 0; i < eKeyCodes::KEY_MAX; i++)
	{
		string& eventName = keyMap->keyboardEvents[i];
		if(eventName.empty())
			continue;

		string funcName("on");
		funcName += eventName;
		LuaRef func = scriptSys->GetLuaFunction(script, funcName);
		if(func.isFunction())
			comp.funcMap->keyboardEvents[i] = new LuaRef(func);
	}

	for(uint16_t i = 0; i < MouseEvents::COUNT; i++)
	{
		string& eventName = keyMap->mouseEvents[i];
		if(eventName.empty())
			continue;

		string funcName("on");
		funcName += eventName;
		LuaRef func = scriptSys->GetLuaFunction(script, funcName);
		if(func.isFunction())
			comp.funcMap->mouseEvents[i] = new LuaRef(func);
	}

	return true;
}

KeyMap* ControllerSystem::GetKeyMap(string& keyMapName)
{
	auto& it = keyMaps.find(keyMapName);
	if(it != keyMaps.end())
	{
		return it->second;
	}

	string path(PATH_KEYMAPS);
	path += keyMapName;
	path += ".cfg";

	FileIO file(path);
	auto root = file.Root();
	if(!root)
		return nullptr;

	KeyMap* res = new KeyMap;

	auto keyboard = file.Node(L"Keyboard", root);
	if(keyboard)
	{
		for(auto& i: *keyboard)
		{
			auto keyCode = keyboardMap.find(WstringToString(i.first));
			if(keyCode != keyboardMap.end())
				res->keyboardEvents[keyCode->second] = WstringToString(i.second.value);
		}
	}

	auto mouse = file.Node(L"Mouse", root);
	if(mouse)
	{
		for(auto& i: *mouse)
		{
			auto keyCode = mouseMap.find(WstringToString(i.first));
			if(keyCode != mouseMap.end())
				res->mouseEvents[keyCode->second] = WstringToString(i.second.value);
		}
	}

	keyMaps.insert(make_pair(keyMapName, res));
	return res;
}

// ------------------------
void ControllerSystem::FillControlMap()
{
	mouseMap["LEFT"] = MouseEvents::LEFT;
	mouseMap["RIGHT"] = MouseEvents::RIGHT;
	mouseMap["MIDDLE"] = MouseEvents::MIDDLE;
	mouseMap["MOVE_X"] = MouseEvents::MOVE_X;
	mouseMap["MOVE_Y"] = MouseEvents::MOVE_Y;
	mouseMap["WHEEL"] = MouseEvents::WHEEL;


	keyboardMap["KEY_LBUTTON"] = 0x00; // Left mouse button
	keyboardMap["KEY_RBUTTON"] = 0x02; // Right mouse button
	keyboardMap["KEY_CANCEL"] = 0x03; // Control-break processing
	keyboardMap["KEY_MBUTTON"] = 0x04; // Middle mouse button (three-button mouse)
	keyboardMap["KEY_XBUTTON1"] = 0x05; // X1 mouse button
	keyboardMap["KEY_XBUTTON2"] = 0x06; // X2 mouse button
	keyboardMap["KEY_BACK"] = 0x08; // BACKSPACE key
	keyboardMap["KEY_TAB"] = 0x09; // TAB key
	keyboardMap["KEY_CLEAR"] = 0x0C; // CLEAR key
	keyboardMap["KEY_RETURN"] = 0x0D; // ENTER key
	keyboardMap["KEY_SHIFT"] = 0x10; // SHIFT key
	keyboardMap["KEY_CONTROL"] = 0x11; // CTRL key
	keyboardMap["KEY_ALT"] = 0x12; // ALT key
	keyboardMap["KEY_PAUSE"] = 0x13; // PAUSE key
	keyboardMap["KEY_CAPITAL"] = 0x14; // CAPS LOCK key
	keyboardMap["KEY_KANA"] = 0x15; // IME Kana mode
	keyboardMap["KEY_HANGUEL"] = 0x15; // IME Hanguel mode
	keyboardMap["KEY_HANGUL"] = 0x15; // IME Hangul mode
	keyboardMap["KEY_JUNJA"] = 0x17; // IME Junja mode
	keyboardMap["KEY_FINAL"] = 0x18; // IME final mode
	keyboardMap["KEY_HANJA"] = 0x19; // IME Hanja mode
	keyboardMap["KEY_KANJI"] = 0x19; // IME Kanji mode

	keyboardMap["KEY_ESCAPE"] = 0x1B;
	keyboardMap["KEY_SPACE"] = 0x20;
	keyboardMap["KEY_PAGEUP"] = 0x21;
	keyboardMap["KEY_PAGEDOWN"] = 0x22;
	keyboardMap["KEY_END"] = 0x23;
	keyboardMap["KEY_HOME"] = 0x24;
	keyboardMap["KEY_LEFT"] = 0x25; // left arrow key
	keyboardMap["KEY_UP"] = 0x26; // up arrow key
	keyboardMap["KEY_RIGHT"] = 0x27; // right arrow key
	keyboardMap["KEY_DOWN"] = 0x28; // down arrow key
	keyboardMap["KEY_SELECT"] = 0x29;
	keyboardMap["KEY_EXE"] = 0x2B; // execute key
	keyboardMap["KEY_SNAPSHOT"] = 0x2C;
	keyboardMap["KEY_INSERT"] = 0x2D;
	keyboardMap["KEY_DELETE"] = 0x2E;
	keyboardMap["KEY_HELP"] = 0x2F;

	keyboardMap["KEY_0"] = 0x30;
	keyboardMap["KEY_1"] = 0x31;
	keyboardMap["KEY_2"] = 0x32;
	keyboardMap["KEY_3"] = 0x33;
	keyboardMap["KEY_4"] = 0x34;
	keyboardMap["KEY_5"] = 0x35;
	keyboardMap["KEY_6"] = 0x36;
	keyboardMap["KEY_7"] = 0x37;
	keyboardMap["KEY_8"] = 0x38;
	keyboardMap["KEY_9"] = 0x39;

	keyboardMap["KEY_A"] = 0x41;
	keyboardMap["KEY_B"] = 0x42;
	keyboardMap["KEY_C"] = 0x43;
	keyboardMap["KEY_D"] = 0x44;
	keyboardMap["KEY_E"] = 0x45;
	keyboardMap["KEY_F"] = 0x46;
	keyboardMap["KEY_G"] = 0x47;
	keyboardMap["KEY_H"] = 0x48;
	keyboardMap["KEY_I"] = 0x49;
	keyboardMap["KEY_J"] = 0x4A;
	keyboardMap["KEY_K"] = 0x4B;
	keyboardMap["KEY_L"] = 0x4C;
	keyboardMap["KEY_M"] = 0x4D;
	keyboardMap["KEY_N"] = 0x4E;
	keyboardMap["KEY_O"] = 0x4F;
	keyboardMap["KEY_P"] = 0x50;
	keyboardMap["KEY_Q"] = 0x51;
	keyboardMap["KEY_R"] = 0x52;
	keyboardMap["KEY_S"] = 0x53;
	keyboardMap["KEY_T"] = 0x54;
	keyboardMap["KEY_U"] = 0x55;
	keyboardMap["KEY_V"] = 0x56;
	keyboardMap["KEY_W"] = 0x57;
	keyboardMap["KEY_X"] = 0x58;
	keyboardMap["KEY_Y"] = 0x59;
	keyboardMap["KEY_Z"] = 0x5A;

	keyboardMap["KEY_WINLEFT"] = 0x5B;
	keyboardMap["KEY_WINRIGHT"] = 0x5C;
	keyboardMap["KEY_APPS"] = 0x5D;

	keyboardMap["KEY_NUMPAD0"] = 0x60;
	keyboardMap["KEY_NUMPAD1"] = 0x61;
	keyboardMap["KEY_NUMPAD2"] = 0x62;
	keyboardMap["KEY_NUMPAD3"] = 0x63;
	keyboardMap["KEY_NUMPAD4"] = 0x64;
	keyboardMap["KEY_NUMPAD5"] = 0x65;
	keyboardMap["KEY_NUMPAD6"] = 0x66;
	keyboardMap["KEY_NUMPAD7"] = 0x67;
	keyboardMap["KEY_NUMPAD8"] = 0x68;
	keyboardMap["KEY_NUMPAD9"] = 0x69;

	keyboardMap["KEY_MULTIPLY"] = 0x6A;
	keyboardMap["KEY_ADD"] = 0x6B;
	keyboardMap["KEY_SEPARATOR"] = 0x6C;
	keyboardMap["KEY_SUBTRACT"] = 0x6D;
	keyboardMap["KEY_DECIMAL"] = 0x6E;
	keyboardMap["KEY_DIVIDE"] = 0x6F;

	keyboardMap["KEY_F1"] = 0x70;
	keyboardMap["KEY_F2"] = 0x71;
	keyboardMap["KEY_F3"] = 0x72;
	keyboardMap["KEY_F4"] = 0x73;
	keyboardMap["KEY_F5"] = 0x74;
	keyboardMap["KEY_F6"] = 0x75;
	keyboardMap["KEY_F7"] = 0x76;
	keyboardMap["KEY_F8"] = 0x77;
	keyboardMap["KEY_F9"] = 0x78;
	keyboardMap["KEY_F10"] = 0x79;
	keyboardMap["KEY_F11"] = 0x7A;
	keyboardMap["KEY_F12"] = 0x7B;
	keyboardMap["KEY_F13"] = 0x7C;
	keyboardMap["KEY_F14"] = 0x7D;
	keyboardMap["KEY_F15"] = 0x7E;
	keyboardMap["KEY_F16"] = 0x7F;
	keyboardMap["KEY_F17"] = 0x80;
	keyboardMap["KEY_F18"] = 0x81;
	keyboardMap["KEY_F19"] = 0x82;
	keyboardMap["KEY_F20"] = 0x83;
	keyboardMap["KEY_F21"] = 0x84;
	keyboardMap["KEY_F22"] = 0x85;
	keyboardMap["KEY_F23"] = 0x86;
	keyboardMap["KEY_F24"] = 0x87;

	keyboardMap["KEY_NUMLOCK"] = 0x90;
	keyboardMap["KEY_SCROLL"] = 0x91;

	keyboardMap["KEY_LSHIFT"] = 0xA0;
	keyboardMap["KEY_RSHIFT"] = 0xA1;
	keyboardMap["KEY_LCONTROL"] = 0xA2;
	keyboardMap["KEY_RCONTROL"] = 0xA3;
	keyboardMap["KEY_LALT"] = 0xA4;
	keyboardMap["KEY_RALT"] = 0xA5;

	keyboardMap["KEY_PLUS"] = 0xBB; // '+'
	keyboardMap["KEY_COMMA"] = 0xBC; // ';'
	keyboardMap["KEY_MINUS"] = 0xBD; // '-'
	keyboardMap["KEY_PERIOD"] = 0xBE; // '.'

	keyboardMap["KEY_EXPONENT"] = 0xDC; // '^'

	keyboardMap["KEY_ATTN"] = 0xF6;
	keyboardMap["KEY_CRSEL"] = 0xF7;
	keyboardMap["KEY_EXSEL"] = 0xF8;
	keyboardMap["KEY_EREOF"] = 0xF9;
	keyboardMap["KEY_PLAY"] = 0xFA;
	keyboardMap["KEY_ZOOM"] = 0xFB;
	keyboardMap["KEY_NONAME"] = 0xFC;
	keyboardMap["KEY_PA1"] = 0xFD;
	keyboardMap["KEY_OEMCLEAR"] = 0xFE;
}