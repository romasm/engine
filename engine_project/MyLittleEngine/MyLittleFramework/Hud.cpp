#include "stdafx.h"
#include "Render.h"
#include "Hud.h"
#include "Font.h"
#include "Text.h"
#include "Common.h"

using namespace EngineCore;

Hud *Hud::instance = nullptr;

static void UpdateLuaFuncs()
{
	Hud::Get()->EntStorge->UpdateLuaFuncs();
}

inline static bool GetShift(){return Hud::Get()->KeyState.shift;}
inline static bool GetCtrl(){return Hud::Get()->KeyState.ctrl;}
inline static bool GetAlt(){return Hud::Get()->KeyState.alt;}

static XMFLOAT4* GetColor(string id){return GCOLOR(id);}

static void PutInClipboard(string str)
{
	if(!OpenClipboard( WindowsMgr::Get()->GetMainWindow()->GetHWND() ))
		return;
	EmptyClipboard();
	
	HGLOBAL hgBuffer = GlobalAlloc(GMEM_MOVEABLE, (str.length() + 1) * sizeof(wchar_t));
	void* strBuffer = GlobalLock(hgBuffer);

	wstring wstr = StringToWstring(str);
	memcpy(strBuffer, wstr.c_str(), wstr.length() * sizeof(wchar_t)); 
	((wchar_t*)strBuffer)[wstr.length()] = wchar_t(0);

	GlobalUnlock(hgBuffer);
	SetClipboardData(CF_UNICODETEXT, hgBuffer);
	CloseClipboard();
}

static string TakeFromClipboard()
{
	string res;

	if(!OpenClipboard( WindowsMgr::Get()->GetMainWindow()->GetHWND() ))
		return res;
	
	HGLOBAL hgBuffer = GetClipboardData(CF_UNICODETEXT);
	wstring wstr = (wchar_t*)GlobalLock(hgBuffer);

	res = WstringToString(wstr);
		
	GlobalUnlock(hgBuffer);
	CloseClipboard();
	return res;
}

static POINT GetCursorPosLua() {return Hud::Get()->GetCursorPos();}
static POINT GetSystemCursorPosLua() {return Hud::Get()->GetSystemCursorPos();}
static HEntityWraper GetMainRootLua() {return Hud::Get()->GetMainRoot();}
static HEntityWraper GetRootByWindowLua(Window* win) {return Hud::Get()->GetRootByWindow(win);}
static HEntityWraper GetEntityByIdLua(string id) {return Hud::Get()->GetEntityById(id);}
static void UpdateEntitiesLua(Window* win) {Hud::Get()->UpdateEntities(win);}
static uint32_t GetDropedItemsCountLua() {return Hud::Get()->GetDropedItemsCount();}
static string GetDropedItemLua(uint32_t i) {return Hud::Get()->GetDropedItem(i);}

static Window* GetMainSysWindow() {return WindowsMgr::Get()->GetMainWindow();}
static Window* GetSysWindowByHwnd(luaHWND h) {return WindowsMgr::Get()->GetWindowByHwnd(h.hwnd);}
static Window* GetSysWindowByRoot(HEntityWraper e) {return Hud::Get()->GetWindowByRoot(e);}
static Window* GetSysWindowByEntity(HEntityWraper e) {return Hud::Get()->GetWindowByRoot(e.GetRoot());}
static bool SetRootClassLua(HEntityWraper e, LuaRef cls) {return Hud::Get()->SetRootClass(e, cls);}

static void UpdateRootRectLua(HEntityWraper e) 
{
	Hud::Get()->UpdateRootRect(HEntityWraper(GET_HENTITY(e.ID)->GetRoot()));
}

static void SetHCursor(uint16_t crs) {SetCursor(WindowsMgr::Get()->GetCursors((HCursors)crs));}

#define RCP255 1.0f / 255.0f
static XMFLOAT4 GetColorUnderCursor() {
	if(!Hud::Get()->screenContext)
		return XMFLOAT4(0,0,0,0);

	POINT pos = Hud::Get()->GetSystemCursorPos();
	HDC hdc = GetDC(NULL);
	COLORREF color = GetPixel(Hud::Get()->screenContext, pos.x, pos.y);
    int r = GetRValue(color);
    int g = GetGValue(color);
    int b = GetBValue(color);

	return XMFLOAT4(float(r) * RCP255, float(g) * RCP255, float(b) * RCP255, 1.0f);
}
static void StartColorRead() {
	Hud::Get()->screenContext = GetDC(NULL);
}
static void EndColorRead() {
	ReleaseDC(NULL, Hud::Get()->screenContext);
}

static Window* CreateSysWindow()
{
	bool ismain = true;
	if(GetMainSysWindow())
		ismain = false;

	Window* wnd = new Window();
	
	DescWindow desc;

	RECT captionR;
	captionR.left = 0;
	captionR.top = 0;
	captionR.right = 0;
	captionR.bottom = 25;
	desc.captionRect = captionR;

	if ( !wnd->Create(desc, ismain) )
	{
		ERR("Unable to create window");
		return nullptr;
	}

	Hud::Get()->CreateRoot(wnd);
	wnd->UpdateWindowState();
	return wnd;
}

static void _SetCursorPos(HEntityWraper e, int x, int y)
{
	auto root = e.GetRoot();
	auto win = GetSysWindowByRoot(root);
	SetCursorPos(x + win->GetLeft(), y + win->GetTop());
}

static void _ShowCursor(bool b) {ShowCursor(b);}

void Hud::RegLuaClass()
{	
	getGlobalNamespace(LSTATE)
		.beginNamespace("CoreGui")
			.addFunction("GetCursorPos", &GetCursorPosLua)
			.addFunction("GetSystemCursorPos", &GetSystemCursorPosLua)

			.beginNamespace("Screen")
				.addFunction("StartColorRead", &StartColorRead)
				.addFunction("ReadColorUnderCursor", &GetColorUnderCursor)
				.addFunction("EndColorRead", &EndColorRead)
			.endNamespace()

			.addFunction("GetMainRoot", &GetMainRootLua)
			.addFunction("GetRootByWindow", &GetRootByWindowLua)
			.addFunction("UpdateLuaFuncs", &UpdateLuaFuncs)
			.addFunction("SetRootClass", &SetRootClassLua)
			.addFunction("UpdateRootRect", &UpdateRootRectLua)

			.addFunction("GetEntityById", &GetEntityByIdLua)
			.addFunction("UpdateEntities", &UpdateEntitiesLua)

			.beginNamespace("DropedItems")
				.addFunction("GetCount", &GetDropedItemsCountLua)
				.addFunction("GetItem", &GetDropedItemLua)
			.endNamespace()

			.addFunction("SetHCursor", &SetHCursor)

			.addFunction("SetCursorPos", &_SetCursorPos)
			.addFunction("ShowCursor", &_ShowCursor)

			.addFunction("GetColor", &GetColor) // move in lua??

			.beginNamespace("SysWindows")
				.addFunction("GetMain", &GetMainSysWindow)
				.addFunction("GetByHwnd", &GetSysWindowByHwnd)
				.addFunction("GetByRoot", &GetSysWindowByRoot)
				.addFunction("GetByEntity", &GetSysWindowByEntity)
				.addFunction("Create", &CreateSysWindow)
			.endNamespace()

			.beginNamespace("Keys")
				.addFunction("Shift", &GetShift)
				.addFunction("Ctrl", &GetCtrl)
				.addFunction("Alt", &GetAlt)
			.endNamespace()

			.beginNamespace("Clipboard")
				.addFunction("PutString", &PutInClipboard)
				.addFunction("TakeString", &TakeFromClipboard)
			.endNamespace()
		.endNamespace();

	Window::RegLuaClass();
	
	HEntityWraper::RegLuaClass();
	HEvent::RegLuaClass();
}

Hud::Hud()
{
	if(!instance)
	{
		instance = this;
		
		system_cursor_pos.x = 0;
		system_cursor_pos.y = 0;

		cursor_pos.x = 0;
		cursor_pos.y = 0;

		dropedItems.reserve(10);

		winForRoots.resize(GUI_ENTITY_COUNT);
		winForRoots.assign(nullptr);
	}
	else
	{
		ERR("Only one instance of Hud is allowed!");
	}
}

bool Hud::Init()
{
	RegLuaClass();
	EntStorge = new HEntityStorage;
	return true;
}

bool Hud::CreateRoot(Window* wnd)
{
	HWND hwnd = wnd->GetHWND();

	if(rootEnts.find(hwnd) != rootEnts.end())
	{
		ERR("Window %i already has root", (int)hwnd);
		return false;
	}

	Hud::root r;
	r.rootEnt = EntStorge->CreateEntity();
	r.win = wnd;
	
	winForRoots[r.rootEnt] = wnd;

	auto ent = GET_HENTITY(r.rootEnt);
	ent->align = 3;
	ent->valign = 3;
	ent->Init("root", LuaRef(LSTATE));

	ent->AddRect(string(BG_SHADER));

	r.border = new Image2D();
	r.border->Init(string(BORDER_SHADER));
	r.border->SetRect(MLRECT());
	auto borderShader = r.border->GetShaderInst();
	borderShader->SetVector(XMFLOAT4(-1000000.0f,-10000000.0f,1000000.0f,1000000.0f), 0);
	borderShader->SetVector(XMFLOAT4(0,0,0,0), 2);

	rootEnts.insert(make_pair(hwnd, r));
	return true;
}

bool Hud::DestroyRoot(Window* wnd)
{
	HWND hwnd = wnd->GetHWND();

	auto it = rootEnts.find(hwnd);
	if(it == rootEnts.end())
	{
		ERR("Window %i has no root", (int)hwnd);
		return false;
	}

	_CLOSE(it->second.border);
	GET_HENTITY(it->second.rootEnt)->SendKill();
	EntStorge->DestroyEntity(it->second.rootEnt);
	rootEnts.erase(it);

	return true;
}

Window* Hud::GetWindowByRoot(HEntityWraper e)
{
	return winForRoots[e.ID];
}

void Hud::UpdateEntities(Window* win)
{
	auto it = rootEnts.find(win->GetHWND());
	if(it != rootEnts.end())
	{
		Render::Get()->CurrentHudWindow = it->second.win;

		auto ent = GET_HENTITY(it->second.rootEnt);
		ent->UpdatePosSize(win);
	}
}

void Hud::UpdateRootRect(HEntityWraper root)
{
	auto win = GetWindowByRoot(root);
	auto it = rootEnts.find(win->GetHWND());
	if(it == rootEnts.end())
		return;
	
	auto ent = GET_HENTITY(it->second.rootEnt);
	MLRECT rect = ent->GetRectAbsolute();

	it->second.border->SetRect(rect);
	XMFLOAT4 shader_data = XMFLOAT4(1.0f / rect.width, 1.0f / rect.height, 0, 0);
	shader_data.z = 1.0f - shader_data.x;
	shader_data.w = 1.0f - shader_data.y;

	auto borderShader = it->second.border->GetShaderInst();
	borderShader->SetVector(shader_data, 1);
	borderShader->SetVector(
		it->second.win->IsActive() ? *(it->second.win->GetColorBorderFocus()) : *(it->second.win->GetColorBorder()), 3);

	ent->SetRect(0, MLRECT(0, 0, rect.width, rect.height));
	auto bgShader = ent->GetRectShaderInst(0);
	bgShader->SetVector(*(it->second.win->GetColorBg()), 1);
}

HEntityWraper Hud::GetEntityById(string id)
{
	for(auto& it : rootEnts)
	{
		auto e = GET_HENTITY(it.second.rootEnt)->GetChildById(id);
		if(e == GUI_ENTITY_COUNT)
			continue;
		return HEntityWraper(e);
	}
	return HEntityWraper();
}

void Hud::Frame(bool force_update_gui, bool no_gui_gc)
{
	if(!no_gui_gc)
		EntStorge->DefferedDestroyProcess();

	const float dt = Timer::Get()->dt_ms;

	// update
	for(auto& it : rootEnts)
	{
		auto win = it.second.win;
		
		if(force_update_gui)
			UpdateEntities(win);
		else if(win->m_finResize)
		{
			UpdateEntities(win);
			win->m_finResize = false;
		}
		
		GET_HENTITY(it.second.rootEnt)->Update(dt);
	}

	// draw
	for(auto& it : rootEnts)
	{
		auto win = it.second.win;
		win->SetRenderTarget();
		
		GET_HENTITY(it.second.rootEnt)->RegToDraw();
		
		it.second.border->GetShaderInst()->SetVector(
			it.second.win->IsActive() ? *(it.second.win->GetColorBorderFocus()) : *(it.second.win->GetColorBorder()), 3);
		it.second.border->Draw();
	}
}

void Hud::Close()
{	
	for(auto& it : rootEnts)
	{
		_CLOSE(it.second.border);
		EntStorge->DestroyEntity(it.second.rootEnt);
	}
	rootEnts.clear();

	_DELETE(EntStorge);
	
	instance = nullptr;
}

bool Hud::KeyPressed(const KeyEvent &arg, bool pressed, Window* win)
{
	switch (arg.code)
	{
	case KEY_SHIFT:
		KeyState.shift = pressed;
		break;
	case KEY_CONTROL:
		KeyState.ctrl = pressed;
		break;
	case KEY_ALT:
		KeyState.alt = pressed;
		break;
	}
	
	auto it = rootEnts.find(win->GetHWND());
	if(it == rootEnts.end())
		return false;

	GET_HENTITY(it->second.rootEnt)->KeyPressed(arg, pressed);

	return true;
}

bool Hud::MousePressed(const MouseEventClick &arg, bool pressed, Window* win)
{
	auto it = rootEnts.find(win->GetHWND());
	if(it == rootEnts.end())
		return false;

	GET_HENTITY(it->second.rootEnt)->MousePressed(arg, pressed);

	return true;
}

bool Hud::MouseWheel(const MouseEventWheel &arg, Window* win)
{
	auto it = rootEnts.find(win->GetHWND());
	if(it == rootEnts.end())
		return false;

	GET_HENTITY(it->second.rootEnt)->MouseWheel(arg);

	return true;
}

bool Hud::MouseMove(const MouseEvent &arg, Window* win)
{
	cursor_pos.x = arg.x;
	cursor_pos.y = arg.y;

	auto it = rootEnts.find(win->GetHWND());
	if(it == rootEnts.end())
		return false;

	GET_HENTITY(it->second.rootEnt)->MouseMove(arg, true);

	return true;
}

void Hud::FinishDropItems(POINT dropPoint, Window* win)
{
	auto it = rootEnts.find(win->GetHWND());
	if(it == rootEnts.end())
		return;

	GET_HENTITY(it->second.rootEnt)->DropEvent(GuiEvents::GE_ITEMS_DROPED, dropPoint);

	dropedItems.resize(0);
}