if not LightCallback then LightCallback = {} end

function LightCallback.AutoDist(power)
    -- return math.sqrt(power * 0.79577) -- min_brightness = 0.1 -- range = sqrt(brightness / (min_brigthness * 4 * math.pi))
    local min_power = 0.1
    min_power = min_power * 4 * math.pi
    return math.sqrt(power / min_power)
end

function LightCallback.SetCastShadows(self, ev, cast)
    local alpha_shadows = self.entity:GetParent():GetChildById('alpha_shadows')
    if cast then alpha_shadows:Activate()
    else alpha_shadows:Deactivate() end

    local history = {
        s_oldval = {},
        s_newval = false,
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.light:SetCastShadows(ent, self.s_oldval[i])
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.light:SetCastShadows(ent, self.s_newval)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light cast shadow"
    }

    history.s_newval = cast
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.light:GetCastShadows(ent)
        Viewport.lua_world.world.light:SetCastShadows(ent, cast)
    end

    History:Push(history)
    return true
end

function LightCallback.UpdCastShadows(self, ev)
    local alpha_shadows = self.entity:GetParent():GetChildById('alpha_shadows')
    local val = false
    for i, ent in ipairs(Viewport.selection_set) do
        local cast = Viewport.lua_world.world.light:GetCastShadows(ent)
        if i > 1 and val ~= cast then
            self:SetCheck(nil)
            alpha_shadows:Activate()
            return true
        else val = cast end
    end
    self:SetCheck(val)
    if val then alpha_shadows:Activate()
    else alpha_shadows:Deactivate() end
    return true 
end

function LightCallback.SetTransparentShadows(self, ev, cast)
    local history = {
        s_oldval = {},
        s_newval = false,
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.light:SetTransparentShadows(ent, self.s_oldval[i])
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.light:SetTransparentShadows(ent, self.s_newval)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light cast transparent shadow"
    }

    history.s_newval = cast
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.light:GetTransparentShadows(ent)
        Viewport.lua_world.world.light:SetTransparentShadows(ent, cast)
    end

    History:Push(history)
    return true
end

function LightCallback.UpdTransparentShadows(self, ev)
    local val = false
    for i, ent in ipairs(Viewport.selection_set) do
        local cast = Viewport.lua_world.world.light:GetTransparentShadows(ent)
        if i > 1 and val ~= cast then
            self:SetCheck(nil)
            return true
        else val = cast end
    end
    self:SetCheck(val)
    return true 
end

function LightCallback.StartColorPicking(self, ev)
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
                    Viewport.lua_world.world.light:SetColor(ent, self.s_oldval[i])
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.light:SetColor(ent, self.s_newval)
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light color"
    }

    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_oldval[i] = Viewport.lua_world.world.light:GetColor(ent)
    end
    return true 
end

function LightCallback.ColorPicking(self)
    local color = ColorPicker:GetColor()
    color.x = CMath.GammaToLin(color.x)
    color.y = CMath.GammaToLin(color.y)
    color.z = CMath.GammaToLin(color.z)
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.light:SetColor(ent, Vector3(color.x, color.y, color.z))
        Viewport.lua_world.world.light:UpdateLightProps(ent)
    end
    return true 
end

function LightCallback.ColorPicked(self)
    if not ColorPicker:IsChanged() then return end
    self.history.s_newval.x = CMath.GammaToLin(self.background.color.x)
    self.history.s_newval.y = CMath.GammaToLin(self.background.color.y)
    self.history.s_newval.z = CMath.GammaToLin(self.background.color.z)
    History:Push(self.history)
    return true 
end

function LightCallback.UpdColor(self, ev)
    local val = Vector3(0,0,0)
    for i, ent in ipairs(Viewport.selection_set) do
        local color = Viewport.lua_world.world.light:GetColor(ent)
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

function LightCallback.StartBrightness(self, ev)
    self.history = {
        s_oldval = {},
        s_newval = {},

        s_oldrange = {},
        s_newrange = {},

        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.light:SetBrightness(ent, self.s_oldval[i])
                    if self.s_oldrange[i] >= 0 then
                        Viewport.lua_world.world.light:SetRange(ent, self.s_oldrange[i])
                        Viewport.lua_world.world.light:UpdateLightProps(ent)
                        Viewport.lua_world.world.light:UpdateShadows(ent)
                    else
                        Viewport.lua_world.world.light:UpdateLightProps(ent)
                    end
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.light:SetBrightness(ent, self.s_newval[i])
                    if self.s_newrange[i] >= 0 then
                        Viewport.lua_world.world.light:SetRange(ent, self.s_newrange[i])
                        Viewport.lua_world.world.light:UpdateLightProps(ent)
                        Viewport.lua_world.world.light:UpdateShadows(ent)
                    else
                        Viewport.lua_world.world.light:UpdateLightProps(ent)
                    end
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light lum power"
    }

    local is_auto_range = self.entity:GetParent():GetChildById('auto_range'):GetInherited():GetCheck()

    local lumpow = self:GetValue()
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_oldval[i] = Viewport.lua_world.world.light:GetBrightness(ent)

        if is_auto_range == true then self.history.s_oldrange[i] = Viewport.lua_world.world.light:GetRange(ent)
        elseif is_auto_range == false then self.history.s_oldrange[i] = -1
        elseif is_auto_range == nil then 
            local power = EntityTypes.LocalLight.LumIntensityToLumPower(self.history.s_oldval[i], ent, Viewport.lua_world.world)
            local new_range = LightCallback.AutoDist(power)
            local range = Viewport.lua_world.world.light:GetRange(ent)
            local is_auto = CMath.IsNearlyEq(new_range, range, 0.01)

            if is_auto then self.history.s_oldrange[i] = range
            else self.history.s_oldrange[i] = -1 end
        end

        if lumpow then
            Viewport.lua_world.world.light:SetBrightness(ent, EntityTypes.LocalLight.LumPowerToLumIntensity(lumpow, ent, Viewport.lua_world.world))
            Viewport.lua_world.world.light:UpdateLightProps(ent)
        end
    end
    return true 
end

function LightCallback.DragBrightness(self, ev)
    local lumpow = self:GetValue()
    if lumpow == nil then return true end

    local update_range_need = false
    for i, ent in ipairs(Viewport.selection_set) do
        local intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(lumpow, ent, Viewport.lua_world.world)

        Viewport.lua_world.world.light:SetBrightness(ent, intensity)

        if self.history.s_oldrange[i] >= 0 then
            Viewport.lua_world.world.light:SetRange(ent, LightCallback.AutoDist(lumpow))
            update_range_need = true
            Viewport.lua_world.world.light:UpdateLightProps(ent)
            Viewport.lua_world.world.light:UpdateShadows(ent)
        else
            Viewport.lua_world.world.light:UpdateLightProps(ent)
        end
    end
    if update_range_need then
        local tf_range = self.entity:GetParent():GetChildById('tf_range')
        LightCallback.UpdRange(tf_range:GetInherited(), ev)
    end
    return true
end

function LightCallback.EndBrightness(self, ev)
    local lumpow = self:GetValue()
    if lumpow == nil then return true end
    
    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_newval[i] = EntityTypes.LocalLight.LumPowerToLumIntensity(lumpow, ent, Viewport.lua_world.world)
        is_change = is_change or not CMath.IsNearlyEq(self.history.s_oldval[i], self.history.s_newval[i], 0.01)
    end
    if not is_change then return true end
    
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.light:SetBrightness(ent, self.history.s_newval[i])

        if self.history.s_oldrange[i] >= 0 then
            local power = EntityTypes.LocalLight.LumIntensityToLumPower(self.history.s_newval[i], ent, Viewport.lua_world.world)
            self.history.s_newrange[i] = LightCallback.AutoDist(power)
            Viewport.lua_world.world.light:SetRange(ent, self.history.s_newrange[i])
            Viewport.lua_world.world.light:UpdateLightProps(ent)
            Viewport.lua_world.world.light:UpdateShadows(ent)
        else
            self.history.s_newrange[i] = -1
            Viewport.lua_world.world.light:UpdateLightProps(ent)
        end
    end
    local tf_range = self.entity:GetParent():GetChildById('tf_range')
    LightCallback.UpdRange(tf_range:GetInherited(), ev)
    History:Push(self.history)
    return true
end

function LightCallback.UpdBrightness(self, ev)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
        local lumpow = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)
        if i > 1 and val ~= lumpow then
            self:SetValue(nil)
            return true
        else val = lumpow end
    end
    self:SetValue(val)
    return true 
end

function LightCallback.SetRange(self, ev)
    if not self:IsChanged() then return true end
    local val = self:GetNum()
    if val == nil then
        self.events[GUI_EVENTS.UPDATE](self, ev)
        return true 
    end

    local history = {
        s_oldval = {},
        s_newval = 0,
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.light:SetRange(ent, self.s_oldval[i])
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.light:SetRange(ent, self.s_newval)
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light max distance"
    }

    history.s_newval = val
    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.light:GetRange(ent)

        is_change = is_change or history.s_oldval[i] ~= history.s_newval

        Viewport.lua_world.world.light:SetRange(ent, val)
        Viewport.lua_world.world.light:UpdateLightProps(ent)
        Viewport.lua_world.world.light:UpdateShadows(ent)
    end

    if not is_change then return true end

    History:Push(history)
    return true
end

function LightCallback.UpdRange(self, ev)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local range = Viewport.lua_world.world.light:GetRange(ent)
        if i > 1 and val ~= range then
            self:SetNum(nil)
            return true
        else val = range end
    end
    self:SetNum(val)
    return true 
end

function LightCallback.SetAutoRange(self, ev, auto)
    local tf_range = self.entity:GetParent():GetChildById('tf_range')
    if auto then tf_range:Deactivate()
    else tf_range:Activate() end

    local history = {
        s_oldval = {},
        s_newval = {},
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.light:SetRange(ent, self.s_oldval[i])
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_newval then return end
                    Viewport.lua_world.world.light:SetRange(ent, self.s_newval[i])
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light max distance auto"
    }

    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        local brightness = Viewport.lua_world.world.light:GetBrightness(ent)
        local power = EntityTypes.LocalLight.LumIntensityToLumPower(brightness, ent, Viewport.lua_world.world)
        history.s_newval[i] = LightCallback.AutoDist(power)

        history.s_oldval[i] = Viewport.lua_world.world.light:GetRange(ent)

        if not CMath.IsNearlyEq(history.s_newval[i], history.s_oldval[i], 0.01) then 
            is_change = true
            Viewport.lua_world.world.light:SetRange(ent, history.s_newval[i])
            Viewport.lua_world.world.light:UpdateLightProps(ent)
            Viewport.lua_world.world.light:UpdateShadows(ent)
        end
    end

    if not is_change then return true end

    LightCallback.UpdRange(tf_range:GetInherited(), ev)

    History:Push(history)
    return true 
end

function LightCallback.UpdAutoRange(self, ev)
    local tf_range = self.entity:GetParent():GetChildById('tf_range')
    local val = false
    for i, ent in ipairs(Viewport.selection_set) do
        local range = Viewport.lua_world.world.light:GetRange(ent)
        local brightness = Viewport.lua_world.world.light:GetBrightness(ent)
        local power = EntityTypes.LocalLight.LumIntensityToLumPower(brightness, ent, Viewport.lua_world.world)
        local new_range = LightCallback.AutoDist(power)

        local is_auto = CMath.IsNearlyEq(new_range, range, 0.01)
        if i > 1 and val ~= is_auto then
            self:SetCheck(nil)
            tf_range:Activate()
            return true
        else val = is_auto end
    end
    self:SetCheck(val)
    if val then tf_range:Deactivate()
    else tf_range:Activate() end
    return true 
end

function LightCallback.SetType(self, ev)
    local selected = self:GetSelected()
    local group = self.entity:GetParent()
    LightCallback.updateAreaInput(group, selected)
    
    local history = {
        s_oldval = {},
        s_newval = false,
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    
                    local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
                    local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

                    Viewport.lua_world.world.light:SetType(ent, self.s_oldval[i])

                    intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
                    Viewport.lua_world.world.light:SetBrightness(ent, intensity)
                    
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
                    local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

                    Viewport.lua_world.world.light:SetType(ent, self.s_newval)

                    intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
                    Viewport.lua_world.world.light:SetBrightness(ent, intensity)

                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light source type"
    }

    history.s_newval = selected - 1
    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.light:GetType(ent)
        if history.s_oldval[i] ~= history.s_newval then
            is_change = true

            local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
            local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)
            
            Viewport.lua_world.world.light:SetType(ent, history.s_newval)

            intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
            Viewport.lua_world.world.light:SetBrightness(ent, intensity)
            
            Viewport.lua_world.world.light:UpdateLightProps(ent)
            Viewport.lua_world.world.light:UpdateShadows(ent)
        end
    end

    if not is_change then return true end

    Properties:UpdateData(false, COMPONENTS.LIGHT)
    History:Push(history)
    return true 
end

function LightCallback.UpdType(self, ev)
    local group = self.entity:GetParent()
    local val = 0
    local undef = false
    local all_spots = true
    for i, ent in ipairs(Viewport.selection_set) do
        local ltype = Viewport.lua_world.world.light:GetType(ent)
        all_spots = all_spots and (ltype+1) >= LIGHT_TYPE.SPOT
        if i > 1 and val ~= ltype then undef = true
        else val = ltype end
    end

    if undef then
        self:SetSelected(0)
        LightCallback.updateAreaInput(group, all_spots and LIGHT_TYPE.SPOT or LIGHT_TYPE.POINT)
    else
        local lt = val+1
        self:SetSelected(lt)
        LightCallback.updateAreaInput(group, lt)
    end
    return true 
end

function LightCallback.StartAreaX(self, ev)
    self.history = {
        s_oldval = {},
        s_newval = 0,

        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
                    local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

                    Viewport.lua_world.world.light:SetAreaX(ent, self.s_oldval[i])

                    intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
                    Viewport.lua_world.world.light:SetBrightness(ent, intensity)

                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
                    local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

                    Viewport.lua_world.world.light:SetAreaX(ent, self.s_newval)

                    intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
                    Viewport.lua_world.world.light:SetBrightness(ent, intensity)

                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light area X"
    }

    local areax = self:GetValue()
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_oldval[i] = Viewport.lua_world.world.light:GetAreaX(ent)

        if areax then
            local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
            local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

            Viewport.lua_world.world.light:SetAreaX(ent, areax)

            intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
            Viewport.lua_world.world.light:SetBrightness(ent, intensity)

            Viewport.lua_world.world.light:UpdateLightProps(ent)
            Viewport.lua_world.world.light:UpdateShadows(ent)
        end
    end
    return true 
end

function LightCallback.DragAreaX(self, ev)
    local areax = self:GetValue()
    if areax == nil then return true end
    for i, ent in ipairs(Viewport.selection_set) do
        local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
        local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

        Viewport.lua_world.world.light:SetAreaX(ent, areax)

        intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
        Viewport.lua_world.world.light:SetBrightness(ent, intensity)

        Viewport.lua_world.world.light:UpdateLightProps(ent)
        Viewport.lua_world.world.light:UpdateShadows(ent)
    end
    return true
end

function LightCallback.EndAreaX(self, ev)
    self.history.s_newval = self:GetValue()
    if self.history.s_newval == nil then return true end

    local is_change = false
    for i, oldval in ipairs(self.history.s_oldval) do
        is_change = is_change or not CMath.IsNearlyEq(oldval, self.history.s_newval, 0.01)
    end
    if not is_change then return true end
    
    for i, ent in ipairs(Viewport.selection_set) do
        local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
        local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

        Viewport.lua_world.world.light:SetAreaX(ent, self.history.s_newval)

        intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
        Viewport.lua_world.world.light:SetBrightness(ent, intensity)

        Viewport.lua_world.world.light:UpdateLightProps(ent)
        Viewport.lua_world.world.light:UpdateShadows(ent)
    end

    History:Push(self.history)
    return true
end

function LightCallback.UpdAreaX(self, ev)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local area = Viewport.lua_world.world.light:GetAreaX(ent)
        if i > 1 and val ~= area then
            self:SetValue(nil)
            return true
        else val = area end
    end
    self:SetValue(val)
    return true 
end

function LightCallback.StartAreaY(self, ev)
    self.history = {
        s_oldval = {},
        s_newval = 0,

        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
                    local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

                    Viewport.lua_world.world.light:SetAreaY(ent, self.s_oldval[i])

                    intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
                    Viewport.lua_world.world.light:SetBrightness(ent, intensity)

                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
                    local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

                    Viewport.lua_world.world.light:SetAreaY(ent, self.s_newval)

                    intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
                    Viewport.lua_world.world.light:SetBrightness(ent, intensity)

                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light area Y"
    }

    local areay = self:GetValue()
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_oldval[i] = Viewport.lua_world.world.light:GetAreaY(ent)

        if areay then
            local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
            local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

            Viewport.lua_world.world.light:SetAreaY(ent, areay)

            intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
            Viewport.lua_world.world.light:SetBrightness(ent, intensity)

            Viewport.lua_world.world.light:UpdateLightProps(ent)
            Viewport.lua_world.world.light:UpdateShadows(ent)
        end
    end
    return true 
end

function LightCallback.DragAreaY(self, ev)
    local areay = self:GetValue()
    if areay == nil then return true end
    for i, ent in ipairs(Viewport.selection_set) do
        local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
        local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

        Viewport.lua_world.world.light:SetAreaY(ent, areay)

        intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
        Viewport.lua_world.world.light:SetBrightness(ent, intensity)

        Viewport.lua_world.world.light:UpdateLightProps(ent)
        Viewport.lua_world.world.light:UpdateShadows(ent)
    end
    return true
end

function LightCallback.EndAreaY(self, ev)
    self.history.s_newval = self:GetValue()
    if self.history.s_newval == nil then return true end

    local is_change = false
    for i, oldval in ipairs(self.history.s_oldval) do
        is_change = is_change or not CMath.IsNearlyEq(oldval, self.history.s_newval, 0.01)
    end
    if not is_change then return true end
    
    for i, ent in ipairs(Viewport.selection_set) do
        local intensity = Viewport.lua_world.world.light:GetBrightness(ent)
        local power = EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, Viewport.lua_world.world)

        Viewport.lua_world.world.light:SetAreaY(ent, self.history.s_newval)

        intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, Viewport.lua_world.world)
        Viewport.lua_world.world.light:SetBrightness(ent, intensity)

        Viewport.lua_world.world.light:UpdateLightProps(ent)
        Viewport.lua_world.world.light:UpdateShadows(ent)
    end

    History:Push(self.history)
    return true
end

function LightCallback.UpdAreaY(self, ev)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local area = Viewport.lua_world.world.light:GetAreaY(ent)
        if i > 1 and val ~= area then
            self:SetValue(nil)
            return true
        else val = area end
    end
    self:SetValue(val)
    return true 
end

function LightCallback.StartConeOut(self, ev)
    self.history = {
        s_oldval = {},
        s_newval = 0,

        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.light:SetConeOut(ent, self.s_oldval[i])
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.light:SetConeOut(ent, self.s_newval)
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                    Viewport.lua_world.world.light:UpdateShadows(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light inner cone"
    }

    local coneout = self:GetValue()
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_oldval[i] = Viewport.lua_world.world.light:GetConeOut(ent)

        if coneout then
            local coneout_rad = EntityTypes.LocalLight.AngleToCone(coneout)
            Viewport.lua_world.world.light:SetConeOut(ent, coneout_rad)
            Viewport.lua_world.world.light:UpdateLightProps(ent)
            Viewport.lua_world.world.light:UpdateShadows(ent)
        end
    end
    return true 
end

function LightCallback.DragConeOut(self, ev)
    local coneout = self:GetValue()
    if coneout == nil then return true end
    coneout = EntityTypes.LocalLight.AngleToCone(coneout)
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.light:SetConeOut(ent, coneout)
        Viewport.lua_world.world.light:UpdateLightProps(ent)
        Viewport.lua_world.world.light:UpdateShadows(ent)
    end
    return true
end

function LightCallback.EndConeOut(self, ev)
    self.history.s_newval = self:GetValue()
    if self.history.s_newval == nil then return true end
    self.history.s_newval = EntityTypes.LocalLight.AngleToCone(self.history.s_newval)

    local is_change = false
    for i, oldval in ipairs(self.history.s_oldval) do
        is_change = is_change or not CMath.IsNearlyEq(oldval, self.history.s_newval, 0.01)
    end
    if not is_change then return true end
    
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.light:SetConeOut(ent, self.history.s_newval)
        Viewport.lua_world.world.light:UpdateLightProps(ent)
        Viewport.lua_world.world.light:UpdateShadows(ent)
    end

    History:Push(self.history)
    return true
end

function LightCallback.UpdConeOut(self, ev)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local coneout = Viewport.lua_world.world.light:GetConeOut(ent)
        if i > 1 and val ~= coneout then
            self:SetValue(nil)
            return true
        else val = coneout end
    end
    self:SetValue(EntityTypes.LocalLight.ConeToAngle(val))
    return true 
end

function LightCallback.StartConeIn(self, ev)
    self.history = {
        s_oldval = {},
        s_newval = 0,

        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.light:SetConeIn(ent, self.s_oldval[i])
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.light:SetConeIn(ent, self.s_newval)
                    Viewport.lua_world.world.light:UpdateLightProps(ent)
                end
                Properties:UpdateData(false, COMPONENTS.LIGHT)
            end,
        msg = "Light inner cone"
    }

    local conein = self:GetValue()
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_oldval[i] = Viewport.lua_world.world.light:GetConeIn(ent)

        if conein then
            Viewport.lua_world.world.light:SetConeIn(ent, EntityTypes.LocalLight.AngleToCone(conein))
            Viewport.lua_world.world.light:UpdateLightProps(ent)
        end
    end
    return true 
end

function LightCallback.DragConeIn(self, ev)
    local conein = self:GetValue()
    if conein == nil then return true end
    conein = EntityTypes.LocalLight.AngleToCone(conein)
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.light:SetConeIn(ent, conein)
        Viewport.lua_world.world.light:UpdateLightProps(ent)
    end
    return true
end

function LightCallback.EndConeIn(self, ev)
    self.history.s_newval = self:GetValue()
    if self.history.s_newval == nil then return true end
    self.history.s_newval = EntityTypes.LocalLight.AngleToCone(self.history.s_newval)

    local is_change = false
    for i, oldval in ipairs(self.history.s_oldval) do
        is_change = is_change or not CMath.IsNearlyEq(oldval, self.history.s_newval, 0.01)
    end
    if not is_change then return true end
    
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.light:SetConeIn(ent, self.history.s_newval)
        Viewport.lua_world.world.light:UpdateLightProps(ent)
    end

    History:Push(self.history)
    return true
end

function LightCallback.UpdConeIn(self, ev)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local conein = Viewport.lua_world.world.light:GetConeIn(ent)
        if i > 1 and val ~= conein then
            self:SetValue(nil)
            return true
        else val = conein end
    end
    self:SetValue(EntityTypes.LocalLight.ConeToAngle(val))
    return true 
end

----- support
function LightCallback.updateAreaInput(group, selected)
    if selected <= LIGHT_TYPE.TUBE then
        group:GetChildById('cone_in').enable = false
        group:GetChildById('cone_in_slider').enable = false
        group:GetChildById('cone_out').enable = false
        group:GetChildById('cone_out_slider').enable = false

        if selected == LIGHT_TYPE.POINT then
            group:GetChildById('draw_shape'):Deactivate()
            group:GetChildById('area_x').enable = false
            group:GetChildById('area_x_slider').enable = false
            group:GetChildById('area_y').enable = false
            group:GetChildById('area_y_slider').enable = false

            group:GetInherited():UpdateH(272)
            group:GetParent():GetInherited().window.entity:UpdateSize()

        elseif selected == LIGHT_TYPE.SPHERE then
            group:GetChildById('draw_shape'):Activate()
            local str = group:GetChildById('area_x')
            str.enable = true
            str:GetInherited():SetString("Sphere radius")
            group:GetChildById('area_x_slider').enable = true
            group:GetChildById('area_y').enable = false
            group:GetChildById('area_y_slider').enable = false

            group:GetInherited():UpdateH(312)
            group:GetParent():GetInherited().window.entity:UpdateSize()

        elseif selected == LIGHT_TYPE.TUBE then
            group:GetChildById('draw_shape'):Activate()
            local str = group:GetChildById('area_x')
            str.enable = true
            str:GetInherited():SetString("Tube radius")
            group:GetChildById('area_x_slider').enable = true
            str = group:GetChildById('area_y')
            str.enable = true
            str:GetInherited():SetString("Tube length")
            str.enable = true
            group:GetChildById('area_y_slider').enable = true

            group:GetInherited():UpdateH(342)
            group:GetParent():GetInherited().window.entity:UpdateSize()
        end

    elseif selected == LIGHT_TYPE.SPOT then
        group:GetChildById('draw_shape'):Deactivate()
        group:GetChildById('area_x').enable = false
        group:GetChildById('area_x_slider').enable = false
        group:GetChildById('area_y').enable = false
        group:GetChildById('area_y_slider').enable = false

        local cone = group:GetChildById('cone_in')
        cone.enable = true
        cone.top = 312
        cone = group:GetChildById('cone_in_slider')
        cone.enable = true
        cone.top = 310
        cone = group:GetChildById('cone_out')
        cone.enable = true
        cone.top = 282
        cone = group:GetChildById('cone_out_slider')
        cone.enable = true
        cone.top = 280

        group:GetInherited():UpdateH(342)
        group:GetParent():GetInherited().window.entity:UpdateSize()

    elseif selected == LIGHT_TYPE.DISK then
        group:GetChildById('draw_shape'):Activate()
        local str = group:GetChildById('area_x')
        str.enable = true
        str:GetInherited():SetString("Disk radius")
        group:GetChildById('area_x_slider').enable = true
        group:GetChildById('area_y').enable = false
        group:GetChildById('area_y_slider').enable = false

        local cone = group:GetChildById('cone_in')
        cone.enable = true
        cone.top = 352
        cone = group:GetChildById('cone_in_slider')
        cone.enable = true
        cone.top = 350
        cone = group:GetChildById('cone_out')
        cone.enable = true
        cone.top = 322
        cone = group:GetChildById('cone_out_slider')
        cone.enable = true
        cone.top = 320

        group:GetInherited():UpdateH(382)
        group:GetParent():GetInherited().window.entity:UpdateSize()

    elseif selected == LIGHT_TYPE.RECT then
        group:GetChildById('draw_shape'):Activate()
        local str = group:GetChildById('area_x')
        str.enable = true
        str:GetInherited():SetString("Rect width")
        group:GetChildById('area_x_slider').enable = true
        str = group:GetChildById('area_y')
        str.enable = true
        str:GetInherited():SetString("Rect length")
        str.enable = true
        group:GetChildById('area_y_slider').enable = true

        local cone = group:GetChildById('cone_in')
        cone.enable = true
        cone.top = 382
        cone = group:GetChildById('cone_in_slider')
        cone.enable = true
        cone.top = 380
        cone = group:GetChildById('cone_out')
        cone.enable = true
        cone.top = 352
        cone = group:GetChildById('cone_out_slider')
        cone.enable = true
        cone.top = 350

        group:GetInherited():UpdateH(412)
        group:GetParent():GetInherited().window.entity:UpdateSize()
    end
end