if not EntityTypes.StaticModel then EntityTypes.StaticModel = class(EntityTypes.Mesh) end

function EntityTypes.StaticModel:init(world, ent)
    if not self:base(EntityTypes.StaticModel).init(self, world, ent) then return false end
    
    self.world:SetEntityType(self.ent, "StaticModel")
    
    return true
end

function EntityTypes.StaticModel:SetMesh(mesh)
    if not self:base(EntityTypes.StaticModel).SetMeshAndCallback(self, mesh, EntityTypes.StaticModel.OnLoad) then return false end
    return true
end

function EntityTypes.StaticModel.OnLoad(world, ent, id, status)
    if not status then return end
    -- temp    
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