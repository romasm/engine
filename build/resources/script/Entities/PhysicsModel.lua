if not EntityTypes.PhysicsModel then EntityTypes.PhysicsModel = class(EntityTypes.Mesh) end

function EntityTypes.PhysicsModel:init(world, ent)
    if not self:base(EntityTypes.PhysicsModel).init(self, world, ent) then return false end
    
    self.world:SetEntityType(self.ent, "PhysicsModel")
    
    self.physicsSys = self.world.physics

    self.physicsSys:AddComponent(self.ent)
    
    -- temp
    local bb_size = self.world.visibility:GetBoxSizeL(self.ent)
    local bb_pos = self.world.visibility:GetBoxCenterL(self.ent)
    local bb_mass = bb_size.x * bb_size.y * bb_size.z * 8 * 100
    
    self.physicsSys:AddBoxCollider(self.ent, bb_pos, Quaternion.Identity, bb_size)
    --self.physicsSys:SetType(self.ent, PHYSICS_TYPES.DYNAMIC)
    self.physicsSys:SetMass(self.ent, bb_mass)
    self.physicsSys:UpdateState(self.ent)
    
    return true
end