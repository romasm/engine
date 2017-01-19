if not MaterialPropsCallback then MaterialPropsCallback = {} end

function MaterialPropsCallback.StartColorPicking(self, id, str)
    if self.picker then 
        self.picker = false
        return true
    else
        ColorPicker:Show(self, self.background.color, false)
        self.picker = true
    end

    self.history = {
        s_oldval = Vector4(0,0,0,0),
        s_newval = Vector4(0,0,0,0),
        undo = function(self) 
                MaterialProps.material:SetVectorByID(self.s_oldval, id, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetVectorByID(self.s_newval, id, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = str.. " color"
    }

    self.history.s_oldval = MaterialProps.material:GetVectorByID(id, SHADERS.PS)
    return true
end

function MaterialPropsCallback.ColorPicking(self, id)
    local color = ColorPicker:GetColor()
    color.w = self.history.s_oldval.w
    MaterialProps.material:SetVectorByID(color, id, SHADERS.PS)
    return true
end

function MaterialPropsCallback.ColorPicked(self)
    if not ColorPicker:IsChanged() then return end
    self.history.s_newval.x = self.background.color.x
    self.history.s_newval.y = self.background.color.y
    self.history.s_newval.z = self.background.color.z
    self.history.s_newval.w = self.history.s_oldval.w
    History:Push(self.history)
    return true
end

function MaterialPropsCallback.UpdColor(self, id)
    local val = MaterialProps.material:GetVectorByID(id, SHADERS.PS)
    self.background.color = Vector4(val.x, val.y, val.z, 1)
    self.background.color_hover = self.background.color
    self.background.color_press = self.background.color
    self:UpdateProps()
    return true
end

function MaterialPropsCallback.StartValue(self, id, str)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        undo = function(self) 
                MaterialProps.material:SetFloatByID(self.s_oldval, id, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloatByID(self.s_newval, id, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = str.. " value"
    }

    self.history.s_oldval = MaterialProps.material:GetFloatByID(id, SHADERS.PS)
    MaterialProps.material:SetFloatByID(self:GetValue(), id, SHADERS.PS)
    return true
end

function MaterialPropsCallback.DragValue(self, id)
    MaterialProps.material:SetFloatByID(self:GetValue(), id, SHADERS.PS)
    return true
end

function MaterialPropsCallback.EndValue(self, id)
    self.history.s_newval = self:GetValue()
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    MaterialProps.material:SetFloatByID(self.history.s_newval, id, SHADERS.PS)
    History:Push(self.history)
    return true
end

function MaterialPropsCallback.UpdValue(self, id)
    self:SetValue(MaterialProps.material:GetFloatByID(id, SHADERS.PS))
    return true
end

-----

function MaterialPropsCallback.SetShader(self, ev)
    
    return true
end

-- ALBEDO
function MaterialPropsCallback.SetAlbedoTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        s_oldflag = 0,
        s_newflag = 0,
        undo = function(self) 
                MaterialProps.material:SetTextureName(self.s_oldval, "albedoTex", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_oldflag, "albedo_tex", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureName(self.s_newval, "albedoTex", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_newflag, "albedo_tex", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Albedo texture"
    }

    history.s_oldval = MaterialProps.material:GetTextureName("albedoTex", SHADERS.PS)
    history.s_oldflag = MaterialProps.material:GetFloat("albedo_tex", SHADERS.PS)

    history.s_newval = texture
    history.s_newflag = texture:len() == 0 and 0 or 1

    MaterialProps.material:SetTextureName(history.s_newval, "albedoTex", SHADERS.PS)
    MaterialProps.material:SetFloat(history.s_newflag, "albedo_tex", SHADERS.PS)

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdAlbedoTex(self, ev)
    local is_tex = MaterialProps.material:GetFloatByID(0, SHADERS.PS)
    if is_tex == 0 then
        self:SetTexture(nil)
    else
        local texture = MaterialProps.material:GetTextureNameByID(0, SHADERS.PS)
        self:SetTexture(texture)
    end
    return true
end

-- NORMAL
function MaterialPropsCallback.SetNormalTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        s_oldflag = 0,
        s_newflag = 0,
        undo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_oldval, 1, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_oldflag, 1, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_newval, 1, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_newflag, 1, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Normals texture"
    }

    history.s_oldval = MaterialProps.material:GetTextureNameByID(1, SHADERS.PS)
    history.s_oldflag = MaterialProps.material:GetFloatByID(1, SHADERS.PS)

    history.s_newval = texture
    history.s_newflag = texture:len() == 0 and 0 or (history.s_oldflag == 0 and 1 or history.s_oldflag)

    MaterialProps.material:SetTextureNameByID(history.s_newval, 1, SHADERS.PS)
    MaterialProps.material:SetFloatByID(history.s_newflag, 1, SHADERS.PS)

    local group = self.entity:GetParent()
    local space_sl = group:GetChildById('normal_space')
    local space_str = group:GetChildById('normal_space_str')
    if history.s_newflag == 0 then
        space_sl:Deactivate()
        space_str:Deactivate()
    else
        space_sl:Activate()
        space_str:Activate()
    end

    MaterialProps:UpdateData(false)

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdNormalTex(self, ev)
    local group = self.entity:GetParent()
    local space_sl = group:GetChildById('normal_space')
    local space_str = group:GetChildById('normal_space_str')

    local is_tex = MaterialProps.material:GetFloatByID(1, SHADERS.PS)
    if is_tex == 0 then
        self:SetTexture(nil)
        space_sl:Deactivate()
        space_str:Deactivate()
    else
        local texture = MaterialProps.material:GetTextureNameByID(1, SHADERS.PS)
        self:SetTexture(texture)
        space_sl:Activate()
        space_str:Activate()
    end
    return true
end

function MaterialPropsCallback.SetNormalSpace(self, ev)
    local selected = self:GetSelected()

    local history = {
        s_oldval = 0,
        s_newval = 0,
        undo = function(self) 
                MaterialProps.material:SetFloatByID(self.s_oldval, 1, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloatByID(self.s_newval, 1, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Normals space"
    }

    history.s_oldval = MaterialProps.material:GetFloatByID(1, SHADERS.PS)
    history.s_newval = selected

    if history.s_oldval == history.s_newval then return true end

    MaterialProps.material:SetFloatByID(history.s_newval, 1, SHADERS.PS)

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdNormalSpace(self, ev)
    local space = MaterialProps.material:GetFloatByID(1, SHADERS.PS)
    if space == 2 then
        self:SetSelected(2)
    elseif space == 1 then
        self:SetSelected(1)
    else
        self:SetSelected(1)
        self.entity:Deactivate()
    end
    return true
end

-- ROUGHNESS
function MaterialPropsCallback.SetRoughnessType(self, ev)
    
    return true
end

function MaterialPropsCallback.UpdRoughnessType(self, ev)
    self.entity:Deactivate()
    return true
end

function MaterialPropsCallback.SetRoughnessTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        s_oldflag = 0,
        s_newflag = 0,
        undo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_oldval, 2, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_oldflag, 2, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_newval, 2, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_newflag, 2, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Microfacets texture"
    }

    history.s_oldval = MaterialProps.material:GetTextureNameByID(2, SHADERS.PS)
    history.s_oldflag = MaterialProps.material:GetFloatByID(2, SHADERS.PS)

    history.s_newval = texture
    history.s_newflag = texture:len() == 0 and 0 or (history.s_oldflag == 0 and 1 or history.s_oldflag)

    MaterialProps.material:SetTextureNameByID(history.s_newval, 2, SHADERS.PS)
    MaterialProps.material:SetFloatByID(history.s_newflag, 2, SHADERS.PS)

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdRoughnessTex(self, ev)
    local is_tex = MaterialProps.material:GetFloatByID(2, SHADERS.PS)
    if is_tex == 0 then
        self:SetTexture(nil)
    else
        local texture = MaterialProps.material:GetTextureNameByID(2, SHADERS.PS)
        self:SetTexture(texture)
    end
    return true
end

function MaterialPropsCallback.SetAnisoRG(self, ev, aniso)

    local history = {
        s_val_changed = false,
        s_oldval = 0,
        s_newval = 0,
        s_rv_changed = false,
        s_oldrv = 0,
        s_newrv = 0,
        undo = function(self) 
                if self.s_val_changed then MaterialProps.material:SetFloatByID(self.s_oldval, 2, SHADERS.PS) end
                if self.s_rv_changed then MaterialProps.material:SetFloatByID(self.s_oldrv, 4, SHADERS.PS) end
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                if self.s_val_changed then MaterialProps.material:SetFloatByID(self.s_newval, 2, SHADERS.PS) end
                if self.s_rv_changed then MaterialProps.material:SetFloatByID(self.s_newrv, 4, SHADERS.PS) end
                MaterialProps:UpdateData(false)
            end,
        msg = "Anisotropic roughness"
    }

    history.s_oldval = MaterialProps.material:GetFloatByID(2, SHADERS.PS)
    if history.s_oldval == 1 and aniso then 
        history.s_val_changed = true
        history.s_newval = 2
        MaterialProps.material:SetFloatByID(history.s_newval, 2, SHADERS.PS)

    elseif history.s_oldval == 2 and not aniso then 
        history.s_val_changed = true
        history.s_newval = 1
        MaterialProps.material:SetFloatByID(history.s_newval, 2, SHADERS.PS) 
    end
    
    local group = self.entity:GetParent()
    local v_str = group:GetChildById('roughness_v_str')
    local v_slider = group:GetChildById('roughness_v')
    if aniso then 
        v_slider:Activate()
        v_str:Activate()
    else
        v_slider:Deactivate()
        v_str:Deactivate()

        history.s_rv_changed = true
        history.s_oldrv = MaterialProps.material:GetFloatByID(4, SHADERS.PS)
        history.s_newrv = MaterialProps.material:GetFloatByID(3, SHADERS.PS)
        v_slider:GetInherited():SetValue(history.s_newrv)
        MaterialProps.material:SetFloatByID(history.s_newrv, 4, SHADERS.PS)
    end

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdAnisoRG(self, ev)
    local aniso = MaterialProps.material:GetFloatByID(2, SHADERS.PS)
    local rU = MaterialProps.material:GetFloatByID(3, SHADERS.PS)
    local rV = MaterialProps.material:GetFloatByID(4, SHADERS.PS)

    local group = self.entity:GetParent()
    local v_str = group:GetChildById('roughness_v_str')
    local v_slider = group:GetChildById('roughness_v')
    if aniso == 2 or rU ~= rV then
        self:SetCheck(true)
        v_slider:Activate()
        v_str:Activate()
    else
        self:SetCheck(false)
        v_slider:Deactivate()
        v_str:Deactivate()
    end
    return true
end

function MaterialPropsCallback.StartRoughU(self)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        s_sync_r = false,
        undo = function(self) 
                MaterialProps.material:SetFloatByID(self.s_oldval, 3, SHADERS.PS)
                if self.s_sync_r then
                    MaterialProps.material:SetFloatByID(self.s_oldval, 4, SHADERS.PS)
                end
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloatByID(self.s_newval, 3, SHADERS.PS)
                if self.s_sync_r then
                    MaterialProps.material:SetFloatByID(self.s_newval, 4, SHADERS.PS)
                end
                MaterialProps:UpdateData(false)
            end,
        msg = "Microfacets U value"
    }

    self.history.s_oldval = MaterialProps.material:GetFloatByID(3, SHADERS.PS)
    local val = self:GetValue()
    MaterialProps.material:SetFloatByID(val, 3, SHADERS.PS)

    local rough_v = self.entity:GetParent():GetChildById('roughness_v'):GetInherited()
    if not rough_v.entity:IsActivated() then
        self.history.s_sync_r = true
        rough_v:SetValue(val)
        MaterialProps.material:SetFloatByID(val, 4, SHADERS.PS)
    end

    return true
end

function MaterialPropsCallback.DragRoughU(self)
    local val = self:GetValue()
    MaterialProps.material:SetFloatByID(val, 3, SHADERS.PS)
    if self.history.s_sync_r then
        local rough_v = self.entity:GetParent():GetChildById('roughness_v'):GetInherited()
        MaterialProps.material:SetFloatByID(val, 4, SHADERS.PS)
        rough_v:SetValue(val)
    end
    return true
end

function MaterialPropsCallback.EndRoughU(self)
    self.history.s_newval = self:GetValue()
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    MaterialProps.material:SetFloatByID(self.history.s_newval, 3, SHADERS.PS)
    if self.history.s_sync_r then
        local rough_v = self.entity:GetParent():GetChildById('roughness_v'):GetInherited()
        MaterialProps.material:SetFloatByID(self.history.s_newval, 4, SHADERS.PS)
        rough_v:SetValue(self.history.s_newval)
    end
    History:Push(self.history)
    return true
end

function MaterialPropsCallback.UpdRoughU(self)
    self:SetValue(MaterialProps.material:GetFloatByID(3, SHADERS.PS))
    return true
end

-- REFLECTIVITY
function MaterialPropsCallback.SetReflectivityType(self, ev)
    local selected = self:GetSelected()

    local history = {
        s_oldval = 0,
        s_newval = 0,
        undo = function(self) 
                MaterialProps.material:SetFloatByID(self.s_oldval, 5, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloatByID(self.s_newval, 5, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Reflectivity parametrization"
    }

    history.s_oldval = MaterialProps.material:GetFloatByID(5, SHADERS.PS)
    if history.s_oldval == 0 or history.s_oldval == -1 then
        history.s_newval = selected == 1 and 0 or -1
    else
        history.s_newval = selected == 1 and 1 or -2
    end

    if history.s_oldval == history.s_newval then return true end

    MaterialProps.material:SetFloatByID(history.s_newval, 5, SHADERS.PS)

    MaterialProps:UpdateData(false)
    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdReflectivityType(self, ev)
    local input_type = MaterialProps.material:GetFloatByID(5, SHADERS.PS)

    local group = self.entity:GetParent()
    local refl_str = group:GetChildById('reflectivity_header'):GetInherited()
    local refl_metal = group:GetChildById('metalness_slider')
    local refl_spec = group:GetChildById('specular_picker')

    if input_type == 0 or input_type == 1 then
        self:SetSelected(1)
        refl_str:SetString("Metalness")
        refl_metal.enable = true
        refl_spec.enable = false
    else
        self:SetSelected(2)
        refl_str:SetString("Specular")
        refl_metal.enable = false
        refl_spec.enable = true      
    end
    return true
end

function MaterialPropsCallback.SetReflectivityTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        s_oldflag = 0,
        s_newflag = 0,
        undo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_oldval, 3, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_oldflag, 5, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_newval, 3, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_newflag, 5, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Reflectivity texture"
    }

    history.s_oldval = MaterialProps.material:GetTextureNameByID(3, SHADERS.PS)
    history.s_oldflag = MaterialProps.material:GetFloatByID(5, SHADERS.PS)

    history.s_newval = texture
    if history.s_oldflag < 0 then
        history.s_newflag = texture:len() == 0 and -1 or -2
    else
        history.s_newflag = texture:len() == 0 and 0 or 1
    end

    local group = self.entity:GetParent()
    local refl_str = group:GetChildById('reflectivity_header')
    local refl_metal = group:GetChildById('metalness_slider')
    local refl_spec = group:GetChildById('specular_picker')
    if texture:len() == 0 then
        refl_str:Activate()
        refl_metal:Activate()
        refl_spec:Activate()
    else
        refl_str:Deactivate()
        refl_metal:Deactivate()
        refl_spec:Deactivate()
    end

    MaterialProps.material:SetTextureNameByID(history.s_newval, 3, SHADERS.PS)
    MaterialProps.material:SetFloatByID(history.s_newflag, 5, SHADERS.PS)

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdReflectivityTex(self, ev)
    local is_tex = MaterialProps.material:GetFloatByID(5, SHADERS.PS)

    local group = self.entity:GetParent()
    local refl_str = group:GetChildById('reflectivity_header')
    local refl_metal = group:GetChildById('metalness_slider')
    local refl_spec = group:GetChildById('specular_picker')

    if is_tex == 0 or is_tex == -1 then
        self:SetTexture(nil)
        refl_str:Activate()
        refl_metal:Activate()
        refl_spec:Activate()
    else
        local texture = MaterialProps.material:GetTextureNameByID(3, SHADERS.PS)
        self:SetTexture(texture)
        refl_str:Deactivate()
        refl_metal:Deactivate()
        refl_spec:Deactivate()
    end
    return true
end

function MaterialPropsCallback.StartMetalness(self, ev)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        undo = function(self) 
                MaterialProps.material:SetVectorByID(Vector4(self.s_oldval,0,0,0), 1, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetVectorByID(Vector4(self.s_newval,0,0,0), 1, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Metalness value"
    }

    self.history.s_oldval = MaterialProps.material:GetVectorByID(1, SHADERS.PS).x
    MaterialProps.material:SetVectorByID(Vector4(self:GetValue(),0,0,0), 1, SHADERS.PS)
    return true
end

function MaterialPropsCallback.DragMetalness(self, ev)
    MaterialProps.material:SetVectorByID(Vector4(self:GetValue(),0,0,0), 1, SHADERS.PS)
    return true
end

function MaterialPropsCallback.EndMetalness(self, ev)
    self.history.s_newval = self:GetValue()
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    MaterialProps.material:SetVectorByID(Vector4(self.history.s_newval,0,0,0), 1, SHADERS.PS)
    History:Push(self.history)
    return true
end

function MaterialPropsCallback.UpdMetalness(self, ev)
    local metalness = MaterialProps.material:GetVectorByID(1, SHADERS.PS)
    self:SetValue(metalness.x)
    return true
end

-- AO
function MaterialPropsCallback.SetAOTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        s_oldflag = 0,
        s_newflag = 0,
        undo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_oldval, 4, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_oldflag, 8, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_newval, 4, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_newflag, 8, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "AO texture"
    }

    history.s_oldval = MaterialProps.material:GetTextureNameByID(4, SHADERS.PS)
    history.s_oldflag = MaterialProps.material:GetFloatByID(8, SHADERS.PS)

    history.s_newval = texture
    history.s_newflag = texture:len() == 0 and 0 or 1

    MaterialProps.material:SetTextureNameByID(history.s_newval, 4, SHADERS.PS)
    MaterialProps.material:SetFloatByID(history.s_newflag, 8, SHADERS.PS)

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdAOTex(self, ev)
    local is_tex = MaterialProps.material:GetFloatByID(8, SHADERS.PS)
    if is_tex == 0 then
        self:SetTexture(nil)
    else
        local texture = MaterialProps.material:GetTextureNameByID(4, SHADERS.PS)
        self:SetTexture(texture)
    end
    return true
end

-- EMISSIVE
function MaterialPropsCallback.SetEmissiveTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        s_oldflag = 0,
        s_newflag = 0,
        undo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_oldval, 6, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_oldflag, 7, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_newval, 6, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_newflag, 7, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Emissive texture"
    }

    history.s_oldval = MaterialProps.material:GetTextureNameByID(6, SHADERS.PS)
    history.s_oldflag = MaterialProps.material:GetFloatByID(7, SHADERS.PS)

    history.s_newval = texture
    history.s_newflag = texture:len() == 0 and 0 or 1

    MaterialProps.material:SetTextureNameByID(history.s_newval, 6, SHADERS.PS)
    MaterialProps.material:SetFloatByID(history.s_newflag, 7, SHADERS.PS)

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdEmissiveTex(self, ev)
    local is_tex = MaterialProps.material:GetFloatByID(7, SHADERS.PS)
    if is_tex == 0 then
        self:SetTexture(nil)
    else
        local texture = MaterialProps.material:GetTextureNameByID(6, SHADERS.PS)
        self:SetTexture(texture)
    end
    return true
end

function MaterialPropsCallback.StartEmissive(self)
    self.history = {
        s_oldval = Vector4(0,0,0,0),
        s_newval = Vector4(0,0,0,0),
        undo = function(self) 
                MaterialProps.material:SetVectorByID(self.s_oldval, 2, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetVectorByID(self.s_newval, 2, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Emissive value"
    }

    self.history.s_oldval = MaterialProps.material:GetVectorByID(2, SHADERS.PS)

    local color_btn = self.entity:GetParent():GetChildById('emissive_color'):GetInherited()
    local color = CMath.ColorDenormalize(color_btn.background.color, self:GetValue())

    MaterialProps.material:SetVectorByID(color, 2, SHADERS.PS)
    return true
end

function MaterialPropsCallback.DragEmissive(self)
    local color_btn = self.entity:GetParent():GetChildById('emissive_color'):GetInherited()
    local color = CMath.ColorDenormalize(color_btn.background.color, self:GetValue())
    MaterialProps.material:SetVectorByID(color, 2, SHADERS.PS)
    return true
end

function MaterialPropsCallback.EndEmissive(self)
    local color_btn = self.entity:GetParent():GetChildById('emissive_color'):GetInherited()
    self.history.s_newval = CMath.ColorDenormalize(color_btn.background.color, self:GetValue())

    if CMath.IsNearlyEq(self.history.s_oldval.x, self.history.s_newval.x, 0.001) and
        CMath.IsNearlyEq(self.history.s_oldval.y, self.history.s_newval.y, 0.001) and
        CMath.IsNearlyEq(self.history.s_oldval.z, self.history.s_newval.z, 0.001) then return true end

    MaterialProps.material:SetVectorByID(self.history.s_newval, 2, SHADERS.PS)
    History:Push(self.history)
    return true
end

function MaterialPropsCallback.UpdEmissive(self)
    local color, val = CMath.ColorNormalize( MaterialProps.material:GetVectorByID(2, SHADERS.PS) )
    self:SetValue(val)
    return true
end

function MaterialPropsCallback.StartEmissivePicking(self)
    if self.picker then 
        self.picker = false
        return true
    else
        ColorPicker:Show(self, self.background.color, true)
        self.picker = true
    end

    self.history = {
        s_oldval = Vector4(0,0,0,0),
        s_newval = Vector4(0,0,0,0),
        undo = function(self) 
                MaterialProps.material:SetVectorByID(self.s_oldval, 2, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetVectorByID(self.s_newval, 2, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Emissive color"
    }

    self.history.s_oldval = MaterialProps.material:GetVectorByID(2, SHADERS.PS)
    return true
end

function MaterialPropsCallback.EmissivePicking(self)
    local color = ColorPicker:GetColor()
    local intensity_sl = self.entity:GetParent():GetChildById('emissive_intensity'):GetInherited()
    MaterialProps.material:SetVectorByID(CMath.ColorDenormalize(color, intensity_sl:GetValue()), 2, SHADERS.PS)
    return true
end

function MaterialPropsCallback.EmissivePicked(self)
    if not ColorPicker:IsChanged() then return end
    
    local intensity_sl = self.entity:GetParent():GetChildById('emissive_intensity'):GetInherited()
    self.history.s_newval = CMath.ColorDenormalize(self.background.color, intensity_sl:GetValue())

    History:Push(self.history)
    return true
end

function MaterialPropsCallback.UpdEmissiveColor(self)
    local val = CMath.ColorNormalize( MaterialProps.material:GetVectorByID(2, SHADERS.PS) )
    self.background.color = Vector4(val.x, val.y, val.z, 1)
    self.background.color_hover = self.background.color
    self.background.color_press = self.background.color
    self:UpdateProps()
    return true
end

-- SUBSURFACE
function MaterialPropsCallback.SetSubsurfaceTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        s_oldflag = 0,
        s_newflag = 0,
        undo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_oldval, 7, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_oldflag, 9, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_newval, 7, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_newflag, 9, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Subsurface texture"
    }

    history.s_oldval = MaterialProps.material:GetTextureNameByID(7, SHADERS.PS)
    history.s_oldflag = MaterialProps.material:GetFloatByID(9, SHADERS.PS)

    history.s_newval = texture
    history.s_newflag = texture:len() == 0 and 1 or 2

    MaterialProps.material:SetTextureNameByID(history.s_newval, 7, SHADERS.PS)
    MaterialProps.material:SetFloatByID(history.s_newflag, 9, SHADERS.PS)

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdSubsurfaceTex(self, ev)
    local is_tex = MaterialProps.material:GetFloatByID(9, SHADERS.PS)
    if is_tex <= 1 then
        self:SetTexture(nil)
    else
        local texture = MaterialProps.material:GetTextureNameByID(7, SHADERS.PS)
        self:SetTexture(texture)
    end
    return true
end

function MaterialPropsCallback.SetThicknessTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        s_oldflag = 0,
        s_newflag = 0,
        undo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_oldval, 8, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_oldflag, 10, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_newval, 8, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_newflag, 10, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Thickness texture"
    }

    history.s_oldval = MaterialProps.material:GetTextureNameByID(8, SHADERS.PS)
    history.s_oldflag = MaterialProps.material:GetFloatByID(10, SHADERS.PS)

    history.s_newval = texture
    history.s_newflag = texture:len() == 0 and 0 or 1

    MaterialProps.material:SetTextureNameByID(history.s_newval, 8, SHADERS.PS)
    MaterialProps.material:SetFloatByID(history.s_newflag, 10, SHADERS.PS)

    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdThicknessTex(self, ev)
    local is_tex = MaterialProps.material:GetFloatByID(10, SHADERS.PS)
    if is_tex == 0 then
        self:SetTexture(nil)
    else
        local texture = MaterialProps.material:GetTextureNameByID(8, SHADERS.PS)
        self:SetTexture(texture)
    end
    return true
end

function MaterialPropsCallback.StartThickness(self, ev)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        undo = function(self) 
                local vect = MaterialProps.material:GetVectorByID(3, SHADERS.PS)
                vect.w = self.s_oldval
                MaterialProps.material:SetVectorByID(vect, 3, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                local vect = MaterialProps.material:GetVectorByID(3, SHADERS.PS)
                vect.w = self.s_newval
                MaterialProps.material:SetVectorByID(vect, 3, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Thickness value"
    }

    local vect = MaterialProps.material:GetVectorByID(3, SHADERS.PS)
    self.history.s_oldval = vect.w
    vect.w = self:GetValue()
    MaterialProps.material:SetVectorByID(vect, 3, SHADERS.PS)
    return true
end

function MaterialPropsCallback.DragThickness(self, ev)
    local vect = MaterialProps.material:GetVectorByID(3, SHADERS.PS)
    vect.w = self:GetValue()
    MaterialProps.material:SetVectorByID(vect, 3, SHADERS.PS)
    return true
end

function MaterialPropsCallback.EndThickness(self, ev)
    self.history.s_newval = self:GetValue()
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    local vect = MaterialProps.material:GetVectorByID(3, SHADERS.PS)
    vect.w = self.history.s_newval
    MaterialProps.material:SetVectorByID(vect, 3, SHADERS.PS)
    History:Push(self.history)
    return true
end

function MaterialPropsCallback.UpdThickness(self, ev)
    local thickness = MaterialProps.material:GetVectorByID(3, SHADERS.PS)
    self:SetValue(thickness.w)
    return true
end

-- ALPHATEST
function MaterialPropsCallback.SetAlphatestTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        s_oldflag = 0,
        s_newflag = 0,
        undo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_oldval, 5, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_oldflag, 6, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureNameByID(self.s_newval, 5, SHADERS.PS)
                MaterialProps.material:SetFloatByID(self.s_newflag, 6, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Alphatest texture"
    }

    history.s_oldval = MaterialProps.material:GetTextureNameByID(5, SHADERS.PS)
    history.s_oldflag = MaterialProps.material:GetFloatByID(6, SHADERS.PS)

    history.s_newval = texture

    if texture:len() == 0 then
        history.s_newflag = 0 
    else
        history.s_newflag = history.s_oldflag
    end

    MaterialProps.material:SetTextureNameByID(history.s_newval, 5, SHADERS.PS)
    MaterialProps.material:SetFloatByID(history.s_newflag, 6, SHADERS.PS)

    MaterialProps:UpdateData(false)
    History:Push(history)
    return true
end

function MaterialPropsCallback.UpdAlphatestTex(self, ev)
    local texture = MaterialProps.material:GetTextureNameByID(5, SHADERS.PS)

    local group = self.entity:GetParent()
    local alpha_str = group:GetChildById('alpha_ref_str')
    local alpha_ref = group:GetChildById('alpha_ref')

    if texture:find("tech/error_albedo") ~= nil then
        self:SetTexture(nil)
        alpha_str:Deactivate()
        alpha_ref:Deactivate()
    else
        self:SetTexture(texture)
        alpha_str:Activate()
        alpha_ref:Activate()
    end
    return true
end