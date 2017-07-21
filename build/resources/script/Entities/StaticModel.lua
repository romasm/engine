if not EntityTypes.StaticModel then EntityTypes.StaticModel = class(EntityTypes.Mesh) end

function EntityTypes.StaticModel:init(world, ent)
    if not self:base(EntityTypes.StaticModel).init(self, world, ent) then return false end
    
    self.world.physics:AddComponent(self.ent)
    self.world.collision:AddComponent(self.ent)

    self.world:SetEntityType(self.ent, "StaticModel")
    
    return true
end

function EntityTypes.StaticModel:SetMesh(mesh)
    if not self:base(EntityTypes.StaticModel).SetMesh(self, mesh) then return false end
    
    -- temp
    local bb_size = self.world.visibility:GetBoxSizeL(self.ent)
    local bb_pos = self.world.visibility:GetBoxCenterL(self.ent)
    
    self.world.collision:AddBoxCollider(self.ent, bb_pos, Quaternion.Identity, 1.0, bb_size, 0)

    local physicsSys = self.world.physics
    physicsSys:SetType(self.ent, PHYSICS_TYPES.STATIC)
    physicsSys:SetBounciness(self.ent, 0.0)
    physicsSys:SetFriction(self.ent, 1.0)
    physicsSys:SetActive(self.ent, true)

    return true
end