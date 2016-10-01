local BUTTON_BG_MAT = "../resources/shaders/gui/rect"

if not GuiButton then GuiButton = class(GuiEntity) end

-- public
function GuiButton:init(props)
    self.holded = false

    self.hover_only = false
    
    self.cursor = SYSTEM_CURSORS.ARROW
    self.alt = ""

    self.fadein_time = 0
    self.fadeout_time = 0
    
    self.icon = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
        color_press = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        rect = { l = 0, t = 0, w = 0, h = 0 },
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
        str = "",
        font = ""
    }
    
    self._base.init(self, props)
    
    local is_bg = self.background.color.w + self.background.color_nonactive.w + 
        self.background.color_hover.w + self.background.color_press.w
    local is_border = self.border.color.w + self.border.color_nonactive.w + 
        self.border.color_hover.w + self.border.color_press.w
    is_border = is_border * self.border.width
    
    if is_border > 0 or is_bg > 0 then
        self.rect = self.entity:AddRect(BUTTON_BG_MAT)
        self.rect_mat = self:SetRectMaterial(self.rect)
    end
    
    if self.icon.material.shader then
        self.iconrect = self.entity:AddRect(self.icon.material.shader)
        self.icon_mat = self:SetRectMaterial(self.iconrect, self.icon.material)
    end
    
    if self.text.font:len() ~= 0 then
        self.str = self.entity:AddText(self.text.font, self.text.str, "", true, 0)
    end
    
    self.state_hover = false
    self.state_press = false
    self.state_mouseover = false

    self.anim_progress = 0
    self.anim_go = 0
    
    self:UpdateProps()
end

function GuiButton:ApplyDisable()
    if self.rect_mat ~= nil then
        self.rect_mat:SetVector(self.background.color_nonactive, 2)
        self.rect_mat:SetVector(self.border.color_nonactive, 3)
    end
    if self.icon_mat ~= nil then
        self.icon_mat:SetVector(self.icon.color_nonactive, 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color_nonactive)
    end
end

function GuiButton:ApplyNone()
    if self.rect_mat ~= nil then
        self.rect_mat:SetVector(self.background.color, 2)
        self.rect_mat:SetVector(self.border.color, 3)
    end
    if self.icon_mat ~= nil then
        self.icon_mat:SetVector(self.icon.color, 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color)
    end
end

function GuiButton:ApplyPress()
    if self.rect_mat ~= nil then
        self.rect_mat:SetVector(self.background.color_press, 2)
        self.rect_mat:SetVector(self.border.color_press, 3)
    end
    if self.icon_mat ~= nil then
        self.icon_mat:SetVector(self.icon.color_press, 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color_press)
    end
end

function GuiButton:ApplyHover()
    if self.rect_mat ~= nil then
        self.rect_mat:SetVector(self.background.color_hover, 2)
        self.rect_mat:SetVector(self.border.color_hover, 3)
    end
    if self.icon_mat ~= nil then
        self.icon_mat:SetVector(self.icon.color_hover, 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color_hover)
    end
end

function GuiButton:ApplyProps(props)
    self._base.ApplyProps(self, props)

    if props.holded ~= nil then self.holded = props.holded end
    if props.hover_only ~= nil then self.hover_only = props.hover_only end
    if props.cursor ~= nil then self.cursor = props.cursor end
    if props.alt ~= nil then self.alt = props.alt end
    if props.fadein_time ~= nil then self.fadein_time = props.fadein_time end
    if props.fadeout_time ~= nil then self.fadeout_time = props.fadeout_time end

    if props.icon ~= nil then
        if props.icon.material ~= nil then self.icon.material = props.icon.material end
        if props.icon.rect ~= nil then 
            if props.icon.rect.l ~= nil then self.icon.rect.l = props.icon.rect.l end
            if props.icon.rect.t ~= nil then self.icon.rect.t = props.icon.rect.t end
            if props.icon.rect.w ~= nil then self.icon.rect.w = props.icon.rect.w end
            if props.icon.rect.h ~= nil then self.icon.rect.h = props.icon.rect.h end
        end

        if props.icon.color ~= nil then 
            self.icon.color = type(props.icon.color) == 'string' and CoreGui.GetColor(props.icon.color) or props.icon.color end
        if props.icon.color_hover ~= nil then 
            self.icon.color_hover = type(props.icon.color_hover) == 'string' and CoreGui.GetColor(props.icon.color_hover) or props.icon.color_hover end
        if props.icon.color_press ~= nil then 
            self.icon.color_press = type(props.icon.color_press) == 'string' and CoreGui.GetColor(props.icon.color_press) or props.icon.color_press end
        if props.icon.color_nonactive ~= nil then 
            self.icon.color_nonactive = type(props.icon.color_nonactive) == 'string' and CoreGui.GetColor(props.icon.color_nonactive) or props.icon.color_nonactive end
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
        if props.text.str ~= nil then self.text.str = props.text.str end
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

function GuiButton:UpdateProps()
    if self.entity:IsActivated() and self.entity:IsActivatedBranch() then self:onActivate()
    else self:onDeactivate() end
end

function GuiButton:onMoveResize(is_move, is_resize)
    if not self._base.onMoveResize(self, is_move, is_resize) then return false end

    local rect = self.entity:GetRectAbsolute()
    
    if self.rect_mat ~= nil then
        local r = Rect(0, 0, rect.w, rect.h)
        self.entity:SetRect(self.rect, r)

        if is_resize then
            local shader_data = Vector4(self.border.width / r.w, self.border.width / r.h, 0, 0)
            shader_data.z = 1 - shader_data.x
            shader_data.w = 1 - shader_data.y
            self.rect_mat:SetVector(shader_data, 1)
        end
    end

    if self.icon_mat ~= nil then
        local r = Rect(self.icon.rect.l, self.icon.rect.t, self.icon.rect.w, self.icon.rect.h)
        r.w = r.w < 0 and rect.w or r.w
        r.h = r.h < 0 and rect.h or r.h
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

    return true
end

function GuiButton:callback(eventData)
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
        if self.hover_only then 
            return self._base.callback(self, res)
        end
        
        self.entity:SetHierarchyFocusOnMe(false)

        if eventData.key == KEYBOARD_CODES.KEY_LBUTTON then
            res.entity = self.entity
            
            if self.holded and self.state_press then
                -- set hover color
                self:ApplyHover()
                self.anim_go = 0 
                self.anim_progress = 1

                self.state_hover = true
                self.state_press = false
                res.event = GUI_EVENTS.BUTTON_UNPRESSED
            else
                -- set pressed color
                self:ApplyPress()
                self.anim_go = 0
                self.anim_progress = 0

                self.entity:GetRoot():GetInherited():HideAlt()
                self.state_press = true
                res.event = GUI_EVENTS.BUTTON_PRESSED
            end
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_UP then 
        res.entity = self.entity
        if self.hover_only then 
            return self._base.callback(self, res)
        end

        if eventData.key == KEYBOARD_CODES.KEY_LBUTTON and not self.holded and self.state_press then
            -- set hover color
            self:ApplyHover()
            self.anim_go = 0
            self.anim_progress = 1

            self.state_hover = true
            self.state_press = false
            res.event = GUI_EVENTS.BUTTON_UNPRESSED
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_HOVER then
        res.entity = self.entity
        if self.cursor ~= SYSTEM_CURSORS.NONE then CoreGui.SetHCursor(self.cursor) end
        res.event = GUI_EVENTS.BUTTON_HOVER
        if not self.state_hover then 
            if not self.state_press then
                self.anim_go = 1
            end
            self.state_hover = true
            self.entity:GetRoot():GetInherited():ShowAlt(self.alt)
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_OUT then 
        res.entity = self.entity
        if self.cursor ~= SYSTEM_CURSORS.NONE then CoreGui.SetHCursor(SYSTEM_CURSORS.ARROW) end
        if self.state_hover then
            if not self.state_press then
                self.anim_go = -1
            end
            self.state_hover = false
            self.entity:GetRoot():GetInherited():HideAlt()
            res.event = GUI_EVENTS.BUTTON_OUT
        end

        if self.state_press and not self.holded then
            self.state_press = false
            self:ApplyNone()
            self.anim_go = 0
            self.anim_progress = 0
            res.event = GUI_EVENTS.BUTTON_UNPRESSED
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_MOVE then 
        res.entity = self.entity
        if self.cursor ~= SYSTEM_CURSORS.NONE then CoreGui.SetHCursor(self.cursor) end
        res.event = GUI_EVENTS.BUTTON_MOVE

    end

    return self._base.callback(self, res)
end

function GuiButton:onActivate()
    if self.state_press then self:ApplyPress()
    else self:ApplyNone() end
    self.anim_progress = 0
    self.anim_go = 0
end

function GuiButton:onDeactivate()
    self:ApplyDisable()
    self.anim_progress = 0
    self.anim_go = 0
end

function GuiButton:onTick(dt)
    if self.anim_go ~= 0 then
        local fade_time = self.anim_go > 0 and self.fadein_time or self.fadeout_time
        self.anim_progress = self.anim_progress + self.anim_go * dt / fade_time
        self.anim_progress = math.max(0, math.min(self.anim_progress, 1))

        local color_icon = Vector4Lerp(self.icon.color, self.icon.color_hover, self.anim_progress)
        local color_background = Vector4Lerp(self.background.color, self.background.color_hover, self.anim_progress)
        local color_border = Vector4Lerp(self.border.color, self.border.color_hover, self.anim_progress)
        local color_text = Vector4Lerp(self.text.color, self.text.color_hover, self.anim_progress)
        
        if self.rect_mat ~= nil then
            self.rect_mat:SetVector(color_background, 2)
            self.rect_mat:SetVector(color_border, 3)
        end
        if self.icon_mat ~= nil then
            self.icon_mat:SetVector(color_icon, 1)
        end
        if self.str ~= nil then
            self.entity:SetTextColor(self.str, color_text)
        end

        if self.anim_progress == 0 or self.anim_progress == 1 then self.anim_go = 0 end
    end
end

function GuiButton:SetPressed(pressed)
    --if not self.holded then return end

    if pressed then
        self:ApplyPress()
        self.anim_go = 0
        self.anim_progress = 0
        self.state_press = true
    else
        self:ApplyNone()
        self.anim_go = 0
        self.anim_progress = 0
        self.state_press = false
    end
end