if not EntityTypes.GlobalLight then EntityTypes.GlobalLight = class(EntityTypes.BaseEntity) end

function EntityTypes.GlobalLight:init(world, ent)
    if not self._base.init(self, world, ent) then return end
    
    self.world:SetEntityType(self.ent, "GlobalLight")

    self.transformSys:AddComponent(self.ent)
    
    self.globalLightSys = self.world.globalLight

    self.globalLightSys:AddComponent(self.ent)

    self.globalLightSys:SetColor(self.ent, Vector3(1,1,1))
    self.globalLightSys:SetBrightness(self.ent, 0.2)
    self.globalLightSys:SetArea(self.ent, 0.0094)

    -- editor
    self.world.visibility:AddComponent(self.ent)

    if not self.world.staticMesh:AddComponentMesh(self.ent, "../resources/meshes/editor/pointlight.stm") then
        error("Cant init EntityTypes.GlobalLight")
        self:Kill()
    else
        self.world.staticMesh:SetEditor(self.ent, true)
        self.world.staticMesh:SetMaterial(self.ent, 0, "../resources/meshes/editor/pointlight.mtb")
    end
end

function EntityTypes.GlobalLight:SetColor(r, g, b)
    self.globalLightSys:SetColor(self.ent, Vector3(r,g,b))
end

function EntityTypes.GlobalLight:GetColor()
    return self.globalLightSys:GetColor(self.ent)
end

function EntityTypes.GlobalLight:SetIlluminance(illuminance)
    local brightness = EntityTypes.GlobalLight.IlluminanceToBrightness(illuminance)
    self.globalLightSys:SetBrightness(self.ent, brightness)
end

function EntityTypes.GlobalLight:GetIlluminance()
    local brightness = self.globalLightSys:GetBrightness(self.ent)
    return EntityTypes.GlobalLight.BrightnessToIlluminance(brightness)
end

function EntityTypes.GlobalLight:SetSolidAngle(angle)
    self.globalLightSys:SetArea(self.ent, angle)
end

function EntityTypes.GlobalLight:GetSolidAngle()
    return self.globalLightSys:GetArea(self.ent)
end


-- STATIC
function EntityTypes.GlobalLight.IlluminanceToBrightness(illuminance)
    return illuminance / 10000
end

function EntityTypes.GlobalLight.BrightnessToIlluminance(brightness)
    return brightness * 10000
end