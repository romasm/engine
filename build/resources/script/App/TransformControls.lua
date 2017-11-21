if not TransformControls then TransformControls = {} end

function TransformControls:Init( world )
    print("TransformControls:Init") 
    
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "move_arrow_x"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "move_arrow_y"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "move_arrow_z"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "scale_arrow_x"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "scale_arrow_y"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "scale_arrow_z"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "scale_center"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "rotate_x"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "rotate_y"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "rotate_z"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "rotate_center"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "move_square_xy"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "move_square_xz"..EXT.MESH, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( PATH.EDITOR_MESHES.. "move_square_yz"..EXT.MESH, RESOURCE_TYPE.MESH )

    self.scaleMul = 0.12
    self.radialCollisionMax = 1.06
    self.radialCollisionMin = 0.78

    self.world = world

    self.Xcontrol = self:CreatePart()
    self.Ycontrol = self:CreatePart()
    self.Zcontrol = self:CreatePart()
    self.XZcontrol = self:CreatePart()
    self.XYcontrol = self:CreatePart()
    self.YZcontrol = self:CreatePart()
    self.Ocontrol = self:CreatePart()

    self.mode = TRANSFORM_MODE.NONE
end

function TransformControls:CreatePart()
    local result = self.world:CreateEntity()
    if result:IsNull() then
        error("Cant init TransformControls entity")
    end 
    self.world:SetEntityType(result, EDITOR_VARS.TYPE)
    self.world.transform:AddComponent(result)
    self.world.staticMesh:AddComponent(result)
    self.world:SetEntityEnable(result, false)
    return result
end

function TransformControls:SetMode(m)
    self.mode = m

    if self.mode == TRANSFORM_MODE.NONE then
        self.world:SetEntityEnable(self.Xcontrol, false)
        self.world:SetEntityEnable(self.Ycontrol, false)
        self.world:SetEntityEnable(self.Zcontrol, false)
        self.world:SetEntityEnable(self.XZcontrol, false)
        self.world:SetEntityEnable(self.XYcontrol, false)
        self.world:SetEntityEnable(self.YZcontrol, false)
        self.world:SetEntityEnable(self.Ocontrol, false)



    elseif self.mode == TRANSFORM_MODE.MOVE then
        self.world:SetEntityEnable(self.Xcontrol, true)
        self.world:SetEntityEnable(self.Ycontrol, true)
        self.world:SetEntityEnable(self.Zcontrol, true)
        self.world:SetEntityEnable(self.XZcontrol, true)
        self.world:SetEntityEnable(self.XYcontrol, true)
        self.world:SetEntityEnable(self.YZcontrol, true)
        self.world:SetEntityEnable(self.Ocontrol, false)


        
    elseif self.mode == TRANSFORM_MODE.ROT then
        self.world:SetEntityEnable(self.Xcontrol, true)
        self.world:SetEntityEnable(self.Ycontrol, true)
        self.world:SetEntityEnable(self.Zcontrol, true)
        self.world:SetEntityEnable(self.XZcontrol, false)
        self.world:SetEntityEnable(self.XYcontrol, false)
        self.world:SetEntityEnable(self.YZcontrol, false)
        self.world:SetEntityEnable(self.Ocontrol, true)


        
    elseif self.mode == TRANSFORM_MODE.SCALE then
        self.world:SetEntityEnable(self.Xcontrol, true)
        self.world:SetEntityEnable(self.Ycontrol, true)
        self.world:SetEntityEnable(self.Zcontrol, true)
        self.world:SetEntityEnable(self.XZcontrol, true)
        self.world:SetEntityEnable(self.XYcontrol, true)
        self.world:SetEntityEnable(self.YZcontrol, true)
        self.world:SetEntityEnable(self.Ocontrol, true)


        
    end
end

function TransformControls:Close()
    self.world = nil
    self.Xcontrol = nil
    self.Ycontrol = nil
    self.Zcontrol = nil
    self.XZcontrol = nil
    self.XYcontrol = nil
    self.YZcontrol = nil
    self.Ocontrol = nil
end