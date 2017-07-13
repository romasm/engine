if not EntityTypes.PhysicsModel then EntityTypes.PhysicsModel = class(EntityTypes.StaticModel) end

function EntityTypes.PhysicsModel:init(world, ent)
    if not self:base(EntityTypes.PhysicsModel).init(self, world, ent) then return false end
    
    self.physicsSys = self.world.physics
    self.physicsSys:AddComponent(self.ent)

    self.world:SetEntityType(self.ent, "PhysicsModel")

    return true
end

function EntityTypes.PhysicsModel:SetType(t)
    self.physicsSys:SetType(self.ent, t)
end