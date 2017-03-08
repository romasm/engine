if not RoughnessCallback then RoughnessCallback = {} end

function RoughnessCallback.SetAnisoFlag(isAniso)
    MaterialProps.material:SetFloat(isAniso and 1.0 or 0.0, "roughnessAnisotropic", SHADERS.PS)
    if isAniso then 
        local rx = MaterialProps.material:GetFloat("roughnessX", SHADERS.PS)
        MaterialProps.material:SetFloat(rx, "roughnessY", SHADERS.PS) 
    end
end

function RoughnessCallback.SetRoughnessAniso(self, checked)
    local history = {
        s_oldval = false,
        s_newval = false,
        undo = function(self) 
                RoughnessCallback.SetAnisoFlag(self.s_oldval)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                RoughnessCallback.SetAnisoFlag(self.s_newval)
                MaterialProps:UpdateData(false)
            end,
        msg = "Microfacets type"
    }

    history.s_oldval = (MaterialProps.material:GetFloat("roughnessAnisotropic", SHADERS.PS) > 0.0)
    history.s_newval = checked

    if history.s_oldval == history.s_newval then return true end

    history:redo()

    History:Push(history)
    return true
end

function RoughnessCallback.UpdRoughnessAniso(self, ev)
    local checked = (MaterialProps.material:GetFloat("roughnessAnisotropic", SHADERS.PS) > 0.0)
    self:SetCheck( checked )

    local group = self.entity:GetParent()
    local v_str = group:GetChildById('roughness_v_str')
    local v_slider = group:GetChildById('roughness_v')

    v_slider.enable = checked
    v_str.enable = checked

    return true
end