#pragma once

#include "LocalTimer.h"
#include "Image.h"
#include "Text.h"
#include "HDefines.h"

using namespace EngineCore;

#define GUI_MINIMUM_FREE_INDICES 128

#define HENTITY_FOCUSMODE_NONE 0
#define HENTITY_FOCUSMODE_NORMAL 1
#define HENTITY_FOCUSMODE_ONTOP 2

#define HENTITY_ALIGN_LEFT 0
#define HENTITY_ALIGN_RIGHT 1
#define HENTITY_ALIGN_CENTER 2
#define HENTITY_ALIGN_BOTH 3

#define HENTITY_VALIGN_TOP 0
#define HENTITY_VALIGN_BOTTOM 1
#define HENTITY_VALIGN_MIDDLE 2
#define HENTITY_VALIGN_BOTH 3

struct HEvent
{
	friend class HEntityWraper;

	uint32_t event_id;
	string object_id;
	uint32_t object_sysid;
		
	POINT coords;
	bool collide;
	wchar_t sym;
	uint32_t key;

	HEvent()
	{
		event_id = GuiEvents::GE_NULL;
		object_id = "";
		object_sysid = GUI_ENTITY_COUNT;
		coords.x = 0;
		coords.y = 0;
		collide = false;
		sym = L'\0';
		key = KEY_LBUTTON;
	}

	HEvent& operator=(const HEvent& right)
	{
		this->coords = right.coords;
		this->collide = right.collide;
		this->event_id = right.event_id;
		this->key = right.key;
		this->object_id = right.object_id;
		this->object_sysid = right.object_sysid;
		this->sym = right.sym;
		return *this;
	}

	inline void SetHEntity(HEntityWraper obj);
	inline HEntityWraper GetHEntity() const;

	inline string GetSymbol() const
	{
		wstring str;
		str.resize(1);
		str[0] = sym;
		return WstringToString(str);
	}

	static void RegLuaClass();
};

class HEntity
{
	friend class HEntityStorage;
public:
	// properties
	bool enable;
	bool visible;

	int16_t left;
	int16_t top;
	int16_t right;
	int16_t bottom;
	uint16_t width;
	uint16_t height;
	
	bool left_percent;
	bool top_percent;
	bool right_percent;
	bool bottom_percent;
	bool width_percent;
	bool height_percent;
	
	uint16_t align;
	uint16_t valign;
	
	uint16_t focus_mode;

	bool collide_through;

	// static params
	bool double_click;
	bool ignore_events;

	// methods
	HEntity();

	bool Init(string id, LuaRef classRef);

	void Update(float dt);
	void RegToDraw();

	// TEMP: move to gui render system
	void ForceUpdateShaderData();

	void Close();

	inline HEvent LocalCallback(HEvent e);
	void LocalCallbackHierarchy(HEvent e);
	void SendEvent(HEvent e);
	void SendEventOnFocus(HEvent e);
	inline void SendKill()
	{
		HEvent ev;
		ev.event_id = GuiEvents::GE_KILL;
		ev.object_sysid = sys_ID;
		ev.object_id = ID;
		SendEvent(ev);
	}
	
	HEvent KeyPressed(const KeyEvent &arg, bool pressed);
	HEvent MousePressed(const MouseEventClick &arg, bool pressed);
	HEvent MouseWheel(const MouseEventWheel &arg);
	HEvent MouseMove(const MouseEvent &arg, bool collide);
	HEvent DragDropEvent(uint32_t dropEvent, POINT pos);

	inline MLRECT GetRectRelative() const {return relativeRect;}
	inline MLRECT GetRectAbsolute() const {return RECTtoMLRECT(globalPos);}
	inline RECT GetCorners() const {return globalPos;}
	inline RECT GetClip() const {return clipRect;}

	bool IsCollide(int16_t x, int16_t y);

	uint32_t GetParent();
	uint32_t GetRoot();

	inline string GetID() const {return ID;}
	inline uint32_t get_id() const {return sys_ID;}

	inline LuaRef GetInherited() const { return lua_class;	}
	
	bool AttachChild(uint32_t e);
	bool DetachChild(uint32_t e);

	uint32_t GetChildById(string id);
	uint32_t GetChildByIdSilent(string id);

	void SetFocus(uint32_t e, bool lock, bool overlay);

	void SetHierarchyFocus(uint32_t e, bool lock, bool overlay);
	void SetHierarchyFocusOnMe(bool lock, bool overlay);

	//void SetOnTop(uint32_t e);

	uint32_t GetFocus();
	bool GetFocusLock();

	// rects
	uint32_t AddRect(string& shadername)
	{
		Image2D& rect = rects->push_back();
		rect.Init(shadername);
		return uint32_t(rects->size() - 1);
	}
	bool SetRect(uint32_t i, MLRECT r)
	{
		if(i >= rects->size())
			return false;
		MLRECT gr = r;
		gr.left += globalPos.left;
		gr.top += globalPos.top;
		(*rects)[i].SetRect(gr);
		return true;
	}
	SimpleShaderInst* GetRectShaderInst(uint32_t i)
	{
		if(i >= rects->size())
			return nullptr;
		return (*rects)[i].GetShaderInst();
	}

	// texts
	uint32_t AddText(string& font, string& text, string& shadername, bool static_text, uint16_t length)
	{
		Text& t = texts->push_back();
		t.Init(font, StringToWstring(text), shadername, static_text, length, true);
		return uint32_t(texts->size() - 1);
	}
	bool SetTextPos(uint32_t i, int16_t x, int16_t y)
	{
		if(i >= texts->size())
			return false;
		x += int16_t(globalPos.left);
		y += int16_t(globalPos.top);
		(*texts)[i].SetPos(x, y);
		return true;
	}
	bool SetText(uint32_t i, string& text)
	{
		if(i >= texts->size())
			return false;
		return (*texts)[i].SetText(StringToWstring(text));
	}
	bool SetTextColor(uint32_t i, Vector4 color)
	{
		if(i >= texts->size())
			return false;
		(*texts)[i].SetColor(color);
		return true;
	}
	MLRECT GetTextBounds(uint32_t i)
	{
		MLRECT res;
		if(i >= texts->size())
			return res;
		res.width = (int32_t)(*texts)[i].GetWidth();
		res.height = (int32_t)(*texts)[i].GetHeight();
		res.left = (int32_t)(*texts)[i].GetLeft();
		res.top = (int32_t)(*texts)[i].GetTop();
		return res;
	}
	Text* GetText(uint32_t i)
	{
		if(i >= texts->size())
			return nullptr;
		return &(*texts)[i];
	}
	SimpleShaderInst* GetTextShaderInst(uint32_t i)
	{
		if(i >= texts->size())
			return nullptr;
		return (*texts)[i].GetShaderInst();
	}
	
protected:
	struct HChild
	{
		uint32_t e;
		HChild* next;
		HChild* prev;
		HChild()
		{
			e = GUI_ENTITY_COUNT;
			next = nullptr;
			prev = nullptr;
		}
		HChild(uint32_t ch)
		{
			e = ch;
			next = nullptr;
			prev = nullptr;
		}
	};
	
	void CalcPos(Window* win = nullptr);
	void CalcSize(Window* win = nullptr);
	
	void UpdateChildrenPosSize();
	void UpdateChildrenPos();
	void UpdateClip();
	
	void ActivateLoop();
	void DeactivateLoop();
	
public:

	void UpdateProps();

	inline void Activate()
	{
		active = true;
		ActivateLoop();
	}
	inline void Deactivate()
	{
		active = false;
		DeactivateLoop();
	}
	inline bool IsActivated()
	{
		return active;
	}
	inline bool IsActivatedBranch()
	{
		return active_branch;
	}

	void UpdatePos()
	{
		CalcPos();
		UpdateClip();

		if(updatepossize_func)
			if((*updatepossize_func)(lua_class, true, false) == false)
				return;

		UpdateChildrenPos();
	}
	void UpdateSize()
	{
		CalcSize();
		UpdateClip();
		
		if(updatepossize_func)
			if((*updatepossize_func)(lua_class, false, true) == false)
				return;

		UpdateChildrenPosSize();
	}
	void UpdatePosSize(Window* win = nullptr)
	{
		CalcSize(win);
		CalcPos(win);
		UpdateClip();

		if(updatepossize_func)
			if((*updatepossize_func)(lua_class, true, true) == false)
				return;

		UpdateChildrenPosSize();
	}

	void RecalcPosSize()
	{
		CalcSize();
		CalcPos();
		UpdateClip();
	}

protected:
	inline void ZeroEntity();
	HChild* GetHChild(uint32_t e, HEntity** out = nullptr);
	
	MLRECT relativeRect;
	RECT globalPos;
	
	RECT clipRect;
	
	bool active;
	bool active_branch;

	bool hover;
	bool dragHover;
	
	float dclick_time;

	HChild* first_child;
	HChild* last_child;
	unordered_map<string, HChild*>* named_children;
	DArray<HChild*>* unnamed_children;

	DArray<Image2D>* rects;
	DArray<Text>* texts;
	
	HChild* focus;
	bool focus_lock;

	HChild* trueTop;

	uint32_t sys_ID;
	string ID;

	uint32_t parent;
		
	LuaRef lua_class;
	LuaRef* tick_func;
	LuaRef* activate_func;
	LuaRef* deactivate_func;
	LuaRef* updatepossize_func;
	LuaRef* callback_func;

	void SetLuaFuncs();

public:
	bool SetLuaClass(LuaRef& classref);
};

class HEntityStorage
{
	public:
		HEntityStorage();
		~HEntityStorage(){instance = nullptr;}

		inline static HEntityStorage* Get() {return instance;}

		uint32_t CreateEntity()
		{
			if(free_id.size() <= GUI_MINIMUM_FREE_INDICES)
				return GUI_ENTITY_COUNT;

			uint32_t idx = free_id.front();
			free_id.pop_front();

			HEntity* ent = entities.add(idx);
			if(!ent) 
				return GUI_ENTITY_COUNT;
			ent->Close();
			ent->sys_ID = idx;
			return idx;
		}

		inline bool HasEntity(uint32_t e) const {return entities.has(e);}
		inline HEntity* GetEntity(uint32_t e)
		{
			size_t idx = entities.getArrayIdx(e);
			if(idx == GUI_ENTITY_COUNT) return nullptr;
			return &entities.getDataByArrayIdx(idx);
		}

		// dont use! system calls!!!
		void DestroyEntity(uint32_t e)
		{
			HEntity* ent = nullptr;
			if(e == GUI_ENTITY_COUNT)
				return;
			if(!(ent = GetEntity(e)))
				return;
			ent->Close();
			HEntity temp = *ent;
			entities.remove(e);
			(*entities.data())[entities.data()->size()] = temp;
			free_id.push_back(e);
		}

		void DefferedDestroyProcess()
		{
			while( !deffered_destroy.empty() )
			{
				uint32_t e = deffered_destroy.front();
				deffered_destroy.pop_front();

				DestroyEntity(e);
			}

			deffered_destroy.clear();
		}

		inline void DefferedDestroy(uint32_t e)
		{
			HEntity* ent;
			if(!(ent = GetEntity(e)))
				return;
			ent->SendKill();

			deffered_destroy.push_back(e);
		}

		inline void UpdateLuaFuncs()
		{
			auto d = entities.data();
			for(auto& i: *d)
				i.SetLuaFuncs();
		}

		inline void UpdateShaderData()
		{
			auto d = entities.data();
			for(auto& i: *d)
				i.ForceUpdateShaderData();
		}

	private:
		SDeque<uint32_t, GUI_ENTITY_COUNT> free_id;
		ComponentRArray<HEntity> entities;
		
		SDeque<uint32_t, GUI_ENTITY_COUNT> deffered_destroy;

		static HEntityStorage* instance;
};

#define GET_HENTITY(_e) HEntityStorage::Get()->GetEntity(_e)

class HEntityWraper
{
public:
	HEntityWraper(){ID = GUI_ENTITY_COUNT;}
	HEntityWraper(uint32_t id){ID = id;}

#define ADD_LUA_PROPERTY_FUNC(type, name) inline type get_##name() const {return GET_HENTITY(ID)->name;} \
									inline void set_##name(type value){GET_HENTITY(ID)->name = value;}
#define ADD_LUA_PROPERTY_DEF(type, name) .addProperty(#name, &HEntityWraper::get_##name, &HEntityWraper::set_##name)

	inline void Create(){ID = HEntityStorage::Get()->CreateEntity();}
	//inline void Destroy(){HEntityStorage::Get()->DestroyEntity(ID);}
	inline void Destroy(){HEntityStorage::Get()->DefferedDestroy(ID);} 
	inline bool IsExist(){return HEntityStorage::Get()->HasEntity(ID);}

	inline bool Init(string id, LuaRef classRef){return GET_HENTITY(ID)->Init(id, classRef);}

	inline void Activate(){GET_HENTITY(ID)->Activate();}
	inline void Deactivate(){GET_HENTITY(ID)->Deactivate();}
	inline bool IsActivated(){return GET_HENTITY(ID)->IsActivated();}
	inline bool IsActivatedBranch(){return GET_HENTITY(ID)->IsActivatedBranch();}

	inline void UpdatePos(){GET_HENTITY(ID)->UpdatePos();}
	inline void UpdateSize(){GET_HENTITY(ID)->UpdateSize();}
	inline void UpdatePosSize(){GET_HENTITY(ID)->UpdatePosSize();}

	inline void RecalcPosSize(){GET_HENTITY(ID)->RecalcPosSize();}

	inline MLRECT GetRectRelative(){return GET_HENTITY(ID)->GetRectRelative();}
	inline MLRECT GetRectAbsolute(){return GET_HENTITY(ID)->GetRectAbsolute();}
	inline RECT GetCorners(){return GET_HENTITY(ID)->GetCorners();}
	inline RECT GetClip(){return GET_HENTITY(ID)->GetClip();}

	inline bool IsCollide(int16_t x, int16_t y){return GET_HENTITY(ID)->IsCollide(x, y);}

	inline HEntityWraper GetParent(){return HEntityWraper(GET_HENTITY(ID)->GetParent());}
	inline HEntityWraper GetRoot(){return HEntityWraper(GET_HENTITY(ID)->GetRoot());}

	inline string GetID(){return GET_HENTITY(ID)->GetID();}
	inline LuaRef GetInherited(){return GET_HENTITY(ID)->GetInherited();}

	inline bool AttachChild(HEntityWraper e){return GET_HENTITY(ID)->AttachChild(e.ID);}
	inline bool DetachChild(HEntityWraper e){return GET_HENTITY(ID)->DetachChild(e.ID);}

	inline HEntityWraper GetChildById(string id){return HEntityWraper(GET_HENTITY(ID)->GetChildById(id));}

	inline void SetFocus(HEntityWraper e, bool lock, bool overlay){GET_HENTITY(ID)->SetFocus(e.ID, lock, overlay);}
	inline void SetHierarchyFocus(HEntityWraper e, bool lock, bool overlay){GET_HENTITY(ID)->SetHierarchyFocus(e.ID, lock, overlay);}
	inline void SetHierarchyFocusOnMe(bool lock, bool overlay){GET_HENTITY(ID)->SetHierarchyFocusOnMe(lock, overlay);}

	//inline void SetOnTop(HEntityWraper e){GET_HENTITY(ID)->SetOnTop(e.ID);}

	inline void CallbackHierarchy(HEvent e){GET_HENTITY(ID)->LocalCallbackHierarchy(e);}
	inline void SendEvent(HEvent e){GET_HENTITY(ID)->SendEvent(e);}
	inline void SendEventOnFocus(HEvent e){GET_HENTITY(ID)->SendEventOnFocus(e);}
	inline void Callback(HEvent e){GET_HENTITY(ID)->LocalCallback(e);}

	inline HEntityWraper GetFocus(){return HEntityWraper(GET_HENTITY(ID)->GetFocus());}
	inline bool GetFocusLock(){return GET_HENTITY(ID)->GetFocusLock();}

	inline uint32_t AddRect(string mat){return GET_HENTITY(ID)->AddRect(mat);}
	inline bool SetRect(uint32_t i, MLRECT r){return GET_HENTITY(ID)->SetRect(i, r);}
	inline SimpleShaderInst* GetRectShaderInst(uint32_t i){return GET_HENTITY(ID)->GetRectShaderInst(i);}

	inline uint32_t AddText(string font, string text, string shader, bool static_text, uint16_t length)
	{return GET_HENTITY(ID)->AddText(font, text, shader, static_text, length);}
	inline bool SetText(uint32_t i, string text){return GET_HENTITY(ID)->SetText(i, text);}
	inline bool SetTextPos(uint32_t i, int16_t x, int16_t y){return GET_HENTITY(ID)->SetTextPos(i, x, y);}
	inline bool SetTextColor(uint32_t i, Vector4 color){return GET_HENTITY(ID)->SetTextColor(i, color);}
	inline MLRECT GetTextBounds(uint32_t i){return GET_HENTITY(ID)->GetTextBounds(i);}
	inline Text* GetText(uint32_t i){return GET_HENTITY(ID)->GetText(i);}
	inline SimpleShaderInst* GetTextShaderInst(uint32_t i){return GET_HENTITY(ID)->GetTextShaderInst(i);}
		
	ADD_LUA_PROPERTY_FUNC(bool, enable)
	ADD_LUA_PROPERTY_FUNC(bool, visible)
	ADD_LUA_PROPERTY_FUNC(int16_t, left)
	ADD_LUA_PROPERTY_FUNC(int16_t, top)
	ADD_LUA_PROPERTY_FUNC(int16_t, right)
	ADD_LUA_PROPERTY_FUNC(int16_t, bottom)
	ADD_LUA_PROPERTY_FUNC(uint16_t, width)
	ADD_LUA_PROPERTY_FUNC(uint16_t, height)
	ADD_LUA_PROPERTY_FUNC(bool, left_percent)
	ADD_LUA_PROPERTY_FUNC(bool, top_percent)
	ADD_LUA_PROPERTY_FUNC(bool, right_percent)
	ADD_LUA_PROPERTY_FUNC(bool, bottom_percent)
	ADD_LUA_PROPERTY_FUNC(bool, width_percent)
	ADD_LUA_PROPERTY_FUNC(bool, height_percent)
	ADD_LUA_PROPERTY_FUNC(uint16_t, align)
	ADD_LUA_PROPERTY_FUNC(uint16_t, valign)
	ADD_LUA_PROPERTY_FUNC(bool, double_click)
	ADD_LUA_PROPERTY_FUNC(bool, ignore_events)
	ADD_LUA_PROPERTY_FUNC(bool, collide_through)
	ADD_LUA_PROPERTY_FUNC(uint16_t, focus_mode)

	inline bool is_eq(HEntityWraper e){return e.ID == ID;}
	inline bool is_null(){return ID == GUI_ENTITY_COUNT;}

	static void RegLuaClass()
	{
		getGlobalNamespace(LSTATE)
			.beginClass<HEntityWraper>("HEntity")
				.addConstructor<void (*)(void)>() 

				ADD_LUA_PROPERTY_DEF(bool, enable)
				ADD_LUA_PROPERTY_DEF(bool, visible)
				ADD_LUA_PROPERTY_DEF(int16_t, left)
				ADD_LUA_PROPERTY_DEF(int16_t, top)
				ADD_LUA_PROPERTY_DEF(int16_t, right)
				ADD_LUA_PROPERTY_DEF(int16_t, bottom)
				ADD_LUA_PROPERTY_DEF(uint16_t, width)
				ADD_LUA_PROPERTY_DEF(uint16_t, height)
				ADD_LUA_PROPERTY_DEF(bool, left_percent)
				ADD_LUA_PROPERTY_DEF(bool, top_percent)
				ADD_LUA_PROPERTY_DEF(bool, right_percent)
				ADD_LUA_PROPERTY_DEF(bool, bottom_percent)
				ADD_LUA_PROPERTY_DEF(bool, width_percent)
				ADD_LUA_PROPERTY_DEF(bool, height_percent)
				ADD_LUA_PROPERTY_DEF(uint16_t, align)
				ADD_LUA_PROPERTY_DEF(uint16_t, valign)
				ADD_LUA_PROPERTY_DEF(bool, double_click)
				ADD_LUA_PROPERTY_DEF(bool, ignore_events)
				ADD_LUA_PROPERTY_DEF(bool, collide_through)
				ADD_LUA_PROPERTY_DEF(uint16_t, focus_mode)
				
				.addFunction("is_eq", &HEntityWraper::is_eq)
				.addFunction("is_null", &HEntityWraper::is_null)

				.addFunction("Init", &HEntityWraper::Init)

				.addFunction("Activate", &HEntityWraper::Activate)
				.addFunction("Deactivate", &HEntityWraper::Deactivate)
				.addFunction("IsActivated", &HEntityWraper::IsActivated)
				.addFunction("IsActivatedBranch", &HEntityWraper::IsActivatedBranch)

				.addFunction("UpdatePos", &HEntityWraper::UpdatePos)
				.addFunction("UpdatePosSize", &HEntityWraper::UpdatePosSize)
				.addFunction("UpdateSize", &HEntityWraper::UpdateSize)

				.addFunction("RecalcPosSize", &HEntityWraper::RecalcPosSize)

				.addFunction("GetRectRelative", &HEntityWraper::GetRectRelative)
				.addFunction("GetRectAbsolute", &HEntityWraper::GetRectAbsolute)
				.addFunction("GetCorners", &HEntityWraper::GetCorners)
				.addFunction("GetClip", &HEntityWraper::GetClip)
								
				.addFunction("IsCollide", &HEntityWraper::IsCollide)
								
				.addFunction("GetParent", &HEntityWraper::GetParent)
				.addFunction("GetRoot", &HEntityWraper::GetRoot)
				.addFunction("GetID", &HEntityWraper::GetID)
				.addFunction("GetInherited", &HEntityWraper::GetInherited)

				.addFunction("AttachChild", &HEntityWraper::AttachChild)
				.addFunction("DetachChild", &HEntityWraper::DetachChild)

				.addFunction("GetChildById", &HEntityWraper::GetChildById)

				.addFunction("SetFocus", &HEntityWraper::SetFocus)
				.addFunction("SetHierarchyFocus", &HEntityWraper::SetHierarchyFocus)
				.addFunction("SetHierarchyFocusOnMe", &HEntityWraper::SetHierarchyFocusOnMe)

				//.addFunction("SetOnTop", &HEntityWraper::SetOnTop) // TODO

				.addFunction("CallbackHierarchy", &HEntityWraper::CallbackHierarchy)
				.addFunction("Callback", &HEntityWraper::Callback)
				.addFunction("SendEvent", &HEntityWraper::SendEvent)
				.addFunction("SendEventOnFocus", &HEntityWraper::SendEventOnFocus)

				.addFunction("GetFocus", &HEntityWraper::GetFocus)
				.addFunction("GetFocusLock", &HEntityWraper::GetFocusLock)

				.addFunction("AddRect", &HEntityWraper::AddRect)
				.addFunction("SetRect", &HEntityWraper::SetRect)
				.addFunction("GetRectShaderInst", &HEntityWraper::GetRectShaderInst)

				.addFunction("AddText", &HEntityWraper::AddText)
				.addFunction("SetText", &HEntityWraper::SetText)
				.addFunction("SetTextPos", &HEntityWraper::SetTextPos)
				.addFunction("SetTextColor", &HEntityWraper::SetTextColor)
				.addFunction("GetTextBounds", &HEntityWraper::GetTextBounds)
				.addFunction("GetText", &HEntityWraper::GetText)
				.addFunction("GetTextShaderInst", &HEntityWraper::GetTextShaderInst)

				.addFunction("Create", &HEntityWraper::Create)
				.addFunction("Destroy", &HEntityWraper::Destroy)
				.addFunction("IsExist", &HEntityWraper::IsExist)

				.addConstructor<void (*)(void)>() 

			.endClass();
	}

	uint32_t ID;
};