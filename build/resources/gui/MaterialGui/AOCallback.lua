if not AOCallback then AOCallback = {} end

function AOCallback.UpdAOTex(self, ev)
    MaterialProps.UpdTexture(self, "aoTexture", "hasAOTexture")

    local hasTex = MaterialProps.material:GetFloat("hasAOTexture", SHADERS.PS)

    local group = self.entity:GetParent()
    local power_str = group:GetChildById('power_str')
    local power_sld = group:GetChildById('power_sld')

    if hasTex == 0 then
        power_str:Deactivate()
        power_sld:Deactivate()
    else
        power_str:Activate()
        power_sld:Activate()
    end
    return true
end