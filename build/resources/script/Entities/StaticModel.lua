if not EntityTypes.StaticModel then EntityTypes.StaticModel = class(EntityTypes.Mesh) end

function EntityTypes.StaticModel:init(world, ent)
    if not self:base(EntityTypes.StaticModel).init(self, world, ent) then return false end
    
    self.world:SetEntityType(self.ent, "StaticModel")
    
    self.physicsSys = self.world.physics
    self.physicsSys:AddComponent(self.ent)
    self.physicsSys:SetType(self.ent, PHYSICS_TYPES.STATIC)

    return true
end

-- temp 
function EntityTypes.StaticModel:SetMesh(mesh)
    if not self:base(EntityTypes.StaticModel).SetMesh(self, mesh) then return false end

    local clsMesh = mesh:gsub(EXT.MESH, EXT.COLLISION)

    self.physicsSys:ClearCollision(self.ent)
    self.physicsSys:SetConvexHullsCollider(self.ent, clsMesh)
    self.physicsSys:SetRestitution(self.ent, 0.1)
    self.physicsSys:SetFriction(self.ent, 0.8)
    self.physicsSys:UpdateState(self.ent)

    return true
end

function EntityTypes.StaticModel.OnLoad(world, ent, id, status)
    if not status then return end
       
    print("TEST LUA CALLBACK")
    
    local physicsSys = world.physics
    physicsSys:AddComponent(ent)

    local bb_size = world.visibility:GetBoxSizeL(ent)
    local bb_pos = world.visibility:GetBoxCenterL(ent)
    physicsSys:AddBoxCollider(ent, bb_pos, Quaternion.Identity, bb_size)
    physicsSys:SetType(ent, PHYSICS_TYPES.STATIC)
    physicsSys:UpdateState(ent)
    
    return true
end