if not EntityTypes.SkinnedMesh then EntityTypes.SkinnedMesh = class(EntityTypes.BaseEntity) end

function EntityTypes.SkinnedMesh:init(world, ent)
    if not self:base(EntityTypes.SkinnedMesh).init(self, world, ent) then return false end
    
    self.world:SetEntityType(self.ent, "SkinnedMesh")

    self.transformSys:AddComponent(self.ent)
    self.world.visibility:AddComponent(self.ent)
    
    self.skeletonSys = self.world.skeleton
    self.skeletonSys:AddComponent(self.ent)

    self.staticMeshSys = self.world.staticMesh
    self.staticMeshSys:AddComponent(self.ent)
    self.staticMeshSys:SetMaterial(self.ent, 0, AssetBrowser.nullMat)
    
    return true
end

function EntityTypes.SkinnedMesh:Test()
    self.transformSys:SetScale(self.ent, 0.01, 0.01, 0.01)

    self.skeletonSys:SetSkeleton(self.ent, "../content/statics/test_char.skl")
    self.staticMeshSys:SetMesh(self.ent, "../content/statics/test_char.msh")
    self.staticMeshSys:SetMaterial(self.ent, 1, AssetBrowser.nullMat)

    self.skeletonSys:AddAnimationBlended(self.ent, "../content/statics/test_char.ani")
    self.skeletonSys:SetAnimationBlend(self.ent, 0, 1.0)
    self.skeletonSys:SetAnimationSpeed(self.ent, 0, 1.0)
    self.skeletonSys:SetAnimationTime(self.ent, 0, 0.0)
    self.skeletonSys:SetAnimationPlaying(self.ent, 0, true)
    self.skeletonSys:SetAnimationLooping(self.ent, 0, true)
    --[[
    self.skeletonSys:AddAnimationBlended(self.ent, "../content/statics/test_char.ani")
    self.skeletonSys:SetAnimationBlend(self.ent, 1, 0.5)
    self.skeletonSys:SetAnimationSpeed(self.ent, 1, 0.0)
    self.skeletonSys:SetAnimationTime(self.ent, 1, 10000.0)
    self.skeletonSys:SetAnimationPlaying(self.ent, 1, true)
    --]]
end