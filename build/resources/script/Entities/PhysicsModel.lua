if not EntityTypes.PhysicsModel then EntityTypes.PhysicsModel = class(EntityTypes.Mesh) end

function EntityTypes.PhysicsModel:init(world, ent)
    if not self:base(EntityTypes.PhysicsModel).init(self, world, ent) then return false end
    
    self.world:SetEntityType(self.ent, "PhysicsModel")
    
    self.physicsSys = self.world.physics
    self.physicsSys:AddComponent(self.ent)
    self.physicsSys:SetMass(self.ent, 100.0)

    return true
end

function EntityTypes.PhysicsModel:ApplyCentralImpulse(impulse)
    self.physicsSys:ApplyCentralImpulse(self.ent, impulse)
end

-- temp 
function EntityTypes.PhysicsModel:SetMesh(mesh)
    if not self:base(EntityTypes.PhysicsModel).SetMeshAndCallback(self, mesh, EntityTypes.PhysicsModel.OnLoad) then return false end
    return true
end

function EntityTypes.PhysicsModel.OnLoad(world, ent, id, status)
    if not status then return end
       
    print("TEST LUA CALLBACK")

    world.physics:ClearCollision(ent)

    local bb_size = world.visibility:GetBoxSizeL(ent)
    local bb_pos = world.visibility:GetBoxCenterL(ent)
    world.physics:AddBoxCollider(ent, bb_pos, Quaternion.Identity, bb_size)

    local bb_mass = bb_size.x * bb_size.y * bb_size.z * 8 * 100
    world.physics:SetMass(ent, bb_mass)
    world.physics:UpdateState(ent)
    
    return true
end