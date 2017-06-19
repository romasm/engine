local BG_MAT = "../resources/shaders/gui/rect"
local SHADOW_MAT = "../resources/shaders/gui/shadow"
local HEADER_MAT = "../resources/shaders/gui/color"
local SCROLL_SPEED = 80

-- support
if not GuiWindowSup then GuiWindowSup = class(GuiEntity) end

function GuiWindowSup:callback(eventData)
    local res = eventData
    
    if eventData.event == GUI_EVENTS.UNFOCUS then
        if eventData.entity:is_eq(self.entity) then
            self.entity:SetFocus(HEntity())
        end
    elseif eventData.event == GUI_EVENTS.MOUSE_DOWN then
        if eventData.entity:is_eq(self.entity) then
            self.entity:SetHierarchyFocusOnMe(false)
            self.entity:SetFocus(HEntity())
        end
    end

    return self._base.callback(self, res)
end

function GuiClientarea(props)
    props.id = "clientarea"
    props.ignore_events = false
    props.collide_through = false
    
    return GuiWindowSup(props)
end

function GuiBody(props)
    props.id = "body"
    props.ignore_events = false
    props.collide_through = false

    if props.groupstack then
        return GuiGroup(props, true)
    else
        return GuiWindowSup(props)
    end
end

if not GuiWindow then GuiWindow = class(GuiEntity) end

-- public
function GuiWindow:init(props)
    self.scrollable = {x = true, y = true}
    self.resizeable = {x = true, y = true}
    self.clamp_resize = {x = true, y = true}
    self.closeable = true
    self.dragable = true

    self.independent = false

    self.header_size = 25

    self.cleintarea_padding = { l = 2, r = 2, t = 2, b = 2 }

    self.close = {}
    self.header = {}
    self.scrollX = {}
    self.scrollY = {}
    self.resize = {}

    self.background = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_live = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0)
    }

    self.border = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_live = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        width = 0
    }
    
    self.shadow = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_live = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        width = 0
    }
    
    self._base.init(self, props)
    
    if self.independent == true then
        self.sys_win = CoreGui.SysWindows.Create()
        
        -- todo
        self.sys_win:HideWinBorder(false)
        self.sys_win:SetCaptionRect(0, self.header_size, 0, self.header_size)
        self.sys_win:SetBorderSize(4)
        self.sys_win:SetIcons("../resources/textures/icons/main_icon.ico", "../resources/textures/icons/main_icon.ico")

        self.sys_win.caption_text = self.header.str ~= nil and self.header.str or "[empty header]"
        
        self.sys_win:SetPosSize(self.entity.left, self.entity.top, self.entity.width, self.entity.height)
        self.entity.width = 100
        self.entity.width_percent = true
        self.entity.height = 100
        self.entity.height_percent = true
        self.entity.left = 0
        self.entity.top = 0
        
        local sys_bg_color = CoreGui.GetColor('main_win_bg')
	    self.sys_win:SetColorBgPtr(sys_bg_color)
	    self.sys_win:SetColorBorderPtr(CoreGui.GetColor('main_win_brd'))
	    self.sys_win:SetColorBorderFocusPtr(CoreGui.GetColor('main_win_brd_focus'))
	    self.sys_win:SetAlpha(sys_bg_color.w)
                
        self.sys_win_root = GuiRoot(CoreGui.GetRootByWindow(self.sys_win))

        self.sys_win_root.entity:AttachChild(self.entity)
    end
    
    self.clientarea_rect = {l = 0, r = 0, t = 0, b = 0, w = 0, h = 0}

    self.clientarea_rect.l = self.border.width
    self.clientarea_rect.r = self.border.width
    self.clientarea_rect.t = self.border.width
    self.clientarea_rect.b = self.border.width

    -- children
    if self.shadow.width > 0 then
        self.shadow_rect = self.entity:AddRect(SHADOW_MAT)
        self.shadow_mat = self:SetRectMaterial(self.shadow_rect)
    end
    
    self.bg_rect = self.entity:AddRect(BG_MAT)
    self.bg_rect_mat = self:SetRectMaterial(self.bg_rect)

    if self.header_size > 0 then
        self.header_rect = self.entity:AddRect(HEADER_MAT)
        self.header_rect_mat = self:SetRectMaterial(self.header_rect)

        self.header_str = GuiString(self.header)
        self.entity:AttachChild(self.header_str.entity)

        self.clientarea_rect.t = math.max(self.clientarea_rect.t, self.header_size)
    end

    if self.closeable then
        self.close_btn = GuiButton(self.close)
        self.entity:AttachChild(self.close_btn.entity)
    end

    if self.scrollable.x then
        self.scrollX.orient = GUI_SLIDER_ORIENT.HORZ
        self.scroll_x_sld = GuiSlider(self.scrollX)
        self.entity:AttachChild(self.scroll_x_sld.entity)
        self.scroll_x_sld.entity.align = GUI_ALIGN.BOTH
        self.scroll_x_sld.entity.valign = GUI_VALIGN.BOTTOM
        self.scroll_x_sld.entity.bottom = self.border.width + (self.scrollX.bottom ~= nil and self.scrollX.bottom or 0) + self.cleintarea_padding.b
        self.scroll_x_sld.entity.left = self.border.width + (self.scrollX.left ~= nil and self.scrollX.left or 0) + self.cleintarea_padding.l
        self.scroll_x_sld.entity.right = self.border.width + ((self.scrollable.y and self.scrollY.width ~= nil) and self.scrollY.width or 0) + (self.scrollX.right ~= nil and self.scrollX.right or 0) + self.cleintarea_padding.r
        
        self.clientarea_rect.b = self.scroll_x_sld.entity.bottom - self.cleintarea_padding.b + self.scrollX.height + (self.scrollX.top ~= nil and self.scrollX.top or 0)
    end

    if self.scrollable.y then
        self.scrollY.orient = GUI_SLIDER_ORIENT.VERT
        self.scroll_y_sld = GuiSlider(self.scrollY)
        self.entity:AttachChild(self.scroll_y_sld.entity)
        self.scroll_y_sld.entity.align = GUI_ALIGN.RIGHT
        self.scroll_y_sld.entity.valign = GUI_VALIGN.BOTH
        self.scroll_y_sld.entity.right = self.border.width + (self.scrollY.right ~= nil and self.scrollY.right or 0) + self.cleintarea_padding.r
        self.scroll_y_sld.entity.bottom = self.border.width + ((self.scrollable.x and self.scrollX.height ~= nil) and self.scrollX.height or 0) + (self.scrollY.bottom ~= nil and self.scrollY.bottom or 0) + self.cleintarea_padding.b
        self.scroll_y_sld.entity.top = math.max(self.header_size, self.border.width) + (self.scrollY.top ~= nil and self.scrollY.top or 0) + self.cleintarea_padding.t
        
        self.clientarea_rect.r = self.scroll_y_sld.entity.right - self.cleintarea_padding.r + self.scrollY.width + (self.scrollY.left ~= nil and self.scrollY.left or 0)
    end

    if self.resizeable.x or self.resizeable.y then
        self.resize_btn = GuiButton(self.resize)
        self.entity:AttachChild(self.resize_btn.entity)
        self.resize_btn.entity.align = GUI_ALIGN.RIGHT
        self.resize_btn.entity.valign = GUI_VALIGN.BOTTOM
        self.resize_btn.entity.right = self.border.width + (self.resize.right ~= nil and self.resize.right or 0)
        self.resize_btn.entity.bottom = self.border.width + (self.resize.bottom ~= nil and self.resize.bottom or 0)
    end

    self.clientarea_rect.l = self.clientarea_rect.l + self.cleintarea_padding.l
    self.clientarea_rect.r = self.clientarea_rect.r + self.cleintarea_padding.r
    self.clientarea_rect.t = self.clientarea_rect.t + self.cleintarea_padding.t
    self.clientarea_rect.b = self.clientarea_rect.b + self.cleintarea_padding.b

    local cl_ent = self.entity:GetChildById("clientarea")
    if cl_ent:is_null() then return end
    
    self.clientarea_ent = cl_ent:GetInherited()
    self.clientarea_ent.window = self

    cl_ent.align = GUI_ALIGN.BOTH
    cl_ent.valign = GUI_VALIGN.BOTH
    cl_ent.left = self.clientarea_rect.l
    cl_ent.right = self.clientarea_rect.r
    cl_ent.top = self.clientarea_rect.t
    cl_ent.bottom = self.clientarea_rect.b
    
    local bd_ent = cl_ent:GetChildById("body")
    if bd_ent:is_null() then return end

    self.body_ent = bd_ent:GetInherited()
    self.body_ent.window = self

    self.state_drag = false
    self.state_resize = false
    self.drag_offset = {x = 0, y = 0}

    self.scroll = {x = 0, y = 0}
    self.scroll_enabled = {x = true, y = true}
    
    self:UpdateProps()
    
	if self.independent == true then
        self.sys_win:Show(true)
    end
end

function GuiWindow:ApplyProps(props)
    self._base.ApplyProps(self, props)

    if props.closeable ~= nil then self.closeable = props.closeable end
    if props.header_size ~= nil then self.header_size = props.header_size end
    if props.dragable ~= nil then self.dragable = props.dragable end
    if props.independent ~= nil then self.independent = props.independent end
    if props.cleintarea_padding ~= nil then 
        if props.cleintarea_padding.l ~= nil then self.cleintarea_padding.l = props.cleintarea_padding.l end
        if props.cleintarea_padding.r ~= nil then self.cleintarea_padding.r = props.cleintarea_padding.r end
        if props.cleintarea_padding.t ~= nil then self.cleintarea_padding.t = props.cleintarea_padding.t end
        if props.cleintarea_padding.b ~= nil then self.cleintarea_padding.b = props.cleintarea_padding.b end
    end
    if props.resizeable ~= nil then 
        if props.resizeable.x ~= nil then self.resizeable.x = props.resizeable.x end 
        if props.resizeable.y ~= nil then self.resizeable.y = props.resizeable.y end 
    end
    if props.clamp_resize ~= nil then 
        if props.clamp_resize.x ~= nil then self.clamp_resize.x = props.clamp_resize.x end 
        if props.clamp_resize.y ~= nil then self.clamp_resize.y = props.clamp_resize.y end 
    end
    if props.scrollable ~= nil then 
        if props.scrollable.x ~= nil then self.scrollable.x = props.scrollable.x end 
        if props.scrollable.y ~= nil then self.scrollable.y = props.scrollable.y end 
    end

    if props.close ~= nil then 
        self.close = props.close 
        self.close.id = "close_btn"
    end

    if props.header ~= nil then 
        self.header = props.header
    end

    if props.scrollX ~= nil then 
        self.scrollX = props.scrollX 
    end
    if props.scrollY ~= nil then 
        self.scrollY = props.scrollY 
    end

    if props.resize ~= nil then 
        self.resize = props.resize 
    end

    if props.background ~= nil then
        if props.background.color ~= nil then 
            self.background.color = type(props.background.color) == 'string' and CoreGui.GetColor(props.background.color) or props.background.color end
        if props.background.color_live ~= nil then 
            self.background.color_live = type(props.background.color_live) == 'string' and CoreGui.GetColor(props.background.color_live) or props.background.color_live end
        if props.background.color_nonactive ~= nil then 
            self.background.color_nonactive = type(props.background.color_nonactive) == 'string' and CoreGui.GetColor(props.background.color_nonactive) or props.background.color_nonactive end
    end

    if props.border ~= nil then
        if props.border.color ~= nil then 
            self.border.color = type(props.border.color) == 'string' and CoreGui.GetColor(props.border.color) or props.border.color end
        if props.border.color_live ~= nil then 
            self.border.color_live = type(props.border.color_live) == 'string' and CoreGui.GetColor(props.border.color_live) or props.border.color_live end
        if props.border.color_nonactive ~= nil then 
            self.border.color_nonactive = type(props.border.color_nonactive) == 'string' and CoreGui.GetColor(props.border.color_nonactive) or props.border.color_nonactive end
        
        if props.border.width ~= nil then self.border.width = props.border.width end
    end

    if props.shadow ~= nil then
        if props.shadow.color ~= nil then 
            self.shadow.color = type(props.shadow.color) == 'string' and CoreGui.GetColor(props.shadow.color) or props.shadow.color end
        if props.shadow.color_live ~= nil then 
            self.shadow.color_live = type(props.shadow.color_live) == 'string' and CoreGui.GetColor(props.shadow.color_live) or props.shadow.color_live end
        if props.shadow.color_nonactive ~= nil then 
            self.shadow.color_nonactive = type(props.shadow.color_nonactive) == 'string' and CoreGui.GetColor(props.shadow.color_nonactive) or props.shadow.color_nonactive end
        
        if props.shadow.width ~= nil then self.shadow.width = props.shadow.width end
    end
end

function GuiWindow:ApplyDisable()
    if self.header_rect_mat ~= nil then
        self.header_rect_mat:SetVectorByID(self.border.color_nonactive, 1)
    end
    if self.shadow_mat ~= nil then
        self.shadow_mat:SetVectorByID(self.shadow.color_nonactive, 2)
    end
    self.bg_rect_mat:SetVectorByID(self.background.color_nonactive, 2)
    self.bg_rect_mat:SetVectorByID(self.border.color_nonactive, 3)
end

function GuiWindow:ApplyNone()
    if self.header_rect_mat ~= nil then
        self.header_rect_mat:SetVectorByID(self.border.color, 1)
    end
    if self.shadow_mat ~= nil then
        self.shadow_mat:SetVectorByID(self.shadow.color, 2)
    end
    self.bg_rect_mat:SetVectorByID(self.background.color, 2)
    self.bg_rect_mat:SetVectorByID(self.border.color, 3)
end

function GuiWindow:ApplyFocus()
    if self.header_rect_mat ~= nil then
        self.header_rect_mat:SetVectorByID(self.border.color_live, 1)
    end
    if self.shadow_mat ~= nil then
        self.shadow_mat:SetVectorByID(self.shadow.color_live, 2)
    end
    self.bg_rect_mat:SetVectorByID(self.background.color_live, 2)
    self.bg_rect_mat:SetVectorByID(self.border.color_live, 3)
end

function GuiWindow:UpdateProps()
    if self.entity:IsActivated() and self.entity:IsActivatedBranch() then self:onActivate()
    else self:onDeactivate() end
end

function GuiWindow:onActivate()
    local parent = self.entity:GetParent()
    if parent:is_null() then
        self:ApplyNone()
        return
    end
    
    local focus = parent:GetFocus()
    if focus:is_eq(self.entity) then self:ApplyFocus()
    else self:ApplyNone() end
end

function GuiWindow:onDeactivate()
    self:ApplyDisable()
end

function GuiWindow:onMoveResize(is_move, is_resize)
    if not self._base.onMoveResize(self, is_move, is_resize) then return false end
    
    local rect = self.entity:GetRectAbsolute()
    
    if self.bg_rect_mat ~= nil then
        local r = Rect(0, 0, rect.w, rect.h)
        self.entity:SetRect(self.bg_rect, r)

        if is_resize then
            local shader_data = Vector4(self.border.width / r.w, self.border.width / r.h, 0, 0)
            shader_data.z = 1 - shader_data.x
            shader_data.w = 1 - shader_data.y
            self.bg_rect_mat:SetVectorByID(shader_data, 1)
        end
    end

    if self.header_rect_mat ~= nil then
        local r = Rect(0, 0, rect.w, self.header_size)
        self.entity:SetRect(self.header_rect, r)
    end

    if self.shadow_mat ~= nil then
        local r = Rect(-self.shadow.width, -self.shadow.width, rect.w, rect.h)
        local double_shadow_w = self.shadow.width * 2
        r.w = r.w + double_shadow_w
        r.h = r.h + double_shadow_w
        self.entity:SetRect(self.shadow_rect, r)

        if is_resize then
            local shader_data = Vector4(self.shadow.width / r.w, self.shadow.width / r.h, 0, 0)
            self.shadow_mat:SetVectorByID(shader_data, 1)
        end
    end

    if not is_resize then return end

    if self.scrollable.x then
        self.clientarea_rect.w = rect.w - self.clientarea_rect.l - self.clientarea_rect.r
        if self.scroll_enabled.x then
            if self.clientarea_rect.w >= self.body_ent.entity.width then
                self.scroll_enabled.x = false
                self.scroll_x_sld.entity.enable = false
                self.clientarea_rect.b = self.clientarea_rect.b - self.scroll_x_sld.entity.top - self.scroll_x_sld.entity.height - self.scroll_x_sld.entity.bottom
                self.clientarea_ent.entity.bottom = self.clientarea_rect.b
            else
                local sl_rect = self.scroll_x_sld.entity:GetRectAbsolute()
                self.scroll_x_sld.btn_slider.entity.width = sl_rect.w * self.clientarea_rect.w / self.body_ent.entity.width
                self.scroll_x_sld:SetSlider(self.scroll_x_sld.value)
            end
            self.scroll.x = math.max(0, (self.body_ent.entity.width - self.clientarea_rect.w) * self.scroll_x_sld.value)
        else
            if self.clientarea_rect.w < self.body_ent.entity.width then
                self.scroll_enabled.x = true
                self.scroll_x_sld.entity.enable = true

                local sl_rect = self.scroll_y_sld.entity:GetRectAbsolute()
                self.scroll_x_sld.btn_slider.entity.width = sl_rect.w * self.clientarea_rect.w / self.body_ent.entity.width
               
                self.clientarea_rect.b = self.clientarea_rect.b + self.scroll_x_sld.entity.top + self.scroll_x_sld.entity.height + self.scroll_x_sld.entity.bottom
                self.clientarea_ent.entity.bottom = self.clientarea_rect.b
            end
        end
    end

    if self.scrollable.y then
        self.clientarea_rect.h = rect.h - self.clientarea_rect.t - self.clientarea_rect.b
        if self.scroll_enabled.y then
            if self.clientarea_rect.h >= self.body_ent.entity.height then
                self.scroll_enabled.y = false
                self.scroll_y_sld.entity.enable = false
                self.clientarea_rect.r = self.clientarea_rect.r - self.scroll_y_sld.entity.left - self.scroll_y_sld.entity.width - self.scroll_y_sld.entity.right
                self.clientarea_ent.entity.right = self.clientarea_rect.r
            else
                local sl_rect = self.scroll_y_sld.entity:GetRectAbsolute()
                self.scroll_y_sld.btn_slider.entity.height = sl_rect.h * self.clientarea_rect.h / self.body_ent.entity.height
                self.scroll_y_sld:SetSlider(self.scroll_y_sld.value)
            end
            self.scroll.y = math.max(0, (self.body_ent.entity.height - self.clientarea_rect.h) * self.scroll_y_sld.value)
        else
            if self.clientarea_rect.h < self.body_ent.entity.height then
                self.scroll_enabled.y = true
                self.scroll_y_sld.entity.enable = true

                local sl_rect = self.scroll_y_sld.entity:GetRectAbsolute()
                self.scroll_y_sld.btn_slider.entity.height = sl_rect.h * self.clientarea_rect.h / self.body_ent.entity.height
               
                self.clientarea_rect.r = self.clientarea_rect.r + self.scroll_y_sld.entity.left + self.scroll_y_sld.entity.width + self.scroll_y_sld.entity.right
                self.clientarea_ent.entity.right = self.clientarea_rect.r
            end
        end
    end

    self.body_ent.entity.left = -self.scroll.x
    self.body_ent.entity.top = -self.scroll.y

    return true
end

function GuiWindow:Close()
    self.entity:GetParent():DetachChild(self.entity)
    self.entity:Destroy()

    if self.independent == true then
        self.sys_win:UserClose()
        self.sys_win = nil
    end
end

function GuiWindow:callback(eventData)
    local res = eventData
    
    if not self.entity:IsActivated() or not self.entity:IsActivatedBranch() then
        if eventData.event == GUI_EVENTS.MOUSE_DOWN then
            self.entity:SetHierarchyFocusOnMe(false)
            res.event = GUI_EVENTS.DO_DENIED
        elseif not (eventData.event == GUI_EVENTS.UPDATE or eventData.event == GUI_EVENTS.SYS_MOVE or 
            eventData.event == GUI_EVENTS.SYS_RESIZE or eventData.event == GUI_EVENTS.MOUSE_WHEEL) then
            res.event = GUI_EVENTS.NULL
        end
        res.entity = self.entity
        return self._base.callback(self, res)
    end

    if eventData.event == GUI_EVENTS.MOUSE_DOWN then 
        self.entity:SetHierarchyFocusOnMe(false)
        self.entity:SetFocus(HEntity())
        res.entity = self.entity
        
        if self.dragable and eventData.entity:is_eq(self.entity) and eventData.key == KEYBOARD_CODES.KEY_LBUTTON then
            local corners = self.entity:GetCorners()
            if eventData.coords.x >= corners.l and eventData.coords.x <= corners.r and
                eventData.coords.y >= corners.t and eventData.coords.y <= corners.t + self.header_size then
                self.entity.left_percent = false
                self.entity.top_percent = false
                self.entity.align = GUI_ALIGN.LEFT
                self.entity.valign = GUI_VALIGN.TOP

                self.state_drag = true
                self.drag_offset.x = eventData.coords.x - corners.l
                self.drag_offset.y = eventData.coords.y - corners.t

                self.entity:SetHierarchyFocusOnMe(true)

                res.event = GUI_EVENTS.WIN_START_MOVE
                res.entity = self.entity
            end
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_UP then 
        if eventData.key == KEYBOARD_CODES.KEY_LBUTTON then
            if self.state_drag then
                self.entity:SetHierarchyFocusOnMe(false)
                self.state_drag = false

                res.event = GUI_EVENTS.WIN_STOP_MOVE
                res.entity = self.entity
            end
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_MOVE then 
        if eventData.entity:is_eq(self.entity) and self.state_drag then
            self.entity.left = eventData.coords.x - self.drag_offset.x
            self.entity.top = eventData.coords.y - self.drag_offset.y
            self.entity:UpdatePos()

            res.event = GUI_EVENTS.WIN_MOVING
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.BUTTON_PRESSED then 
        if self.closeable and eventData.entity:is_eq(self.close_btn.entity) then
            self:Close()

            res.event = GUI_EVENTS.WIN_CLOSE
            res.entity = self.entity

        elseif (self.resizeable.x or self.resizeable.y) and eventData.entity:is_eq(self.resize_btn.entity) then
            self.state_resize = true
            self.entity:SetHierarchyFocus(self.resize_btn.entity, true)

            local rect = self.entity:GetRectAbsolute()

            if self.entity.width_percent then
                self.entity.width_percent = false
                self.entity.width = rect.w
            end
            if self.entity.height_percent then
                self.entity.height_percent = false
                self.entity.height = rect.h
            end

            local btn_rect = self.resize_btn.entity:GetRectAbsolute()
            self.drag_offset.x = eventData.coords.x - btn_rect.l
            self.drag_offset.y = eventData.coords.y - btn_rect.t

            res.event = GUI_EVENTS.WIN_START_RESIZE
            res.entity = self.entity
        end
        
    elseif eventData.event == GUI_EVENTS.BUTTON_UNPRESSED then 
        if (self.resizeable.x or self.resizeable.y) and eventData.entity:is_eq(self.resize_btn.entity) then
            self.state_resize = false
            self.entity:SetHierarchyFocus(self.resize_btn.entity, false)

            res.event = GUI_EVENTS.WIN_STOP_RESIZE
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.BUTTON_MOVE then 
        if self.state_resize then
            local btn_rect = self.resize_btn.entity:GetRectAbsolute()
            if self.resizeable.x then 
                if self.clamp_resize.x then
                    local max_size = self.body_ent.entity.width + self.clientarea_rect.l + self.clientarea_rect.r
                    self.entity.width = math.min(max_size, math.max(100, self.entity.width + eventData.coords.x - self.drag_offset.x - btn_rect.l))
                else
                    self.entity.width = math.max(100, self.entity.width + eventData.coords.x - self.drag_offset.x - btn_rect.l)
                end
            end
            if self.resizeable.y then 
                if self.clamp_resize.y then
                    local max_size = self.body_ent.entity.height + self.clientarea_rect.t + self.clientarea_rect.b
                    self.entity.height = math.min(max_size, math.max(100, self.entity.height + eventData.coords.y - self.drag_offset.y - btn_rect.t))
                else
                    self.entity.height = math.max(100, self.entity.height + eventData.coords.y - self.drag_offset.y - btn_rect.t)
                end
            end
            self.entity:UpdateSize()

            res.event = GUI_EVENTS.WIN_RESIZING
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.SLIDER_DRAG or eventData.event == GUI_EVENTS.SLIDER_START_DRAG then
        if self.scrollable.x and eventData.entity:is_eq(self.scroll_x_sld.entity) then
            self.scroll.x = (self.body_ent.entity.width - self.clientarea_rect.w) * self.scroll_x_sld.value
            self.body_ent.entity.left = -self.scroll.x
            self.body_ent.entity:UpdatePos()
            res.event = GUI_EVENTS.WIN_SCROLL
            res.entity = self.entity
        elseif self.scrollable.y and eventData.entity:is_eq(self.scroll_y_sld.entity) then
            self.scroll.y = (self.body_ent.entity.height - self.clientarea_rect.h) * self.scroll_y_sld.value
            self.body_ent.entity.top = -self.scroll.y
            self.body_ent.entity:UpdatePos()
            res.event = GUI_EVENTS.WIN_SCROLL
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_WHEEL then
        if self.scrollable.y and self.scroll_enabled.y then
            self.scroll_y_sld:SetSlider( self.scroll_y_sld.value - eventData.coords.x * (SCROLL_SPEED / self.body_ent.entity.height) )
            self.scroll.y = (self.body_ent.entity.height - self.clientarea_rect.h) * self.scroll_y_sld.value
            self.body_ent.entity.top = -self.scroll.y
            self.body_ent.entity:UpdatePos()

            res.event = GUI_EVENTS.WIN_SCROLL_WHEEL
            res.entity = self.entity
        elseif self.scrollable.x and self.scroll_enabled.x then
            self.scroll_x_sld:SetSlider( self.scroll_x_sld.value - eventData.coords.x * (SCROLL_SPEED / self.body_ent.entity.width) )
            self.scroll.x = (self.body_ent.entity.width - self.clientarea_rect.w) * self.scroll_x_sld.value
            self.body_ent.entity.left = -self.scroll.x
            self.body_ent.entity:UpdatePos()

            res.event = GUI_EVENTS.WIN_SCROLL_WHEEL
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.UNFOCUS then
        if eventData.entity:is_eq(self.entity) then 
            self.entity:SetFocus(HEntity()) 
            self:ApplyNone()
        end

    elseif eventData.event == GUI_EVENTS.MENU_CLOSE then 
        self.entity:SetHierarchyFocusOnMe(false)

    elseif eventData.event == GUI_EVENTS.FOCUS then
        self:ApplyFocus()
    end

    return self._base.callback(self, res)
end

function GuiWindow:GetBody()
    return self.body_ent
end

function GuiWindow:GetClient()
    return self.clientarea_ent
end

function GuiWindow:SetScrollX(x)
    if not self.scrollable.x or not self.scroll_enabled.x then return end

    self.scroll.x = math.min( self.body_ent.entity.width - self.clientarea_rect.w, x )

    self.scroll_x_sld:SetSlider( self.scroll.x / (self.body_ent.entity.width - self.clientarea_rect.w) )
    self.body_ent.entity.left = -self.scroll.x
    self.body_ent.entity:UpdatePos()
end

function GuiWindow:SetScrollY(y)
    if not self.scrollable.y or not self.scroll_enabled.y then return end

    self.scroll.y = math.min( self.body_ent.entity.height - self.clientarea_rect.h, y )

    self.scroll_y_sld:SetSlider( self.scroll.y / (self.body_ent.entity.height - self.clientarea_rect.h) )
    self.body_ent.entity.top = -self.scroll.y
    self.body_ent.entity:UpdatePos()    
end

function GuiWindow:SetHeader(str)
    if self.header_str then
        self.header_str:SetString(str)
    end

    if self.independent == true then
        self.sys_win.caption_text = str
    end
end