if not EntityTypes.EnvProb then EntityTypes.EnvProb = class(EntityTypes.BaseEntity) end

function EntityTypes.EnvProb:init(world, ent)
    if not self:base(EntityTypes.EnvProb).init(self, world, ent) then return false end

    self.world:SetEntityType(self.ent, "EnvProb")

    self.transformSys:AddComponent(self.ent)
    self.world.earlyVisibility:AddComponent(self.ent)

    self.envProbSys = self.world.envprobs

    self.envProbSys:AddComponent(self.ent)
    
    self:editor_init()

    return true
end

function EntityTypes.EnvProb:editor_init()
    local editorEnt = self.world:CreateEntity()
    if not editorEnt:IsNull() then 
        self.world:SetEntityType(editorEnt, EDITOR_VARS.TYPE)
        self.transformSys:AddComponent(editorEnt)
        self.world.visibility:AddComponent(editorEnt)
        
        if self.world.staticMesh:AddComponent(editorEnt) then
            self.world.staticMesh:SetMesh(editorEnt, PATH.EDITOR_MESHES.. "pointlight"..EXT.MESH)
            self.world.staticMesh:SetMaterial(editorEnt, 0, PATH.EDITOR_MESHES.. "pointlight"..EXT.MATERIAL)
            self.transformSys:Attach(editorEnt, self.ent)
            return
        end

        self.world:DestroyEntity(editorEnt)
    end
    error("Cant init EntityTypes.EnvProb editor entity")
end

function EntityTypes.EnvProb:GetType()
    return self.envProbSys:GetType(self.ent)
end

function EntityTypes.EnvProb:SetType(t)
    self.envProbSys:SetType(self.ent, t)
end

function EntityTypes.EnvProb:GetFade()
    return self.envProbSys:GetFade(self.ent)
end

function EntityTypes.EnvProb:SetFade(fade)
    self.envProbSys:SetFade(self.ent, fade)
end

function EntityTypes.EnvProb:GetOffset()
    return self.envProbSys:GetOffset(self.ent)
end

function EntityTypes.EnvProb:SetOffset(vect)
    self.envProbSys:SetOffset(self.ent, vect)
end

function EntityTypes.EnvProb:GetNearClip()
    return self.envProbSys:GetNearClip(self.ent)
end

function EntityTypes.EnvProb:SetNearClip(clip)
    self.envProbSys:SetNearClip(self.ent, clip)
end

function EntityTypes.EnvProb:GetFarClip()
    return self.envProbSys:GetFarClip(self.ent)
end

function EntityTypes.EnvProb:SetFarClip(clip)
    self.envProbSys:SetFarClip(self.ent, clip)
end

function EntityTypes.EnvProb:GetPriority()
    return self.envProbSys:GetPriority(self.ent)
end

function EntityTypes.EnvProb:SetPriority(priority)
    self.envProbSys:SetPriority(self.ent, priority)
end

function EntityTypes.EnvProb:GetQuality()
    return self.envProbSys:GetQuality(self.ent)
end

function EntityTypes.EnvProb:SetQuality(quality)
    self.envProbSys:SetQuality(self.ent, quality)
end

function EntityTypes.EnvProb:Bake()
    self.envProbSys:Bake(self.ent)
end