local COMBO_BG_MAT = "../resources/shaders/gui/rect"

if not GuiCombo then GuiCombo = class(GuiEntity) end

function GuiCombo:init(props)
    self.cursor = SYSTEM_CURSORS.ARROW
    self.alt = ""

    self.fadein_time = 0
    self.fadeout_time = 0

    self.none_str = ""
    self.allow_none = false

    self.str_height = 20

    self.list_h_offset = 0
    
    self.icon = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
        color_press = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),

        background = {
            color = Vector4(0.0, 0.0, 0.0, 1.0),
            color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
            color_press = Vector4(0.0, 0.0, 0.0, 1.0),
            color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0)
        },

        material = {}
    }

    self.background = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
        color_press = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0)
    }

    self.border = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
        color_press = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        width = 0
    }

    self.text = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
        color_press = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        offset = { x = 0, y = 0 },
        center = { x = true, y = true },
        font = "",
        length = 64
    }
    
    -- replace self:base(GuiCombo).init(self, props)
    self.entity = HEntity()
    self.entity:Create()
    
    self:ApplyProps(props)
    self:ApplyCallbacks(props)

    local id = ""
    if props.id then id = props.id end

    self.entity:Init(id, self)
    
    -- child process
    self.list = {}
    if props.list ~= nil then
        for ch = 1, #props.list do
            self.list[ch] = props.list[ch]
        end
    end

    if #self.list == 0 then 
        error("Pointless (no list options) GuiCombo named "..id)
        return
    end
    ---------
    
    self.menu_props = {
        styles = {GuiStyles.menu_default},
        adapt_horz = false,
        adapt_vert = true,
        offset = {x = 0, y = 0},
        --background = {color = self.background.color_press},
        background = {color = Vector4(0,0,0,0)},
        border = {
            color = self.border.color_press,
            width = self.border.width
        },
        shadow = {width = 0},
        id = "combo_menu",
        width = 0,
        height = #self.list * self.str_height + self.border.width * 2, 
    }

    self.list_props = {
        styles = {GuiStyles.combo_button},

        align = GUI_ALIGN.BOTH,
        valign = GUI_VALIGN.TOP,
        right = self.border.width,
        left = self.border.width,
        height = self.str_height,
        top = 0,

        background = {
            color = self.background.color_press,
            color_hover = self.background.color_hover,
            color_press = self.background.color_hover,
            color_nonactive = self.background.color_nonactive
        },

        text = {
            offset = { x = self.text.offset.x, y = 0 },
            center = { x = self.text.center.x, y = true },
            font = self.text.font,
            color = self.text.color_press,
            color_hover = self.text.color_hover,
            color_press = self.text.color_hover,
            color_nonactive = self.text.color_nonactive,
            str = ""
        },

        id = ""
    }

    local is_bg = self.background.color.w + self.background.color_nonactive.w + 
        self.background.color_hover.w + self.background.color_press.w
    local is_border = self.border.color.w + self.border.color_nonactive.w + 
        self.border.color_hover.w + self.border.color_press.w
    is_border = is_border * self.border.width
    if is_border > 0 or is_bg > 0 then
        self.rect = self.entity:AddRect(COMBO_BG_MAT)
        self.rect_mat = self:SetRectMaterial(self.rect)
    end

    if self.icon.material.shader then
        self.iconrect = self.entity:AddRect(self.icon.material.shader)
        self.icon_mat = self:SetRectMaterial(self.iconrect, self.icon.material)
    end
    
    if self.text.font:len() ~= 0 then
        if self.allow_none then
            self.str = self.entity:AddText(self.text.font, self.none_str, "", false, self.text.length)
            self.selected_id = 0
        else
            self.str = self.entity:AddText(self.text.font, self.list[1], "", false, self.text.length)
            self.selected_id = 1
        end
    else
        error("No font in GuiCombo named "..id)
        return
    end
    
    self.state_hover = false
    self.state_press = false

    self.anim_progress = 0
    self.anim_go = 0

    self.just_closed = false

    self:UpdateProps()
end

function GuiCombo:ApplyDisable()
    if self.rect_mat ~= nil then
        self.rect_mat:SetVectorByID(self.background.color_nonactive, 2)
        self.rect_mat:SetVectorByID(self.border.color_nonactive, 3)
    end
    if self.icon_mat ~= nil then
        self.icon_mat:SetVectorByID(self.icon.color_nonactive, 2)
        self.icon_mat:SetVectorByID(self.icon.background.color_nonactive, 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color_nonactive)
    end
end

function GuiCombo:ApplyNone()
    if self.rect_mat ~= nil then
        self.rect_mat:SetVectorByID(self.background.color, 2)
        self.rect_mat:SetVectorByID(self.border.color, 3)
    end
    if self.icon_mat ~= nil then
        self.icon_mat:SetVectorByID(self.icon.color, 2)
        self.icon_mat:SetVectorByID(self.icon.background.color, 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color)
    end
end

function GuiCombo:ApplyPress()
    if self.rect_mat ~= nil then
        self.rect_mat:SetVectorByID(self.background.color_press, 2)
        self.rect_mat:SetVectorByID(self.border.color_press, 3)
    end
    if self.icon_mat ~= nil then
        self.icon_mat:SetVectorByID(self.icon.color_press, 2)
        self.icon_mat:SetVectorByID(self.icon.background.color_press, 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color_press)
    end
end

function GuiCombo:ApplyHover()
    if self.rect_mat ~= nil then
        self.rect_mat:SetVectorByID(self.background.color_hover, 2)
        self.rect_mat:SetVectorByID(self.border.color_hover, 3)
    end
    if self.icon_mat ~= nil then
        self.icon_mat:SetVectorByID(self.icon.color_hover, 2)
        self.icon_mat:SetVectorByID(self.icon.background.color_hover, 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color_hover)
    end
end

function GuiCombo:ApplyProps(props)
    self._base.ApplyProps(self, props)

    if props.cursor ~= nil then self.cursor = props.cursor end
    if props.alt ~= nil then self.alt = props.alt end
    if props.fadein_time ~= nil then self.fadein_time = props.fadein_time end
    if props.fadeout_time ~= nil then self.fadeout_time = props.fadeout_time end

    if props.none_str ~= nil then self.none_str = props.none_str end
    if props.allow_none ~= nil then self.allow_none = props.allow_none end
    if props.str_height ~= nil then self.str_height = props.str_height end
    if props.list_h_offset ~= nil then self.list_h_offset = props.list_h_offset end

    if props.icon ~= nil then
        if props.icon.material ~= nil then self.icon.material = props.icon.material end

        if props.icon.color ~= nil then 
            self.icon.color = type(props.icon.color) == 'string' and CoreGui.GetColor(props.icon.color) or props.icon.color end
        if props.icon.color_hover ~= nil then 
            self.icon.color_hover = type(props.icon.color_hover) == 'string' and CoreGui.GetColor(props.icon.color_hover) or props.icon.color_hover end
        if props.icon.color_press ~= nil then 
            self.icon.color_press = type(props.icon.color_press) == 'string' and CoreGui.GetColor(props.icon.color_press) or props.icon.color_press end
        if props.icon.color_nonactive ~= nil then 
            self.icon.color_nonactive = type(props.icon.color_nonactive) == 'string' and CoreGui.GetColor(props.icon.color_nonactive) or props.icon.color_nonactive end
    
        if props.icon.background ~= nil then
            if props.icon.background.color ~= nil then 
                self.icon.background.color = type(props.icon.background.color) == 'string' and CoreGui.GetColor(props.icon.background.color) or props.icon.background.color end
            if props.icon.background.color_hover ~= nil then 
                self.icon.background.color_hover = type(props.icon.background.color_hover) == 'string' and CoreGui.GetColor(props.icon.background.color_hover) or props.icon.background.color_hover end
            if props.icon.background.color_press ~= nil then 
                self.icon.background.color_press = type(props.icon.background.color_press) == 'string' and CoreGui.GetColor(props.icon.background.color_press) or props.icon.background.color_press end
            if props.icon.background.color_nonactive ~= nil then 
                self.icon.background.color_nonactive = type(props.icon.background.color_nonactive) == 'string' and CoreGui.GetColor(props.icon.background.color_nonactive) or props.icon.background.color_nonactive end
        end
    end

    if props.background ~= nil then
        if props.background.color ~= nil then 
            self.background.color = type(props.background.color) == 'string' and CoreGui.GetColor(props.background.color) or props.background.color end
        if props.background.color_hover ~= nil then 
            self.background.color_hover = type(props.background.color_hover) == 'string' and CoreGui.GetColor(props.background.color_hover) or props.background.color_hover end
        if props.background.color_press ~= nil then 
            self.background.color_press = type(props.background.color_press) == 'string' and CoreGui.GetColor(props.background.color_press) or props.background.color_press end
        if props.background.color_nonactive ~= nil then 
            self.background.color_nonactive = type(props.background.color_nonactive) == 'string' and CoreGui.GetColor(props.background.color_nonactive) or props.background.color_nonactive end
    end

    if props.border ~= nil then
        if props.border.color ~= nil then 
            self.border.color = type(props.border.color) == 'string' and CoreGui.GetColor(props.border.color) or props.border.color end
        if props.border.color_hover ~= nil then 
            self.border.color_hover = type(props.border.color_hover) == 'string' and CoreGui.GetColor(props.border.color_hover) or props.border.color_hover end
        if props.border.color_press ~= nil then 
            self.border.color_press = type(props.border.color_press) == 'string' and CoreGui.GetColor(props.border.color_press) or props.border.color_press end
        if props.border.color_nonactive ~= nil then 
            self.border.color_nonactive = type(props.border.color_nonactive) == 'string' and CoreGui.GetColor(props.border.color_nonactive) or props.border.color_nonactive end
        
        if props.border.width ~= nil then self.border.width = props.border.width end
    end

    if props.text ~= nil then
        if props.text.font ~= nil then self.text.font = props.text.font end
        if props.text.offset ~= nil then
            if props.text.offset.x ~= nil then self.text.offset.x = props.text.offset.x end
            if props.text.offset.y ~= nil then self.text.offset.y = props.text.offset.y end
        end
        if props.text.center ~= nil then
            if props.text.center.x ~= nil then self.text.center.x = props.text.center.x end
            if props.text.center.y ~= nil then self.text.center.y = props.text.center.y end
        end

        if props.text.color ~= nil then 
            self.text.color = type(props.text.color) == 'string' and CoreGui.GetColor(props.text.color) or props.text.color end
        if props.text.color_hover ~= nil then 
            self.text.color_hover = type(props.text.color_hover) == 'string' and CoreGui.GetColor(props.text.color_hover) or props.text.color_hover end
        if props.text.color_press ~= nil then 
            self.text.color_press = type(props.text.color_press) == 'string' and CoreGui.GetColor(props.text.color_press) or props.text.color_press end
        if props.text.color_nonactive ~= nil then 
            self.text.color_nonactive = type(props.text.color_nonactive) == 'string' and CoreGui.GetColor(props.text.color_nonactive) or props.text.color_nonactive end
    end
end

function GuiCombo:UpdateProps()
    if self.entity:IsActivated() and self.entity:IsActivatedBranch() then self:onActivate()
    else self:onDeactivate() end
end

function GuiCombo:onMoveResize(is_move, is_resize)
    if not self._base.onMoveResize(self, is_move, is_resize) then return false end

    local rect = self.entity:GetRectAbsolute()
    
    if self.rect_mat ~= nil then
        local r = Rect(0, 0, rect.w, rect.h)
        self.entity:SetRect(self.rect, r)

        if is_resize then
            local shader_data = Vector4(self.border.width / r.w, self.border.width / r.h, 0, 0)
            shader_data.z = 1 - shader_data.x
            shader_data.w = 1 - shader_data.y
            self.rect_mat:SetVectorByID(shader_data, 1)
        end
    end

    local double_border = self.border.width * 2

    if self.icon_mat ~= nil then
        local icon_size = rect.h - double_border
        local r = Rect(rect.w - (rect.h - self.border.width), self.border.width, icon_size, icon_size)
        self.entity:SetRect(self.iconrect, r)
    end
    
    if self.str ~= nil then
        if is_resize then
            local text_bounds = self.entity:GetTextBounds(self.str)
            if self.text.center.x then self.text.offset.x = (rect.w - text_bounds.w) / 2 end
            if self.text.center.y then self.text.offset.y = (rect.h - text_bounds.h) / 2 end
        end
        self.entity:SetTextPos(self.str, self.text.offset.x, self.text.offset.y)
    end

    if self.menu_props then
        self.menu_props.width = rect.w
        self.menu_props.offset.y = rect.h - double_border
    end

    return true
end

function GuiCombo:OpenCombo()
    local topOffset = self.border.width
    for i = 1, #self.list do
        self.list_props.text.str = self.list[i]
        self.list_props.id = tostring(i)
        self.list_props.top = topOffset
        self.menu_props[i] = GuiButton(self.list_props)
        topOffset = topOffset + self.str_height
    end

    self.combo = GuiMenu(self.menu_props)
    self:AttachOverlay(self.combo)
    local coners = self.entity:GetCorners()
    self.combo:Open(coners.l, coners.b - self.border.width + self.list_h_offset)
end

function GuiCombo:OnCloseCombo()
    for i = 1, #self.menu_props do
        self.menu_props[i] = nil
    end
end

function GuiCombo:callback(eventData)
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
        res.entity = self.entity
        self.entity:SetHierarchyFocusOnMe(false)
        
        if eventData.key == KEYBOARD_CODES.KEY_LBUTTON then
            if not self.just_closed then
                self:ApplyPress()
                self.anim_go = 0
                self.anim_progress = 0

                self:OpenCombo()
                
                self.entity:GetRoot():GetInherited():HideAlt()
                self.state_press = true
                res.event = GUI_EVENTS.COMBO_OPEN
            else
                self.just_closed = false
                res.event = GUI_EVENTS.NULL
            end
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_HOVER then 
        res.entity = self.entity
        if not self.state_hover then 
            if not self.state_press then
                self.anim_go = 1
            end
            self.state_hover = true
            self.entity:GetRoot():GetInherited():ShowAlt(self.alt)
            res.event = GUI_EVENTS.COMBO_HOVER
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_OUT then 
        res.entity = self.entity
        if self.state_hover then
            if not self.state_press then
                self.anim_go = -1
            end
            self.state_hover = false
            self.entity:GetRoot():GetInherited():HideAlt()
            res.event = GUI_EVENTS.COMBO_OUT
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_WHEEL then 
        if self.state_press then 
            self.wheel_close = true 
        end

    elseif eventData.event == GUI_EVENTS.MENU_CLOSE then 
        res.entity = self.entity
        self.entity:SetHierarchyFocusOnMe(false)
        
        local coords = CoreGui.GetCursorPos()
        if self.entity:IsCollide(coords.x, coords.y) then
            self:ApplyHover()
            self.anim_go = 0
            self.anim_progress = 1
            self.state_hover = true
            
            if eventData.key ~= KEYBOARD_CODES.KEY_ESCAPE and self.wheel_close ~= true then
                self.just_closed = true
            end
            self.wheel_close = false
        else
            self:ApplyNone()
            self.anim_go = 0
            self.anim_progress = 0
            self.state_hover = false

            self.just_closed = false
            self.wheel_close = false
        end

        self:OnCloseCombo()
        
        self.state_press = false
        res.event = GUI_EVENTS.COMBO_CLOSE

    elseif eventData.event == GUI_EVENTS.MENU_CLICK then 
        self:ApplyNone()
        self.anim_go = 0
        self.anim_progress = 0
        self.state_hover = false
        self.state_press = false

        self:OnCloseCombo()

        local list_id = tonumber(eventData.entity:GetID())
        if list_id > 0 and list_id <= #self.list then
            self.entity:SetText(self.str, self.list[list_id])
            self.selected_id = list_id
            
            res.entity = self.entity
            res.event = GUI_EVENTS.COMBO_SELECT
        end

    elseif eventData.event == GUI_EVENTS.KEY_DOWN then 
        if eventData.key == KEYBOARD_CODES.KEY_DOWN then
            self.selected_id = self.selected_id + 1
            if self.selected_id > #self.list then self.selected_id = 1 end
            self.entity:SetText(self.str, self.list[self.selected_id])
            res.entity = self.entity
            res.event = GUI_EVENTS.COMBO_SELECT
        elseif eventData.key == KEYBOARD_CODES.KEY_UP then
            self.selected_id = self.selected_id - 1
            if self.selected_id < 1 then self.selected_id = #self.list end
            self.entity:SetText(self.str, self.list[self.selected_id])
            res.entity = self.entity
            res.event = GUI_EVENTS.COMBO_SELECT
        end   

    end

    return self._base.callback(self, res)
end

function GuiCombo:onActivate()
    if self.state_press then self:ApplyPress()
    else self:ApplyNone() end
    self.anim_progress = 0
    self.anim_go = 0
end

function GuiCombo:onDeactivate()
    self:ApplyDisable()
    self.anim_progress = 0
    self.anim_go = 0
end

function GuiCombo:onTick(dt)
    if self.anim_go ~= 0 then
        local fade_time = self.anim_go > 0 and self.fadein_time or self.fadeout_time
        self.anim_progress = self.anim_progress + self.anim_go * dt / fade_time
        self.anim_progress = math.max(0, math.min(self.anim_progress, 1))

        local color_icon = Vector4.Lerp(self.icon.color, self.icon.color_hover, self.anim_progress)
        local color_icon_bg = Vector4.Lerp(self.icon.background.color, self.icon.background.color_hover, self.anim_progress)
        local color_background = Vector4.Lerp(self.background.color, self.background.color_hover, self.anim_progress)
        local color_border = Vector4.Lerp(self.border.color, self.border.color_hover, self.anim_progress)
        local color_text = Vector4.Lerp(self.text.color, self.text.color_hover, self.anim_progress)
        
        if self.rect_mat ~= nil then
            self.rect_mat:SetVectorByID(color_background, 2)
            self.rect_mat:SetVectorByID(color_border, 3)
        end
        if self.icon_mat ~= nil then
            self.icon_mat:SetVectorByID(color_icon, 2)
            self.icon_mat:SetVectorByID(color_icon_bg, 1)
        end
        if self.str ~= nil then
            self.entity:SetTextColor(self.str, color_text)
        end

        if self.anim_progress == 0 or self.anim_progress == 1 then self.anim_go = 0 end
    end
end

function GuiCombo:SetSelected(list_id)
    if self.allow_none and (list_id == 0 or list_id == nil) then
        self.entity:SetText(self.str, self.none_str)
        self.selected_id = 0
    else
        if list_id < 1 or list_id > #self.list then return end
        self.entity:SetText(self.str, self.list[list_id])
        self.selected_id = list_id
    end
end

function GuiCombo:GetSelected()
    return self.selected_id
end