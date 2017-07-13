if not EntityTypes.StaticModel then EntityTypes.StaticModel = class(EntityTypes.BaseEntity) end

function EntityTypes.StaticModel:init(world, ent)
    if not self:base(EntityTypes.StaticModel).init(self, world, ent) then return false end

    self.world:SetEntityType(self.ent, "StaticModel")

    self.transformSys:AddComponent(self.ent)
    self.world.visibility:AddComponent(self.ent)
    
    self.staticMeshSys = self.world.staticMesh

    if not self.staticMeshSys:AddComponentMesh(self.ent, "") then
        error("Cant init EntityTypes.StaticModel")
        self:Kill()
    end
    self.staticMeshSys:SetMaterial(self.ent, 0, AssetBrowser.nullMat)
    
    return true
end

function EntityTypes.StaticModel:SetMaterial(material, id)
    return self.staticMeshSys:SetMaterial(self.ent, id, material)
end

function EntityTypes.StaticModel:GetMaterialName(id)
    return self.staticMeshSys:GetMaterial(self.ent, id)
end

function EntityTypes.StaticModel:GetMaterialRef(id)
    return self.staticMeshSys:GetMaterialObject(self.ent, id)
end

function EntityTypes.StaticModel:GetMaterialsCount()
    return self.staticMeshSys:GetMaterialsCount(self.ent)
end

function EntityTypes.StaticModel:SetMesh(mesh)
    return self.staticMeshSys:SetMesh(self.ent, mesh)
end

function EntityTypes.StaticModel:GetMeshName()
    return self.staticMeshSys:GetMesh(self.ent)
end