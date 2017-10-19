if not EntityTypes.SkinnedMesh then EntityTypes.SkinnedMesh = class(EntityTypes.BaseEntity) end

function EntityTypes.SkinnedMesh:init(world, ent)
    if not self:base(EntityTypes.SkinnedMesh).init(self, world, ent) then return false end
    
    self.world:SetEntityType(self.ent, "SkinnedMesh")

    self.transformSys:AddComponent(self.ent)
    self.world.visibility:AddComponent(self.ent)
    
    self.skeletonSys = self.world.skeleton
    self.skeletonSys:AddComponent(self.ent)
    self.skeletonSys:SetSkeleton(self.ent, "../content/statics/test_char.skl")
    self.skeletonSys:SetAnimation(self.ent, "../content/statics/test_char.ani")

    self.staticMeshSys = self.world.staticMesh
    self.staticMeshSys:AddComponent(self.ent)
    self.staticMeshSys:SetMesh(self.ent, "../content/statics/test_char.msh")
    self.staticMeshSys:SetMaterial(self.ent, 0, AssetBrowser.nullMat)
    self.staticMeshSys:SetMaterial(self.ent, 1, AssetBrowser.nullMat)
    
    return true
end