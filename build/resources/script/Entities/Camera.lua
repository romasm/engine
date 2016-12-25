if not EntityTypes.Camera then EntityTypes.Camera = class(EntityTypes.BaseEntity) end

function EntityTypes.Camera:init(world, ent)
    if not self._base.init(self, world, ent) then return end

    self.world:SetEntityType(self.ent, "Camera")

    self.transformSys:AddComponent(self.ent)

    self.cameraSys = self.world.camera

    self.cameraSys:AddComponent(self.ent)

    self.cameraSys:SetFov(self.ent, 1.0)
    self.cameraSys:SetAspect(self.ent, 1.0)
    self.cameraSys:SetFar(self.ent, 10000.0)
    self.cameraSys:SetNear(self.ent, 0.1)

    -- editor: todo
end

function EntityTypes.Camera:SetFov(fov)
    self.cameraSys:SetFov(self.ent, fov)
end

function EntityTypes.Camera:SetFar(d)
    self.cameraSys:SetFar(self.ent, d)
end

function EntityTypes.Camera:SetNear(d)
    self.cameraSys:SetNear(self.ent, d)
end

function EntityTypes.Camera:GetFov()
    return self.cameraSys:GetFov(self.ent)
end

function EntityTypes.Camera:GetFar()
    return self.cameraSys:GetFar(self.ent)
end

function EntityTypes.Camera:GetNear()
    return self.cameraSys:GetNear(self.ent)
end

function EntityTypes.Camera:GetAspect()
    return self.cameraSys:GetAspect(self.ent)
end

function EntityTypes.Camera:GetUp()
    return self.cameraSys:GetUp(self.ent)
end

function EntityTypes.Camera:GetLookDir()
    return self.cameraSys:GetLookDir(self.ent)
end

function EntityTypes.Camera:GetLookTangent()
    return self.cameraSys:GetLookTangent(self.ent)
end

function EntityTypes.Camera:Activate(scene)
    self.cameraSys:Activate(self.ent, scene)
end

function EntityTypes.Camera:Deactivate(scene)
    self.cameraSys:Deactivate(self.ent, scene)
end

function EntityTypes.Camera:IsActive()
    return self.cameraSys:IsActive(self.ent)
end