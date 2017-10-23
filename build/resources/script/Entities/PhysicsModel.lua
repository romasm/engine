if not EntityTypes.PhysicsModel then EntityTypes.PhysicsModel = class(EntityTypes.Mesh) end

function EntityTypes.PhysicsModel:init(world, ent)
    if not self:base(EntityTypes.PhysicsModel).init(self, world, ent) then return false end
    
    self.world:SetEntityType(self.ent, "PhysicsModel")
    
    self.physicsSys = self.world.physics
    self.physicsSys:AddComponent(self.ent)
    self.physicsSys:SetType(self.ent, PHYSICS_TYPES.STATIC)
    self.physicsSys:SetRestitution(self.ent, 0.1)
    self.physicsSys:SetFriction(self.ent, 0.8)
    self.physicsSys:SetMass(self.ent, 100.0)
    self.physicsSys:UpdateState(self.ent)

    return true
end

function EntityTypes.PhysicsModel:ApplyCentralImpulse(impulse)
    self.physicsSys:ApplyCentralImpulse(self.ent, impulse)
end

-- temp 
function EntityTypes.PhysicsModel:SetMesh(mesh)
    if not self:base(EntityTypes.PhysicsModel).SetMeshAndCallback(self, mesh, EntityTypes.PhysicsModel.OnLoad) then return false end
    
    self.physicsSys:ClearCollision(self.ent)
    self.physicsSys:SetConvexHullsCollider(self.ent, mesh)
    
    return true
end

function EntityTypes.PhysicsModel.OnLoad(world, ent, id, status)
    if not status then return end
    
    print("TEST LUA CALLBACK")
        
    local bb_size = world.visibility:GetBoxSizeL(ent)
    local bb_mass = bb_size.x * bb_size.y * bb_size.z * 8 * 100
    world.physics:SetMass(ent, bb_mass)
    world.physics:UpdateState(ent)
    
    return true
end