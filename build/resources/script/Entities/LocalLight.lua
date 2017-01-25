if not EntityTypes.LocalLight then EntityTypes.LocalLight = class(EntityTypes.BaseEntity) end

function EntityTypes.LocalLight:init(world, ent)
    if not self._base.init(self, world, ent) then return end

    self.world:SetEntityType(self.ent, "LocalLight")

    self.transformSys:AddComponent(self.ent)
    self.world.earlyVisibility:AddComponent(self.ent)

    self.lightSys = self.world.light

    self.lightSys:AddComponent(self.ent)

    self.lightSys:SetType(self.ent, 0)
    self.lightSys:SetColor(self.ent, Vector3(1,1,1))
    self.lightSys:SetBrightness(self.ent, 10)
    self.lightSys:SetRange(self.ent, 10)
    self.lightSys:UpdateLightProps(self.ent)

    -- editor
    self.world.visibility:AddComponent(self.ent)

    if not self.world.staticMesh:AddComponentMesh(self.ent, "../content/statics/editor/pointlight.stm") then
        error("Cant init EntityTypes.LocalLight")
        self:Kill()
    else
        self.world.staticMesh:SetEditor(self.ent, true)
        self.world.staticMesh:SetMaterial(self.ent, 0, "../content/materials/editor/pointlight.mtb")
    end
end

function EntityTypes.LocalLight:SetColor(r, g, b)
    self.lightSys:SetColor(self.ent, Vector3(r,g,b))
    self.lightSys:UpdateLightProps(self.ent)
end

function EntityTypes.LocalLight:GetColor()
    return self.lightSys:GetColor(self.ent)
end

function EntityTypes.LocalLight:SetLumPower(power)
    local intensity = EntityTypes.LocalLight.LumPowerToLumIntensity(power, self.ent, self.world)
    self.lightSys:SetBrightness(self.ent, intensity)
    self.lightSys:UpdateLightProps(self.ent)
end

function EntityTypes.LocalLight:GetLumPower()
    local intensity = self.lightSys:GetBrightness(self.ent)
    return EntityTypes.LocalLight.LumIntensityToLumPower(intensity, self.ent, self.world)
end

function EntityTypes.LocalLight:SetRange(r)
    self.lightSys:SetRange(self.ent, r)
    self.lightSys:UpdateLightProps(self.ent)
end

function EntityTypes.LocalLight:GetRange()
    return self.lightSys:GetRange(self.ent)
end

function EntityTypes.LocalLight:SetCone(angleIn, angleOut)
    self.lightSys:SetConeIn(self.ent, EntityTypes.LocalLight.AngleToCone(angleIn))
    self.lightSys:SetConeOut(self.ent, EntityTypes.LocalLight.AngleToCone(angleOut))
    self.lightSys:UpdateLightProps(self.ent)
end

function EntityTypes.LocalLight:GetCone()
    local coneIn = self.lightSys:ConeToAngle(EntityTypes.LocalLight.GetConeIn(self.ent))
    local coneOut = self.lightSys:ConeToAngle(EntityTypes.LocalLight.GetConeOut(self.ent))
    return Vector2(coneIn, coneOut)
end

function EntityTypes.LocalLight:SetArea(x, y)
    self.lightSys:SetAreaX(self.ent, x)
    self.lightSys:SetAreaY(self.ent, y)
    self.lightSys:UpdateLightProps(self.ent)
end

function EntityTypes.LocalLight:GetArea()
    local x = self.lightSys:GetAreaX(self.ent)
    local y = self.lightSys:GetAreaY(self.ent)
    return Vector2(x, y)
end


-- STATIC
function EntityTypes.LocalLight.AngleToCone(angle)
    return angle * math.pi / 360
end

function EntityTypes.LocalLight.ConeToAngle(cone)
    return cone * 360 / math.pi
end

function EntityTypes.LocalLight.LumPowerToLumIntensity(power, ent, world) -- TODO: move intensity calc code in c++ from lua
    local lt = world.light:GetType(ent) + 1
    local areax = world.light:GetAreaX(ent)
    local areay = world.light:GetAreaY(ent)
    
    local basicIntensity = power / (4 * math.pi)

    if lt == LIGHT_TYPE.POINT then return basicIntensity
    elseif lt == LIGHT_TYPE.SPOT then return basicIntensity -- div 4 for power sync
    elseif lt == LIGHT_TYPE.SPHERE then return basicIntensity / (areax * areax * math.pi)
    elseif lt == LIGHT_TYPE.TUBE then return basicIntensity / ( 0.5 * math.pi * areax *(areay + 2 * areax) )
    elseif lt == LIGHT_TYPE.DISK then return basicIntensity / (areax * areax * math.pi)
    elseif lt == LIGHT_TYPE.RECT then return basicIntensity / (areax * areay)
    end
end

function EntityTypes.LocalLight.LumIntensityToLumPower(intensity, ent, world) -- TODO: move intensity calc code in c++ from lua
    local lt = world.light:GetType(ent) + 1
    local areax = world.light:GetAreaX(ent)
    local areay = world.light:GetAreaY(ent)

    local basicPower = intensity * (4 * math.pi)

    if lt == LIGHT_TYPE.POINT then return basicPower
    elseif lt == LIGHT_TYPE.SPOT then return basicPower
    elseif lt == LIGHT_TYPE.SPHERE then return basicPower * (areax * areax * math.pi)
    elseif lt == LIGHT_TYPE.TUBE then return basicPower * ( 0.5 * math.pi * areax *(areay + 2 * areax) )
    elseif lt == LIGHT_TYPE.DISK then return basicPower * (areax * areax * math.pi)
    elseif lt == LIGHT_TYPE.RECT then return basicPower * (areax * areay)
    end
end