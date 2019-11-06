if not EntityTypes.Mesh then EntityTypes.Mesh = class(EntityTypes.BaseEntity) end

function EntityTypes.Mesh:init(world, ent)
    if not self:base(EntityTypes.Mesh).init(self, world, ent) then return false end

    self.world:SetEntityType(self.ent, "Mesh")

    self.transformSys:AddComponent(self.ent)
    self.world.visibility:AddComponent(self.ent)
    
    self.staticMeshSys = self.world.staticMesh

    if not self.staticMeshSys:AddComponent(self.ent) then
        error("Cant init EntityTypes.Mesh")
        self:Kill()
    end
    self.staticMeshSys:SetMaterial(self.ent, 0, AssetBrowser.nullMat)
    
    return true
end

function EntityTypes.Mesh:SetMaterial(material, id)
    return self.staticMeshSys:SetMaterial(self.ent, id, material)
end

function EntityTypes.Mesh:GetMaterialName(id)
    return self.staticMeshSys:GetMaterial(self.ent, id)
end

function EntityTypes.Mesh:GetMaterialRef(id)
    return self.staticMeshSys:GetMaterialObject(self.ent, id)
end

function EntityTypes.Mesh:GetMaterialsCount()
    return self.staticMeshSys:GetMaterialsCount(self.ent)
end

function EntityTypes.Mesh:SetMesh(mesh)
    return self.staticMeshSys:SetMesh(self.ent, mesh)
end

function EntityTypes.Mesh:SetMeshAndCallback(mesh, func)
    return self.staticMeshSys:SetMeshAndCallback(self.ent, mesh, func)
end

function EntityTypes.Mesh:GetMeshName()
    return self.staticMeshSys:GetMesh(self.ent)
end