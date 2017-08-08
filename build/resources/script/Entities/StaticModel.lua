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

    -- TEST
    --world.staticMesh:SetMesh(ent, "")
    --world.staticMesh:SetMeshAndCallback(ent, PATH.ROOT .. "content/statics/ebisu_200.fbx", EntityTypes.StaticModel.OnLoad)

    world.physics:AddComponent(ent)
    world.collision:AddComponent(ent)

    local bb_size = world.visibility:GetBoxSizeL(ent)
    local bb_pos = world.visibility:GetBoxCenterL(ent)
    world.collision:AddBoxCollider(ent, bb_pos, Quaternion.Identity, 1.0, bb_size, 0)

    local physicsSys = world.physics
    physicsSys:SetType(ent, PHYSICS_TYPES.STATIC)
    physicsSys:SetBounciness(ent, 0.0)
    physicsSys:SetFriction(ent, 1.0)
    physicsSys:SetActive(ent, true)
    
    return true
end