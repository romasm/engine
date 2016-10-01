if not GlobalLightCallback then GlobalLightCallback = {} end

function GlobalLightCallback.StartColorPicking(self, ev)
    if self.picker then 
        self.picker = false
        return true
    else
        ColorPicker:Show(self, self.background.color, true)
        self.picker = true
    end

    self.history = {
        s_oldval = {},
        s_newval = Vector3(0,0,0),
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.globalLight:SetColor(ent, self.s_oldval[i])
                end
                Properties:UpdateData(false, COMPONENTS.GLIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.globalLight:SetColor(ent, self.s_newval)
                end
                Properties:UpdateData(false, COMPONENTS.GLIGHT)
            end,
        msg = "Global Light color"
    }

    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_oldval[i] = Viewport.lua_world.world.globalLight:GetColor(ent)
    end
    return true 
end

function GlobalLightCallback.ColorPicking(self)
    local color = ColorPicker:GetColor()
    color.x = math.pow(color.x, 2.2)
    color.y = math.pow(color.y, 2.2)
    color.z = math.pow(color.z, 2.2)
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.globalLight:SetColor(ent, Vector3(color.x, color.y, color.z))
    end
    return true 
end

function GlobalLightCallback.ColorPicked(self)
    self.history.s_newval.x = self.background.color.x
    self.history.s_newval.y = self.background.color.y
    self.history.s_newval.z = self.background.color.z
    History:Push(self.history)
    return true 
end

function GlobalLightCallback.UpdColor(self, ev)
    local val = Vector3(0,0,0)
    for i, ent in ipairs(Viewport.selection_set) do
        local color = Viewport.lua_world.world.globalLight:GetColor(ent)
        if i > 1 and not (val.x == color.x and val.y == color.y and val.z == color.z) then
            self.background.color = Vector4(0.5,0.5,0.5,1)
            self.background.color_hover = self.background.color
            self.background.color_press = self.background.color
            self:UpdateProps()
            return true
        else val = color end
    end
    val.x = math.pow(val.x, 0.45)
    val.y = math.pow(val.y, 0.45)
    val.z = math.pow(val.z, 0.45)
    self.background.color = Vector4(val.x, val.y, val.z, 1)
    self.background.color_hover = self.background.color
    self.background.color_press = self.background.color
    self:UpdateProps()
    return true 
end

function GlobalLightCallback.StartBrightness(self, ev)
    self.history = {
        s_oldval = {},
        s_newval = {},

        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.globalLight:SetBrightness(ent, self.s_oldval[i])
                end
                Properties:UpdateData(false, COMPONENTS.GLIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.globalLight:SetBrightness(ent, self.s_newval[i])
                end
                Properties:UpdateData(false, COMPONENTS.GLIGHT)
            end,
        msg = "Global Light illuminance"
    }

    local illuminance = self:GetValue()
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_oldval[i] = Viewport.lua_world.world.globalLight:GetBrightness(ent)

        if illuminance then
            Viewport.lua_world.world.globalLight:SetBrightness(ent, EntityTypes.GlobalLight.IlluminanceToBrightness(illuminance))
        end
    end
    return true 
end

function GlobalLightCallback.DragBrightness(self, ev)
    local illuminance = self:GetValue()
    if illuminance == nil then return true end

    for i, ent in ipairs(Viewport.selection_set) do
        local intensity = EntityTypes.GlobalLight.IlluminanceToBrightness(illuminance)
        Viewport.lua_world.world.globalLight:SetBrightness(ent, intensity)
    end
    return true
end

function GlobalLightCallback.EndBrightness(self, ev)
    local illuminance = self:GetValue()
    if illuminance == nil then return true end
    
    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_newval[i] = EntityTypes.GlobalLight.IlluminanceToBrightness(illuminance)
        is_change = is_change or not CMath.IsNearlyEq(self.history.s_oldval[i], self.history.s_newval[i], 0.01)
    end
    if not is_change then return true end
    
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.globalLight:SetBrightness(ent, self.history.s_newval[i])
    end
    
    History:Push(self.history)
    return true
end

function GlobalLightCallback.UpdBrightness(self, ev)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local intensity = Viewport.lua_world.world.globalLight:GetBrightness(ent)
        if i > 1 and val ~= intensity then
            self:SetValue(nil)
            return true
        else val = intensity end
    end
    self:SetValue( EntityTypes.GlobalLight.BrightnessToIlluminance(val) )
    return true 
end

function GlobalLightCallback.StartArea(self, ev)
    self.history = {
        s_oldval = {},
        s_newval = 0,

        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.globalLight:SetArea(ent, self.s_oldval[i])
                end
                Properties:UpdateData(false, COMPONENTS.GLIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.globalLight:SetArea(ent, self.s_newval)
                end
                Properties:UpdateData(false, COMPONENTS.GLIGHT)
            end,
        msg = "Global Light solid angle"
    }

    local area = self:GetValue()
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_oldval[i] = Viewport.lua_world.world.globalLight:GetArea(ent)
        if area then
            Viewport.lua_world.world.globalLight:SetArea(ent, area * math.pi / 180)
        end
    end
    return true 
end

function GlobalLightCallback.DragArea(self, ev)
    local area = self:GetValue()
    if area == nil then return true end
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.globalLight:SetArea(ent, area * math.pi / 180)
    end
    return true
end

function GlobalLightCallback.EndArea(self, ev)
    self.history.s_newval = self:GetValue()
    if self.history.s_newval == nil then return true end
    self.history.s_newval = self.history.s_newval * math.pi / 180

    local is_change = false
    for i, oldval in ipairs(self.history.s_oldval) do
        is_change = is_change or not CMath.IsNearlyEq(oldval, self.history.s_newval, 0.01)
    end
    if not is_change then return true end
    
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.globalLight:SetArea(ent, self.history.s_newval)
    end

    History:Push(self.history)
    return true
end

function GlobalLightCallback.UpdArea(self, ev)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local area = Viewport.lua_world.world.globalLight:GetArea(ent)
        if i > 1 and val ~= area then
            self:SetValue(nil)
            return true
        else val = area end
    end
    self:SetValue(val * 180 / math.pi)
    return true 
end
