if not EntityTypes.PhysicsModel then EntityTypes.PhysicsModel = class(EntityTypes.StaticModel) end

function EntityTypes.PhysicsModel:init(world, ent)
    if not self:base(EntityTypes.PhysicsModel).init(self, world, ent) then return false end
    
    self.physicsSys = self.world.physics
    self.physicsSys:AddComponent(self.ent)

    self.world:SetEntityType(self.ent, "PhysicsModel")

    -- temp
    local bb_size = self.world.visibility:GetBoxSizeL(self.ent)
    local bb_pos = self.world.visibility:GetBoxCenterL(self.ent)
    local bb_mass = bb_size.x * bb_size.y * bb_size.z * 80
    
    self.physicsSys:AddBoxShape(self.ent, bb_pos, Quaternion.Identity, bb_mass, bb_size)
    self.physicsSys:SetActive(self.ent, true)

    return true
end

function EntityTypes.PhysicsModel:SetType(t)
    self.physicsSys:SetType(self.ent, t)
end

function EntityTypes.PhysicsModel:SetRollingResistance(f)
    self.physicsSys:SetRollingResistance(self.ent, f)
end