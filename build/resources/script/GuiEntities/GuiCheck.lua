local BOX_MAT = "../resources/shaders/gui/rect"
local CHECK_MAT = "../resources/shaders/gui/color"

if not GuiCheck then GuiCheck = class(GuiEntity) end

-- public
function GuiCheck:init(props)
    self.alt = ""

    self.fadein_time = 0
    self.fadeout_time = 0
    
    self.box = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0)
    }

    self.check = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        padding = { x = 0, y = 0 }
    }

    self.border = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        width = 0
    }

    self.text = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_hover = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        offset = { x = 0, y = 0 },
        center = true,
        str = "",
        font = ""
    }
    
    self._base.init(self, props)
    
    self.group_id = 0

    self.boxrect = self.entity:AddRect(BOX_MAT)
    self.boxrect_mat = self:SetRectMaterial(self.boxrect)

    self.checkrect = self.entity:AddRect(CHECK_MAT)
    self.checkrect_mat = self:SetRectMaterial(self.checkrect)
    
    if self.text.font:len() ~= 0 then
        self.str = self.entity:AddText(self.text.font, self.text.str, "", true, 0)
    end
    
    self.state_hover = false
    self.state_check = false

    self.anim_progress = 0
    self.anim_go = 0

    self.undef = false
    
    self:UpdateProps()
end

function GuiCheck:ApplyDisable()
    if self.boxrect_mat ~= nil then
        self.boxrect_mat:SetVectorByID(self.box.color_nonactive, 2)
        self.boxrect_mat:SetVectorByID(self.border.color_nonactive, 3)
    end
    if self.checkrect_mat ~= nil then
        self.checkrect_mat:SetVectorByID(self.undef and self.check.color_nonactive or (self.state_check and self.check.color_nonactive or self.box.color_nonactive), 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color_nonactive)
    end
end

function GuiCheck:ApplyNone()
    if self.boxrect_mat ~= nil then
        self.boxrect_mat:SetVectorByID(self.box.color, 2)
        self.boxrect_mat:SetVectorByID(self.border.color, 3)
    end
    if self.checkrect_mat ~= nil then
        self.checkrect_mat:SetVectorByID(self.undef and self.check.color_nonactive or (self.state_check and self.check.color or self.box.color), 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color)
    end
end

function GuiCheck:ApplyHover()
    if self.boxrect_mat ~= nil then
        self.boxrect_mat:SetVectorByID(self.box.color_hover, 2)
        self.boxrect_mat:SetVectorByID(self.border.color_hover, 3)
    end
    if self.checkrect_mat ~= nil then
        self.checkrect_mat:SetVectorByID(self.undef and self.check.color_nonactive or (self.state_check and self.check.color_hover or self.box.color_hover), 1)
    end
    if self.str ~= nil then
        self.entity:SetTextColor(self.str, self.text.color_hover)
    end
end

function GuiCheck:ApplyProps(props)
    self._base.ApplyProps(self, props)

    if props.alt ~= nil then self.alt = props.alt end
    if props.fadein_time ~= nil then self.fadein_time = props.fadein_time end
    if props.fadeout_time ~= nil then self.fadeout_time = props.fadeout_time end

    if props.check ~= nil then
        if props.check.color ~= nil then 
            self.check.color = type(props.check.color) == 'string' and CoreGui.GetColor(props.check.color) or props.check.color end
        if props.check.color_hover ~= nil then 
            self.check.color_hover = type(props.check.color_hover) == 'string' and CoreGui.GetColor(props.check.color_hover) or props.check.color_hover end
        if props.check.color_nonactive ~= nil then 
            self.check.color_nonactive = type(props.check.color_nonactive) == 'string' and CoreGui.GetColor(props.check.color_nonactive) or props.check.color_nonactive end
        if props.check.padding ~= nil then
            if props.check.padding.x ~= nil then self.check.padding.x = props.check.padding.x end
            if props.check.padding.y ~= nil then self.check.padding.y = props.check.padding.y end
        end
    end

    if props.box ~= nil then
        if props.box.color ~= nil then 
            self.box.color = type(props.box.color) == 'string' and CoreGui.GetColor(props.box.color) or props.box.color end
        if props.box.color_hover ~= nil then 
            self.box.color_hover = type(props.box.color_hover) == 'string' and CoreGui.GetColor(props.box.color_hover) or props.box.color_hover end
        if props.box.color_nonactive ~= nil then 
            self.box.color_nonactive = type(props.box.color_nonactive) == 'string' and CoreGui.GetColor(props.box.color_nonactive) or props.box.color_nonactive end
    end

    if props.border ~= nil then
        if props.border.color ~= nil then 
            self.border.color = type(props.border.color) == 'string' and CoreGui.GetColor(props.border.color) or props.border.color end
        if props.border.color_hover ~= nil then 
            self.border.color_hover = type(props.border.color_hover) == 'string' and CoreGui.GetColor(props.border.color_hover) or props.border.color_hover end
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
        if props.text.center ~= nil then self.text.center = props.text.center end

        if props.text.color ~= nil then 
            self.text.color = type(props.text.color) == 'string' and CoreGui.GetColor(props.text.color) or props.text.color end
        if props.text.color_hover ~= nil then 
            self.text.color_hover = type(props.text.color_hover) == 'string' and CoreGui.GetColor(props.text.color_hover) or props.text.color_hover end
        if props.text.color_nonactive ~= nil then 
            self.text.color_nonactive = type(props.text.color_nonactive) == 'string' and CoreGui.GetColor(props.text.color_nonactive) or props.text.color_nonactive end
    end
end

function GuiCheck:UpdateProps()
    if self.entity:IsActivated() and self.entity:IsActivatedBranch() then self:onActivate()
    else self:onDeactivate() end
end

function GuiCheck:onMoveResize(is_move, is_resize)
    if not self._base.onMoveResize(self, is_move, is_resize) then return false end

    local rect = self.entity:GetRectAbsolute()
    local r = Rect(0, 0, rect.h, rect.h)
    
    if self.boxrect_mat ~= nil then
        self.entity:SetRect(self.boxrect, r)

        local shader_data = Vector4(self.border.width / r.w, self.border.width / r.h, 0, 0)
        shader_data.z = 1 - shader_data.x
        shader_data.w = 1 - shader_data.y
        self.boxrect_mat:SetVectorByID(shader_data, 1)
    end

    if self.checkrect_mat ~= nil then
        r.l = r.l + self.check.padding.x
        r.w = r.w - self.check.padding.x * 2
        r.t = r.t + self.check.padding.y
        r.h = r.h - self.check.padding.y * 2
        self.entity:SetRect(self.checkrect, r)
    end
    
    if self.str ~= nil then
        local text_bounds = self.entity:GetTextBounds(self.str)
        if self.text.center then self.text.offset.y = (rect.h - text_bounds.h) / 2 end
        self.entity:SetTextPos(self.str, rect.h + self.text.offset.x, self.text.offset.y)
    end

    return true
end

function GuiCheck:callback(eventData)
    local res = eventData
    res.entity = self.entity
    
    if not self.entity:IsActivated() or not self.entity:IsActivatedBranch() then
        if eventData.event == GUI_EVENTS.MOUSE_DOWN then
            self.entity:SetHierarchyFocusOnMe(false)
            res.event = GUI_EVENTS.DO_DENIED
            res.coords.x = self.group_id
            res.coords.y = 0
        elseif not (eventData.event == GUI_EVENTS.UPDATE or eventData.event == GUI_EVENTS.SYS_MOVE or 
            eventData.event == GUI_EVENTS.SYS_RESIZE or eventData.event == GUI_EVENTS.MOUSE_WHEEL) then
            res.event = GUI_EVENTS.NULL
        end
        return self._base.callback(self, res)
    end

    if eventData.event == GUI_EVENTS.MOUSE_DOWN then 
        self.entity:SetHierarchyFocusOnMe(false)

        if eventData.key == KEYBOARD_CODES.KEY_LBUTTON then
            self.undef = false
            if self.state_check then
                self.state_check = false
                self:ApplyHover()
                res.event = GUI_EVENTS.CB_UNCHECKED
                res.coords.x = self.group_id
                res.coords.y = 0
            else
                self.state_check = true
                self:ApplyHover()
                res.event = GUI_EVENTS.CB_CHECKED
                res.coords.x = self.group_id
                res.coords.y = 0
            end
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_HOVER then
        if not self.state_hover then 
            self.anim_go = 1
            self.state_hover = true
            self.entity:GetRoot():GetInherited():ShowAlt(self.alt)
            res.event = GUI_EVENTS.CB_HOVER
            res.coords.x = self.group_id
            res.coords.y = 0
        end
     
    elseif eventData.event == GUI_EVENTS.MOUSE_OUT then
        if self.state_hover then
            self.anim_go = -1
            self.state_hover = false
            self.entity:GetRoot():GetInherited():HideAlt()
            res.event = GUI_EVENTS.CB_OUT
            res.coords.x = self.group_id
            res.coords.y = 0
        end
    end

    return self._base.callback(self, res)
end

function GuiCheck:onActivate()
    self:ApplyNone()
    self.anim_progress = 0
    self.anim_go = 0
end

function GuiCheck:onDeactivate()
    self:ApplyDisable()
    self.anim_progress = 0
    self.anim_go = 0
end

function GuiCheck:onTick(dt)
    if self.anim_go ~= 0 then
        local fade_time = self.anim_go > 0 and self.fadein_time or self.fadeout_time
        self.anim_progress = self.anim_progress + self.anim_go * dt / fade_time
        self.anim_progress = math.max(0, math.min(self.anim_progress, 1))

        local color_box = Vector4Lerp(self.box.color, self.box.color_hover, self.anim_progress)
        local color_check = self.undef and self.check.color_nonactive or (self.state_check and Vector4Lerp(self.check.color, self.check.color_hover, self.anim_progress) or color_box)
        local color_border = Vector4Lerp(self.border.color, self.border.color_hover, self.anim_progress)
        local color_text = Vector4Lerp(self.text.color, self.text.color_hover, self.anim_progress)
        
        if self.boxrect_mat ~= nil then
            self.boxrect_mat:SetVectorByID(color_box, 2)
            self.boxrect_mat:SetVectorByID(color_border, 3)
        end
        if self.checkrect_mat ~= nil then
            self.checkrect_mat:SetVectorByID(color_check, 1)
        end
        if self.str ~= nil then
            self.entity:SetTextColor(self.str, color_text)
        end

        if self.anim_progress == 0 or self.anim_progress == 1 then self.anim_go = 0 end
    end
end

function GuiCheck:GetCheck()
    if self.undef == true then return nil end
    return self.state_check
end

function GuiCheck:SetCheck(check)
    if check == nil then
        self.undef = true
    else
        self.undef = false
        self.state_check = check
    end
    self:UpdateProps()
end