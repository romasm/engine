if not GuiString then GuiString = class(GuiEntity) end

-- public
function GuiString:init(props)
    self.str = ""
    self.static = true
    self.length = 0
    self.font = ""

    self.offset = { x = 0, y = 0 }
    self.center = { x = true, y = true }
    self.padding = { x = -1, y = -1 }

    self.color = Vector4(0.0, 0.0, 0.0, 1.0)
    self.color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0)
    
    self:base(GuiString).init(self, props)
    
    if self.font:len() == 0 or 
        (self.str:len() == 0 and self.static) or 
        (self.str:len() == 0 and not self.static and self.length == 0) then
        error("Wrong params for GuiString named "..self.entity:GetID())
        return
    end
    
    self.text = self.entity:AddText(self.font, self.str, "", self.static, self.length)
    
    self:UpdateProps()
end

function GuiString:ApplyProps(props)
    self._base.ApplyProps(self, props)
    
    if props.str ~= nil then self.str = props.str end
    if props.static ~= nil then self.static = props.static end
    if props.length ~= nil then self.length = props.length end
    if props.font ~= nil then self.font = props.font end

    if props.offset ~= nil then
        if props.offset.x ~= nil then self.offset.x = props.offset.x end
        if props.offset.y ~= nil then self.offset.y = props.offset.y end
    end
    if props.center ~= nil then
        if props.center.x ~= nil then self.center.x = props.center.x end
        if props.center.y ~= nil then self.center.y = props.center.y end
    end
    if props.padding ~= nil then
        if props.padding.x ~= nil then self.padding.x = props.padding.x end
        if props.padding.y ~= nil then self.padding.y = props.padding.y end
    end

    if props.color ~= nil then 
        self.color = type(props.color) == 'string' and CoreGui.GetColor(props.color) or props.color end
    if props.color_nonactive ~= nil then 
        self.color_nonactive = type(props.color_nonactive) == 'string' and CoreGui.GetColor(props.color_nonactive) or props.color_nonactive end
end

function GuiString:UpdateProps()
    if self.entity:IsActivated() and self.entity:IsActivatedBranch() then self:onActivate()
    else self:onDeactivate() end
end

function GuiString:onMoveResize(is_move, is_resize)
    if not self._base.onMoveResize(self, is_move, is_resize) then return false end

    if self.text == nil then return end

    local text_bounds = self.entity:GetTextBounds(self.text)

    local need_resize = false
    if self.padding.x >= 0 and self.center.x then
        self.entity.width = text_bounds.w + self.padding.x * 2
        need_resize = true
    end
    if self.padding.y >= 0 and self.center.y then
        self.entity.height = text_bounds.h + self.padding.y * 2
        need_resize = true
    end

    if need_resize then self.entity:RecalcPosSize() end

    local rect = self.entity:GetRectAbsolute()
    if self.center.x then self.offset.x = (rect.w - text_bounds.w) / 2 end
    if self.center.y then self.offset.y = (rect.h - text_bounds.h) / 2 end

    self.entity:SetTextPos(self.text, self.offset.x, self.offset.y)

    return true
end

function GuiString:callback(eventData)
    local res = eventData
    res.entity = self.entity
    
    if eventData.event == GUI_EVENTS.MOUSE_DOWN then self.entity:SetHierarchyFocusOnMe(false) end

    return self._base.callback(self, res)
end

function GuiString:onActivate()
    if self.text ~= nil then
        self.entity:SetTextColor(self.text, self.color)
    end
end

function GuiString:onDeactivate()
    if self.text ~= nil then
        self.entity:SetTextColor(self.text, self.color_nonactive)
    end
end

function GuiString:SetString(str)
    if self.static then return end
    
    self.str = str
    self.entity:SetText(self.text, self.str)
    self:onMoveResize(false, true)
end

function GuiString:GetTextSize()
    return self.entity:GetTextBounds(self.text)
end