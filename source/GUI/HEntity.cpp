#include "stdafx.h"
#include "HEntity.h"

using namespace EngineCore;

void HEvent::SetHEntity(HEntityWraper obj)
{
	object_id = obj.GetID();
	object_sysid = obj.ID;
}

HEntityWraper HEvent::GetHEntity() const
{
	return HEntityWraper(object_sysid);
}

void HEvent::RegLuaClass()
{
	getGlobalNamespace(LSTATE)
		.beginClass<HEvent>("HEvent")
			.addData("event", &HEvent::event_id)
			.addData("coords", &HEvent::coords)
			.addData("collide", &HEvent::collide)
			.addData("key", &HEvent::key)
			.addData("id", &HEvent::object_id)
			.addProperty("symbol", &HEvent::GetSymbol)
			.addProperty("entity", &HEvent::GetHEntity, &HEvent::SetHEntity)
			.addConstructor<void (*)(void)>()
		.endClass();
}

HEntityStorage* HEntityStorage::instance = nullptr;

HEntityStorage::HEntityStorage()
{
	if(instance)
	{
		ERR("Only one instance of HEntityStorage is allowed!");
		return;
	}
	instance = this;

	entities.create(GUI_ENTITY_COUNT);

	free_id.resize(GUI_ENTITY_COUNT);
	for(uint32_t i=0; i<GUI_ENTITY_COUNT; i++)
		free_id[i] = uint32_t(i);
}

HEntity::HEntity() : lua_class(LSTATE)
{
	dclick_time = 0;
	named_children = nullptr;
	unnamed_children = nullptr;
	rects = nullptr;
	texts = nullptr;

	tick_func = nullptr;
	activate_func = nullptr;
	deactivate_func = nullptr;
	updatepossize_func = nullptr;
	callback_func = nullptr;

	first_child = nullptr;
	last_child = nullptr;
	trueTop = nullptr;

	ZeroEntity();
}

void HEntity::ZeroEntity()
{
	if(unnamed_children)
		unnamed_children->clear();
	if(named_children)
		named_children->clear();

	if(rects)
		rects->clear();
	
	if(texts)
		texts->clear();

	focus_lock = false;
	
	sys_ID = GUI_ENTITY_COUNT;
	ID.reserve(64);
	ID = "";
	parent = GUI_ENTITY_COUNT;

	enable = true;
	active = true;
	active_branch = true;
	visible = true;
	left = 0;
	top = 0;
	right = 0;
	bottom = 0;
	width = 0;
	height = 0;
	left_percent = false;
	top_percent = false;
	right_percent = false;
	bottom_percent = false;
	width_percent = false;
	height_percent = false;

	align = 0;
	valign = 0;

	double_click = false;
	dclick_time = 0;
	ignore_events = true;
	collide_through = false;

	hover = false;

	focus_mode = 0;
	
	relativeRect = MLRECT();
	globalPos.bottom = globalPos.left = globalPos.right = globalPos.top = 0;
	clipRect.bottom = clipRect.left = clipRect.right = clipRect.top = 0;

	focus = nullptr;
}

bool HEntity::Init(string id, LuaRef classRef)
{
	first_child = new HChild();
	last_child = new HChild();
	first_child->next = last_child;
	last_child->prev = first_child;

	trueTop = first_child;
	
	unnamed_children = new DArray<HChild*>;
	rects = new DArray<Image2D>;
	texts = new DArray<Text>;

	ID = id;

	return SetLuaClass(classRef);
}

bool HEntity::SetLuaClass(LuaRef& classref)
{
	if(!lua_class.isNil() || !classref.isTable())
		return false;
	lua_class = classref;

	SetLuaFuncs();
	return true;
}

void HEntity::SetLuaFuncs()
{
	if(lua_class.isNil())
		return;

	LuaRef lua_func = lua_class["onTick"];
	if(lua_func.isFunction())
	{
		if(tick_func)
			_DELETE(tick_func);
		tick_func = new LuaRef(lua_func);
	}

	lua_func = lua_class["onActivate"];
	if(lua_func.isFunction())	
	{
		if(activate_func)
			_DELETE(activate_func);
		activate_func = new LuaRef(lua_func);
	}

	lua_func = lua_class["onDeactivate"];
	if(lua_func.isFunction())
	{
		if(deactivate_func)
			_DELETE(deactivate_func);
		deactivate_func = new LuaRef(lua_func);
	}

	lua_func = lua_class["onMoveResize"];
	if(lua_func.isFunction())
	{
		if(updatepossize_func)
			_DELETE(updatepossize_func);
		updatepossize_func = new LuaRef(lua_func);
	}

	lua_func = lua_class["callback"];
	if(lua_func.isFunction())
	{
		if(callback_func)
			_DELETE(callback_func);
		callback_func = new LuaRef(lua_func);
	}
}

void HEntity::Update(float dt)
{
	if(dclick_time >= 0)
	{
		dclick_time += dt;
		if(dclick_time > DOUBLE_CLICK_INTERVAL)
		{
			dclick_time = -1.0f;
		}
	}

	if(!enable)
		return;
	
	if(unnamed_children)
		for(auto child: *unnamed_children)
		{
			auto ent = GET_HENTITY(child->e);
			if(!ent)
				break;
			ent->Update(dt);
		}

	if(named_children)
		for(auto& child: *named_children)
		{
			auto ent = GET_HENTITY(child.second->e);
			if(!ent)
				break;
			ent->Update(dt);
		}

	if(tick_func)
		(*tick_func)(lua_class, dt);
}

void HEntity::RegToDraw()
{
	if(!enable || !visible)
		return;

	if(!isInRect(globalPos, clipRect))
		return;

	if(rects)
		for(auto& it: *rects)
			it.Draw();
	if(texts)
		for(auto& jt: *texts)
			jt.Draw();

	HChild* it = first_child->next;
	while(it != last_child)
	{
		auto ent = GET_HENTITY(it->e);
		if(!ent)
			break;

		ent->RegToDraw();

		it = it->next;
	}
}

void HEntity::Close()
{
	if(first_child && last_child)
	{
		HChild* it = first_child->next;
		while(it != last_child)
		{
			HEntityStorage::Get()->DefferedDestroy(it->e);
			auto removedCh = it;
			it = it->next;
			_DELETE(removedCh);
		}
	}

	_DELETE(first_child);
	_DELETE(last_child);

	_DELETE(unnamed_children);
	_DELETE(named_children);

	if(rects)
	{
		for(auto& it: *rects)
			it.Close();
		_DELETE(rects);
	}

	if(texts)
	{
		for(auto& it: *texts)
			it.Close();
		_DELETE(texts);
	}

	_DELETE(tick_func);
	_DELETE(activate_func);
	_DELETE(deactivate_func);
	_DELETE(updatepossize_func);
	_DELETE(callback_func);

	lua_class = LuaRef(LSTATE);

	ZeroEntity();
}

HEvent HEntity::LocalCallback(HEvent e)
{
	if( e.event_id == GuiEvents::GE_NULL )
		return e;

	if(!callback_func)
		return e;
	return (*callback_func)(lua_class, e);
}

void HEntity::LocalCallbackHierarchy(HEvent e)
{
	if(parent == GUI_ENTITY_COUNT)
	{
		LocalCallback(e);
		return;
	}

	auto parent_pointer = GET_HENTITY(parent);
	if(parent_pointer)
		parent_pointer->LocalCallbackHierarchy(LocalCallback(e));
	else
		LocalCallback(e);
}

void HEntity::SendEvent(HEvent e)
{
	if(focus_lock && focus)
	{
		auto fent = GET_HENTITY(focus->e);
		if(!fent)
			return;
		fent->SendEvent(e);
		LocalCallback(e);
		return;
	}

	if(named_children)
	{
		for(auto& i: *named_children)
		{
			auto ent = GET_HENTITY(i.second->e);
			if(!ent)
				return;
			ent->SendEvent(e);
		}
	}

	for(auto& i: *unnamed_children)
	{
		auto ent = GET_HENTITY(i->e);
		if(!ent)
			return;
		ent->SendEvent(e);
	}

	LocalCallback(e);
}

void HEntity::SendEventOnFocus(HEvent e)
{
	if(!focus)
	{
		LocalCallback(e);
		return;
	}
	
	auto fent = GET_HENTITY(focus->e);
	if(!fent)
		return;
	fent->SendEventOnFocus(e);
	LocalCallback(e);
}
	
HEvent HEntity::KeyPressed(const KeyEvent &arg, bool pressed)
{
	HEvent gui_event;

	if(!focus)
	{
		if(pressed)
			gui_event.event_id = GuiEvents::GE_KEY_DOWN;
		else
			gui_event.event_id = GuiEvents::GE_KEY_UP;

		gui_event.key = arg.code;
		gui_event.sym = arg.wc;
		gui_event.object_sysid = sys_ID;
		gui_event.object_id = ID;

		return LocalCallback(gui_event);
	}

	auto fent = GET_HENTITY(focus->e);
	if(!fent)
		return HEvent();

	return LocalCallback(fent->KeyPressed(arg, pressed));
}

HEvent HEntity::MousePressed(const MouseEventClick &arg, bool pressed)
{
	HEvent gui_event;

	// children
	if(focus_lock && focus)
	{
		auto fent = GET_HENTITY(focus->e);
		if(!fent)
			return HEvent();

		return LocalCallback(fent->MousePressed(arg, pressed));
	}

	HChild* it = last_child->prev;
	while(it != first_child)
	{
		auto ent = GET_HENTITY(it->e);
		if(!ent)
			break;

		if( ent->enable && !ent->ignore_events && ent->IsCollide(arg.x, arg.y) )
			return LocalCallback(ent->MousePressed(arg, pressed));

		it = it->prev;
	}

	// self
	if(pressed)
	{
		gui_event.event_id = GuiEvents::GE_MOUSE_DOWN;
		if( arg.btn == MOUSE_LEFT && double_click )
		{
			if(dclick_time < 0)
				dclick_time = 0;
			else
			{
				gui_event.event_id = GuiEvents::GE_MOUSE_DBLCLICK;
				dclick_time = -1;
			}
		}
	}
	else
		gui_event.event_id = GuiEvents::GE_MOUSE_UP;

	gui_event.coords.x = arg.x;
	gui_event.coords.y = arg.y;
	gui_event.object_sysid = sys_ID;
	gui_event.object_id = ID;

	switch(arg.btn)
	{
	case MOUSE_LEFT:
		gui_event.key = KEY_LBUTTON;
		break;
	case MOUSE_MIDDLE:
		gui_event.key = KEY_MBUTTON;
		break;
	case MOUSE_RIGHT:
		gui_event.key = KEY_RBUTTON;
		break;
	}

	return LocalCallback(gui_event);
}

HEvent HEntity::MouseWheel(const MouseEventWheel &arg)
{
	if(focus && focus_lock)
	{
		auto fent = GET_HENTITY(focus->e);
		if(!fent)
			return HEvent();

		return LocalCallback(fent->MouseWheel(arg));
	}

	HEvent gui_event;
	gui_event.event_id = GuiEvents::GE_ERROR;

	HChild* it = last_child->prev;
	while(it != first_child)
	{
		auto ent = GET_HENTITY(it->e);
		if(!ent)
			break;

		if(ent->ignore_events || !ent->enable)
		{
			it = it->prev;
			continue;
		}

		if(ent->IsCollide(arg.x, arg.y))
		{
			gui_event = LocalCallback(ent->MouseWheel(arg));
			if(!ent->collide_through)
				break;
		}

		it = it->prev;
	}
	if(gui_event.event_id != GuiEvents::GE_ERROR)
		return gui_event;

	gui_event.event_id = GuiEvents::GE_MOUSE_WHEEL;
	gui_event.coords.x = gui_event.coords.y = arg.wheel;
	gui_event.object_sysid = sys_ID;
	gui_event.object_id = ID;

	return LocalCallback(gui_event);
}

HEvent HEntity::MouseMove(const MouseEvent &arg, bool collide)
{
	// children
	if(focus_lock && focus)
	{
		auto fent = GET_HENTITY(focus->e);
		if(!fent)
			return HEvent();

		return LocalCallback(fent->MouseMove(arg, true));
	}

	HEvent res_event;
	HEvent temp_event;
	bool collidecheck = collide;

	HChild* it = last_child->prev;
	while(it != first_child)
	{
		auto ent = GET_HENTITY(it->e);
		if(!ent)
			break;

		if(ent->ignore_events || !ent->enable)
		{
			it = it->prev;
			continue;
		}

		if(collidecheck)
		{
			if(ent->IsCollide(arg.x, arg.y))
			{
				temp_event = LocalCallback(ent->MouseMove(arg, true));
				if(ent->collide_through)
					collidecheck = true;
				else
					collidecheck = false;
			}
			else
			{
				if(ent->hover)
					temp_event = LocalCallback(ent->MouseMove(arg, false));
			}
		}
		else
		{
			if(ent->hover)
				temp_event = LocalCallback(ent->MouseMove(arg, false));
		}

		if(temp_event.event_id != GuiEvents::GE_ERROR && temp_event.event_id != GuiEvents::GE_NULL && 
			(temp_event.event_id != GuiEvents::GE_MOUSE_MOVE || res_event.event_id == GuiEvents::GE_ERROR))	 // todo
			res_event = temp_event;

		it = it->prev;
	}

	// self
	HEvent gui_event;

	if(collide && hover && !arg.out)
		gui_event.event_id = GuiEvents::GE_MOUSE_MOVE;
	else if(collide && !hover && !arg.out)
	{
		gui_event.event_id = GuiEvents::GE_MOUSE_HOVER;
		hover = true;
	}
	else if(!collide && hover || arg.out)
	{
		gui_event.event_id = GuiEvents::GE_MOUSE_OUT;
		hover = false;
	}
	else
		return res_event;

	/*if(arg.out)
		gui_event.event_id = GuiEvents::GE_MOUSE_OUTOFWIN;
	else
		gui_event.event_id = GuiEvents::GE_MOUSE_MOVE;*/

	gui_event.collide = collide;
	gui_event.coords.x = arg.x;
	gui_event.coords.y = arg.y;
	gui_event.object_sysid = sys_ID;
	gui_event.object_id = ID;

	temp_event = LocalCallback(gui_event);

	if(temp_event.event_id != GuiEvents::GE_ERROR && temp_event.event_id != GuiEvents::GE_NULL && 
		(temp_event.event_id != GuiEvents::GE_MOUSE_MOVE || res_event.event_id == GuiEvents::GE_ERROR))	 // todo
		return temp_event;
	return res_event;
}

HEvent HEntity::DropEvent(uint32_t dropEvent, POINT pos)
{
	HEvent gui_event;

	// children
	if(focus_lock && focus)
	{
		auto fent = GET_HENTITY(focus->e);
		if(!fent)
			return HEvent();

		return LocalCallback(fent->DropEvent(dropEvent, pos));
	}

	HChild* it = last_child->prev;
	while(it != first_child)
	{
		auto ent = GET_HENTITY(it->e);
		if(!ent)
			break;

		if( !ent->ignore_events && ent->IsCollide(int16_t(pos.x), int16_t(pos.y)) )
			return LocalCallback(ent->DropEvent(dropEvent, pos));

		it = it->prev;
	}
	
	// self
	gui_event.event_id = dropEvent;
	gui_event.coords = pos;
	gui_event.object_sysid = sys_ID;
	gui_event.object_id = ID;
	return LocalCallback(gui_event);
}

bool HEntity::IsCollide(int16_t x, int16_t y)
{
	if( x >= globalPos.left && x <= globalPos.right && 
		y >= globalPos.top && y <= globalPos.bottom )
		return true;
	else return false;
}

uint32_t HEntity::GetParent()
{
	return parent;
}

uint32_t HEntity::GetRoot()
{
	if(parent == GUI_ENTITY_COUNT)
		return sys_ID;

	auto parent_pointer = GET_HENTITY(parent);
	if(!parent_pointer)
		return GUI_ENTITY_COUNT;
	return parent_pointer->GetRoot();
}
	
bool HEntity::AttachChild(uint32_t e)
{
	if(e == GUI_ENTITY_COUNT)
		return false;

	auto ent = GET_HENTITY(e);
	if(!ent)
		return false;

	HChild* newChild = new HChild(e);
	HChild* top = nullptr;
	if(ent->focus_mode != HENTITY_FOCUSMODE_ONTOP)
		top = trueTop;
	else
		top = last_child->prev;

	auto next = top->next;

	top->next = newChild;
	newChild->prev = top;
	newChild->next = next;
	next->prev = newChild;

	if(ent->focus_mode != HENTITY_FOCUSMODE_ONTOP)
		trueTop = newChild;

	if(ent->ID.size() > 0)
	{
		if(!named_children)
			named_children = new unordered_map<string, HChild*>;
		named_children->insert(make_pair(ent->ID, newChild));
	}
	else
		unnamed_children->push_back(newChild);

	ent->parent = sys_ID;
	return true;
}

bool HEntity::DetachChild(uint32_t e)
{
	if(e == GUI_ENTITY_COUNT)
		return false;

	auto ent = GET_HENTITY(e);
	if(!ent)
		return false;

	HChild* removedChild = nullptr;
	if(ent->ID.size() > 0)
	{
		if(!named_children)
		{
			ERR("No named child in: %s", ID.data());
			return false;
		}
		auto it = named_children->find(ent->ID);
		if(it == named_children->end())
		{
			ERR("Wrong child: %s", ent->ID.data());
			return false;
		}
		removedChild = it->second;
		named_children->erase(it);
	}
	else
	{
		if(unnamed_children)
			for(auto& i: *unnamed_children)
				if(i->e == e)
				{
					removedChild = i;
					unnamed_children->erase_and_pop_back(i);
					break;
				}
		if(!removedChild)
		{
			ERR("Wrong child: %u", e);
			return false;
		}
	}
	
	if(trueTop == removedChild)
		trueTop = removedChild->prev;

	if(focus == removedChild) // drop focus?
		focus = nullptr;

	removedChild->next->prev = removedChild->prev;
	removedChild->prev->next = removedChild->next;

	ent->parent = GUI_ENTITY_COUNT;
	_DELETE(removedChild);
	return true;
}

uint32_t HEntity::GetChildById(string id)
{
	if(!named_children)
	{
		DBG("Child with id \"%s\" do not exist", id.data());
		return GUI_ENTITY_COUNT;
	}

	auto it = named_children->find(id);
	if(it != named_children->end())
		return it->second->e;
	
	if(named_children)
		for(auto& ch_it: *named_children)
		{
			auto ent = GET_HENTITY(ch_it.second->e);
			if(!ent) continue;
			uint32_t res = GUI_ENTITY_COUNT;
			if( (res = ent->GetChildById(id)) != GUI_ENTITY_COUNT )
				return res;
		}

	DBG("Child with id \"%s\" do not exist", id.data());
	return GUI_ENTITY_COUNT;
}

HEntity::HChild* HEntity::GetHChild(uint32_t e, HEntity** outEnt)
{
	auto ent = GET_HENTITY(e);
	if(!ent)
		return nullptr;

	HChild* findedChild = nullptr;
	if(ent->ID.size() > 0)
	{
		auto it = named_children->find(ent->ID);
		if(it == named_children->end())
			return nullptr;
		findedChild = it->second;
	}
	else
	{
		for(auto& i: *unnamed_children)
			if(i->e == e)
			{
				findedChild = i;
				break;
			}
	}
	if(outEnt)
		*outEnt = ent;
	return findedChild;
}
/*
void HEntity::SetOnTop(uint32_t e)
{
	HEntity* ent = nullptr;
	auto child = GetHChild(e, &ent);
	if(!child)
		return;

	if(ent->focus_mode != HENTITY_FOCUSMODE_ONTOP)
		ent->focus_mode = HENTITY_FOCUSMODE_ONTOP;

	auto top = last_child->prev;
	if(top == child)
		return;

	auto next = top->next;

	top->next = child;
	child->prev = top;
	child->next = next;
	next->prev = child;
}*/

void HEntity::SetFocus(uint32_t e, bool lock, bool overlay)
{
	if(focus_lock && e != focus->e && e != GUI_ENTITY_COUNT)
		return;
	else if(focus && e == focus->e && e != GUI_ENTITY_COUNT)
	{
		focus_lock = lock;
		return;
	}

	HEvent gui_event;
	gui_event.event_id = GuiEvents::GE_UNFOCUS;

	if(e == GUI_ENTITY_COUNT)
	{
		if(focus && !overlay)
		{
			gui_event.object_sysid = focus->e;
			auto fent = GET_HENTITY(focus->e);
			if(fent)
			{
				gui_event.object_id = fent->ID;
				fent->LocalCallbackHierarchy(gui_event);
			}
		}
		focus = nullptr;
		focus_lock = false;
		return;
	}

	HEntity* ent = nullptr;
	auto child = GetHChild(e, &ent);
	if(!child)
	{
		ERR("Wrong child: %u", e);
		return;
	}

	auto oldfocus = focus;

	focus = child;
	focus_lock = lock;

	if(ent->focus_mode == HENTITY_FOCUSMODE_NORMAL && trueTop != child)
	{
		child->prev->next = child->next;
		child->next->prev = child->prev;

		child->prev = trueTop;
		child->next = trueTop->next;
		trueTop->next->prev = child;
		trueTop->next = child;

		trueTop = child;
	}

	if(oldfocus && !overlay)
	{
		gui_event.object_sysid = oldfocus->e;
		auto ofent = GET_HENTITY(oldfocus->e);
		if(ofent)
		{
			gui_event.object_id = ofent->ID;
			ofent->LocalCallbackHierarchy(gui_event);
		}
		//gui_event.object_sysid = oldfocus->e; ??????
	}

	gui_event.event_id = GuiEvents::GE_FOCUS;

	if(!focus)
		return;

	auto fent = GET_HENTITY(focus->e);
	if(fent)
	{
		gui_event.object_id = fent->ID;
		fent->LocalCallbackHierarchy(gui_event);
	}
}

void HEntity::SetHierarchyFocus(uint32_t e, bool lock, bool overlay)
{
	if(parent != GUI_ENTITY_COUNT)
	{
		auto parent_pointer = GET_HENTITY(parent);
		if(!parent_pointer)
			return;

		parent_pointer->SetHierarchyFocus(sys_ID, lock, overlay);
	}

	SetFocus(e, lock, overlay);
}

void HEntity::SetHierarchyFocusOnMe(bool lock, bool overlay)
{
	if(parent == GUI_ENTITY_COUNT)
		return;
	auto parent_pointer = GET_HENTITY(parent);
	if(parent_pointer)
		parent_pointer->SetHierarchyFocus(sys_ID, lock, overlay);
}

uint32_t HEntity::GetFocus()
{
	if(!focus)
		return GUI_ENTITY_COUNT;
	return focus->e;
}

bool HEntity::GetFocusLock()
{
	return focus_lock;
}

void HEntity::CalcPos(Window* win)
{
	MLRECT parent_rect;

	if(parent == GUI_ENTITY_COUNT)
	{
		if(!win)
			return;
		parent_rect = MLRECT(0, 0, win->m_desc.width, win->m_desc.height);
	}
	else
		parent_rect = GET_HENTITY(parent)->GetRectAbsolute();
	
	switch(align)
	{
	case HENTITY_ALIGN_LEFT:
		if(left_percent) 
			relativeRect.left = FloatRoundInt(parent_rect.width * (float(left) * 0.01f));
		else
			relativeRect.left = left;
		globalPos.left = parent_rect.left + relativeRect.left;
		globalPos.right = globalPos.left + relativeRect.width;
		break;

	case HENTITY_ALIGN_RIGHT:
		{
			int16_t temp_right = 0;
			if(right_percent)
			{
				temp_right = FloatRoundInt(parent_rect.width * (float(100 - right) * 0.01f));
				globalPos.right = parent_rect.left + temp_right;
			}
			else
			{
				temp_right = right;
				globalPos.right = parent_rect.left + parent_rect.width - temp_right;
			}
			relativeRect.left = temp_right - relativeRect.width;
			globalPos.left = globalPos.right - relativeRect.width;
		}
		break;

	case HENTITY_ALIGN_CENTER:
		if(left_percent)
		{
			relativeRect.left = FloatRoundInt(parent_rect.width * (float(50 + left) * 0.01f)) - relativeRect.width/2;
			globalPos.left = parent_rect.left + relativeRect.left;
		}
		else
		{
			relativeRect.left = parent_rect.width/2 + left - relativeRect.width/2;
			globalPos.left = parent_rect.left + relativeRect.left;
		}
		globalPos.right = globalPos.left + relativeRect.width;
		break;

	case HENTITY_ALIGN_BOTH:
		if(left_percent) 
			relativeRect.left = FloatRoundInt(parent_rect.width * (float(left) * 0.01f));
		else
			relativeRect.left = left;
		globalPos.left = parent_rect.left + relativeRect.left;

		if(right_percent)
			globalPos.right = parent_rect.left + FloatRoundInt(parent_rect.width * (float(100 - right) * 0.01f));
		else
			globalPos.right = parent_rect.left + parent_rect.width - right;
		
		relativeRect.width = globalPos.right - globalPos.left; 
		break;
	}

	switch(valign)
	{
	case HENTITY_VALIGN_TOP:
		if(top_percent) 
			relativeRect.top = FloatRoundInt(parent_rect.height * (float(top) * 0.01f));
		else
			relativeRect.top = top;
		globalPos.top = parent_rect.top + relativeRect.top;
		globalPos.bottom = globalPos.top + relativeRect.height;
		break;

	case HENTITY_VALIGN_BOTTOM:
		{
			int16_t temp_bottom = 0;
			if(bottom_percent)
			{
				temp_bottom = FloatRoundInt(parent_rect.height * (float(100 - bottom) * 0.01f));
				globalPos.bottom = parent_rect.top + temp_bottom;
			}
			else
			{
				temp_bottom = bottom;
				globalPos.bottom = parent_rect.top + parent_rect.height - temp_bottom;
			}
			relativeRect.top = temp_bottom - relativeRect.height;
			globalPos.top = globalPos.bottom - relativeRect.height;
		}
		break;

	case HENTITY_VALIGN_MIDDLE:
		if(top_percent)
		{
			relativeRect.top = FloatRoundInt(parent_rect.height * (float(50 + top) * 0.01f)) - relativeRect.height/2;
			globalPos.top = parent_rect.top + relativeRect.top;
		}
		else
		{
			relativeRect.top = parent_rect.height/2 + top - relativeRect.height/2;
			globalPos.top = parent_rect.top + relativeRect.top;
		}
		globalPos.bottom = globalPos.top + relativeRect.height;
		break;

	case HENTITY_VALIGN_BOTH:
		if(top_percent) 
			relativeRect.top = FloatRoundInt(parent_rect.height * (float(top) * 0.01f));
		else
			relativeRect.top = top;
		globalPos.top = parent_rect.top + relativeRect.top;

		if(bottom_percent)
			globalPos.bottom = parent_rect.top + FloatRoundInt(parent_rect.height * (float(100 - bottom) * 0.01f));
		else
			globalPos.bottom = parent_rect.top + parent_rect.height - bottom;
		
		relativeRect.height = globalPos.bottom - globalPos.top; 
		break;
	}
}

void HEntity::CalcSize(Window* win)
{
	MLRECT parent_rect;

	if(parent == GUI_ENTITY_COUNT)
	{
		if(!win)
			return;
		parent_rect = MLRECT(0, 0, win->m_desc.width, win->m_desc.height);
	}
	else
		parent_rect = GET_HENTITY(parent)->GetRectAbsolute();
	
	if(align != HENTITY_ALIGN_BOTH)
	{
		if(width_percent)
			relativeRect.width = FloatRoundInt(parent_rect.width * (float(width) * 0.01f));
		else
			relativeRect.width = width;

		if(align == HENTITY_ALIGN_RIGHT)
			globalPos.left = globalPos.right - relativeRect.width;
		else
			globalPos.right = globalPos.left + relativeRect.width;
	}

	if(valign != HENTITY_VALIGN_BOTH)
	{
		if(height_percent)
			relativeRect.height = FloatRoundInt(parent_rect.height * (float(height) * 0.01f));
		else
			relativeRect.height = height;
	
		if(valign == HENTITY_VALIGN_BOTTOM)
			globalPos.top = globalPos.bottom - relativeRect.height;
		else
			globalPos.bottom = globalPos.top + relativeRect.height;
	}
}

void HEntity::UpdateChildrenPosSize()
{
	for(auto it: *unnamed_children)
		GET_HENTITY(it->e)->UpdatePosSize();
	if(named_children)
		for(auto it: *named_children)
			GET_HENTITY(it.second->e)->UpdatePosSize();
}

void HEntity::UpdateChildrenPos()
{
	for(auto it: *unnamed_children)
		GET_HENTITY(it->e)->UpdatePos();
	if(named_children)
		for(auto it: *named_children)
			GET_HENTITY(it.second->e)->UpdatePos();
}

void HEntity::UpdateClip()
{
	if(parent == GUI_ENTITY_COUNT)
	{
		clipRect = globalPos;
	}
	else
	{
		RECT clipParent = GET_HENTITY(parent)->GetClip();

		clipRect.left = max(globalPos.left, clipParent.left);
		clipRect.right = min(globalPos.right, clipParent.right);
		clipRect.top = max(globalPos.top, clipParent.top);
		clipRect.bottom = min(globalPos.bottom, clipParent.bottom);
	}

	if(rects)
		for(auto& it: *rects)
			it.GetShaderInst()->SetVector(XMFLOAT4(float(clipRect.left), float(clipRect.top), float(clipRect.right), float(clipRect.bottom)), 0);
	if(texts)
		for(auto& jt: *texts)
			jt.SetClip(clipRect);
}

void HEntity::ActivateLoop()
{
	if(!active)
		return;

	active_branch = true;

	if(activate_func)
		(*activate_func)(lua_class);

	for(auto it: *unnamed_children)
		GET_HENTITY(it->e)->ActivateLoop();
	if(named_children)
		for(auto it: *named_children)
			GET_HENTITY(it.second->e)->ActivateLoop();
}

void HEntity::DeactivateLoop()
{
	active_branch = false;

	if(deactivate_func)
		(*deactivate_func)(lua_class);

	for(auto it: *unnamed_children)
		GET_HENTITY(it->e)->DeactivateLoop();
	if(named_children)
		for(auto it: *named_children)
			GET_HENTITY(it.second->e)->DeactivateLoop();
}