local RECT_MAT = "../resources/shaders/gui/rect"

if not GuiRect then GuiRect = class(GuiEntity) end

function GuiRect:init(props)
    self.background = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0)
    }
    self.border = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        width = 0
    }
    
    self.material = {shader = RECT_MAT}

    self._base.init(self, props)
    
    self.rect = self.entity:AddRect(self.material.shader)
    self.rect_mat = self:SetRectMaterial(self.rect, self.material)
    
    self:UpdateProps()
end

function GuiRect:ApplyProps(props)
    self._base.ApplyProps(self, props)
    
    if props.material ~= nil then self.material = props.material end

    if props.background ~= nil then
        if props.background.color ~= nil then 
            self.background.color = type(props.background.color) == 'string' and CoreGui.GetColor(props.background.color) or props.background.color end
        if props.background.color_nonactive ~= nil then 
            self.background.color_nonactive = type(props.background.color_nonactive) == 'string' and CoreGui.GetColor(props.background.color_nonactive) or props.background.color_nonactive end
    end

    if props.border ~= nil then
        if props.border.color ~= nil then 
            self.border.color = type(props.border.color) == 'string' and CoreGui.GetColor(props.border.color) or props.border.color end
        if props.border.color_nonactive ~= nil then 
            self.border.color_nonactive = type(props.border.color_nonactive) == 'string' and CoreGui.GetColor(props.border.color_nonactive) or props.border.color_nonactive end
        if props.border.width ~= nil then self.border.width = props.border.width end
    end
end

function GuiRect:UpdateProps()
    if self.entity:IsActivated() and self.entity:IsActivatedBranch() then self:onActivate()
    else self:onDeactivate() end
end

function GuiRect:onMoveResize(is_move, is_resize)
    if not self._base.onMoveResize(self, is_move, is_resize) then return false end

    local rect = self.entity:GetRectAbsolute()
    rect.l = 0
    rect.t = 0
    self.entity:SetRect(self.rect, rect)

    local shader_data = Vector4(0,0,0,0)
    shader_data.x = self.border.width / rect.w
    shader_data.y = self.border.width / rect.h
    shader_data.z = 1 - shader_data.x
    shader_data.w = 1 - shader_data.y
    self.rect_mat:SetVectorByID(shader_data, 1)

    return true
end

function GuiRect:callback(eventData)
    local res = eventData

    if eventData.event == GUI_EVENTS.MOUSE_DOWN then 
        self.entity:SetHierarchyFocusOnMe(false)
        if eventData.entity:is_eq(self.entity) then self.entity:SetFocus(HEntity()) end
        res.entity = self.entity

    elseif eventData.event == GUI_EVENTS.UNFOCUS then
        if eventData.entity:is_eq(self.entity) then self.entity:SetFocus(HEntity()) end

    end

    return self._base.callback(self, res)
end

function GuiRect:onActivate()
    self.rect_mat:SetVectorByID(self.background.color, 2)
    self.rect_mat:SetVectorByID(self.border.color, 3)
end

function GuiRect:onDeactivate()
    self.rect_mat:SetVectorByID(self.background.color_nonactive, 2)
    self.rect_mat:SetVectorByID(self.border.color_nonactive, 3)
end