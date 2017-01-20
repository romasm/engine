if not ReflectivityCallback then ReflectivityCallback = {} end

function ReflectivityCallback.UpdReflectivityType(self, ev)
    MaterialProps.UpdSelector(self, "isMetalPipeline")
    
    local isMetal = MaterialProps.material:GetFloat("isMetalPipeline", SHADERS.PS)

    local group = self.entity:GetParent()
    local refl_str = group:GetChildById('reflectivity_header'):GetInherited()
    local refl_metal = group:GetChildById('metalness_slider')
    local refl_spec = group:GetChildById('specular_picker')

    if isMetal > 0 then
        refl_str:SetString("Metalness")
        refl_metal.enable = true
        refl_spec.enable = false
    else
        refl_str:SetString("Specular")
        refl_metal.enable = false
        refl_spec.enable = true      
    end
    return true
end

function ReflectivityCallback.UpdReflectivityTex(self, ev)
    MaterialProps.UpdTexture(self, "reflectivityTexture", "hasReflectivityTexture")

    local hasTex = MaterialProps.material:GetFloat("hasReflectivityTexture", SHADERS.PS)
    
    local group = self.entity:GetParent()
    local refl_str = group:GetChildById('reflectivity_header')
    local refl_metal = group:GetChildById('metalness_slider')
    local refl_spec = group:GetChildById('specular_picker')

    if hasTex == 0 then
        refl_str:Activate()
        refl_metal:Activate()
        refl_spec:Activate()
    else
        refl_str:Deactivate()
        refl_metal:Deactivate()
        refl_spec:Deactivate()
    end
    return true
end