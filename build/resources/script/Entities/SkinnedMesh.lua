if not EntityTypes.SkinnedMesh then EntityTypes.SkinnedMesh = class(EntityTypes.BaseEntity) end

function EntityTypes.SkinnedMesh:init(world, ent)
    if not self:base(EntityTypes.SkinnedMesh).init(self, world, ent) then return false end
    
    self.world:SetEntityType(self.ent, "SkinnedMesh")

    self.transformSys:AddComponent(self.ent)
    self.world.visibility:AddComponent(self.ent)
    
    self.skeletonSys = self.world.skeleton
    self.skeletonSys:AddComponent(self.ent)

    self.staticMeshSys = self.world.staticMesh
    if not self.staticMeshSys:AddComponent(self.ent) then
        error("Cant init EntityTypes.SkinnedMesh")
        self:Kill()
    end
    self.staticMeshSys:SetMaterial(self.ent, 0, "../resources/materials/template_skinned.mtb")
    
    return true
end