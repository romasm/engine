if not NormalCallback then NormalCallback = {} end

function NormalCallback.UpdNormalTex(self, ev)
    MaterialProps.UpdTexture(self, "normalTexture", "hasNormalTexture")

    local hasTex = MaterialProps.material:GetFloat("hasNormalTexture", SHADERS.PS)

    local group = self.entity:GetParent()
    local space_sl = group:GetChildById('normal_space')
    --local space_str = group:GetChildById('normal_space_str')
    local invert_y = group:GetChildById('normal_y')

    if hasTex == 0 then
        space_sl:Deactivate()
        --space_str:Deactivate()
        invert_y:Deactivate()
    else
        space_sl:Activate()
        --space_str:Activate()
        invert_y:Activate()
    end
    return true
end

function NormalCallback.SetInvertY(self, checked)
    local history = {
        s_oldval = false,
        s_newval = false,
        undo = function(self) 
                MaterialProps.material:SetFloat(self.s_oldval and 1.0 or 0.0, "normalMapInvertY", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloat(self.s_newval and 1.0 or 0.0, "normalMapInvertY", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Microfacets type"
    }

    history.s_oldval = (MaterialProps.material:GetFloat("normalMapInvertY", SHADERS.PS) > 0.0)
    history.s_newval = checked

    if history.s_oldval == history.s_newval then return true end

    MaterialProps.material:SetFloat(checked and 1.0 or 0.0, "normalMapInvertY", SHADERS.PS)

    History:Push(history)
    return true
end

function NormalCallback.UpdInvertY(self, ev)
    local checked = (MaterialProps.material:GetFloat("normalMapInvertY", SHADERS.PS) > 0.0)
    self:SetCheck( checked )
    return true
end