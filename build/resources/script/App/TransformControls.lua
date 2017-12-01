if not TransformControls then TransformControls = {} end

function TransformControls:PreInit()
    self.XArrowPath = PATH.EDITOR_MESHES.. "move_arrow_x"..EXT.MESH
    self.YArrowPath = PATH.EDITOR_MESHES.. "move_arrow_y"..EXT.MESH
    self.ZArrowPath = PATH.EDITOR_MESHES.. "move_arrow_z"..EXT.MESH
    self.XScalePath = PATH.EDITOR_MESHES.. "scale_arrow_x"..EXT.MESH
    self.YScalePath = PATH.EDITOR_MESHES.. "scale_arrow_y"..EXT.MESH
    self.ZScalePath = PATH.EDITOR_MESHES.. "scale_arrow_z"..EXT.MESH
    self.CenterPath = PATH.EDITOR_MESHES.. "scale_center"..EXT.MESH
    self.XRotPath = PATH.EDITOR_MESHES.. "rotate_x"..EXT.MESH
    self.YRotPath = PATH.EDITOR_MESHES.. "rotate_y"..EXT.MESH
    self.ZRotPath = PATH.EDITOR_MESHES.. "rotate_z"..EXT.MESH
    self.CenterRotPath = PATH.EDITOR_MESHES.. "rotate_center"..EXT.MESH
    self.XYPlanePath = PATH.EDITOR_MESHES.. "move_square_xy"..EXT.MESH
    self.XZPlanePath = PATH.EDITOR_MESHES.. "move_square_xz"..EXT.MESH
    self.YZPlanePath = PATH.EDITOR_MESHES.. "move_square_yz"..EXT.MESH

    self.XArrowMatPath = PATH.EDITOR_MESHES.. "arrow_x"..EXT.MATERIAL
    self.YArrowMatPath = PATH.EDITOR_MESHES.. "arrow_y"..EXT.MATERIAL
    self.ZArrowMatPath = PATH.EDITOR_MESHES.. "arrow_z"..EXT.MATERIAL
    self.CenterMatPath = PATH.EDITOR_MESHES.. "arrow_all"..EXT.MATERIAL
    self.XYPlaneMatPath = PATH.EDITOR_MESHES.. "plane_xy"..EXT.MATERIAL
    self.XZPlaneMatPath = PATH.EDITOR_MESHES.. "plane_xz"..EXT.MATERIAL
    self.YZPlaneMatPath = PATH.EDITOR_MESHES.. "plane_yz"..EXT.MATERIAL
    self.XRotMatPath = PATH.EDITOR_MESHES.. "rot_x"..EXT.MATERIAL
    self.YRotMatPath = PATH.EDITOR_MESHES.. "rot_y"..EXT.MATERIAL
    self.ZRotMatPath = PATH.EDITOR_MESHES.. "rot_z"..EXT.MATERIAL

    Resource.PreloadResource( self.XArrowPath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.YArrowPath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.ZArrowPath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.XScalePath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.YScalePath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.ZScalePath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.CenterPath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.XRotPath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.YRotPath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.ZRotPath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.CenterRotPath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.XYPlanePath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.XZPlanePath, RESOURCE_TYPE.MESH )
    Resource.PreloadResource( self.YZPlanePath, RESOURCE_TYPE.MESH )

    Resource.PreloadResource( self.XArrowMatPath, RESOURCE_TYPE.MATERIAL )
    Resource.PreloadResource( self.YArrowMatPath, RESOURCE_TYPE.MATERIAL )
    Resource.PreloadResource( self.ZArrowMatPath, RESOURCE_TYPE.MATERIAL )
    Resource.PreloadResource( self.CenterMatPath, RESOURCE_TYPE.MATERIAL )
    Resource.PreloadResource( self.XYPlaneMatPath, RESOURCE_TYPE.MATERIAL )
    Resource.PreloadResource( self.XZPlaneMatPath, RESOURCE_TYPE.MATERIAL )
    Resource.PreloadResource( self.YZPlaneMatPath, RESOURCE_TYPE.MATERIAL )
    Resource.PreloadResource( self.XRotMatPath, RESOURCE_TYPE.MATERIAL )
    Resource.PreloadResource( self.YRotMatPath, RESOURCE_TYPE.MATERIAL )
    Resource.PreloadResource( self.ZRotMatPath, RESOURCE_TYPE.MATERIAL )

    self.scaleMul = 0.12
    self.radialCollisionMaxSq = 1.06 * 1.06
    self.radialCollisionMinSq = 0.78 * 0.78

    self.hoverColor = Vector4(1.0, 0.5, 0.0, 1.0)
    self.XColor = Vector4(0.9, 0.0, 0.0, 1.0)
    self.YColor = Vector4(0.0, 0.9, 0.0, 1.0)
    self.ZColor = Vector4(0.0, 0.0, 0.9, 1.0)
    self.XYColor = Vector4(0.6, 0.6, 0.0, 1.0)
    self.XZColor = Vector4(0.6, 0.0, 0.6, 1.0)
    self.YZColor = Vector4(0.0, 0.6, 0.6, 1.0)
    self.CColor = Vector4(0.9, 0.9, 0.9, 1.0)
end

function TransformControls:Init( world )
    print("TransformControls:Init") 
       
    self.world = world
    self.MeshSy = self.world.staticMesh
    self.TransformSy = self.world.transform
    self.CameraSy = self.world.camera
    self.VisSy = self.world.visibility

    self.Xcontrol = self:CreatePart()
    self.Ycontrol = self:CreatePart()
    self.Zcontrol = self:CreatePart()
    self.XZcontrol = self:CreatePart()
    self.XYcontrol = self:CreatePart()
    self.YZcontrol = self:CreatePart()
    self.Ccontrol = self:CreatePart()

    self.TransformSy:Attach(self.Xcontrol, self.Ccontrol)
    self.TransformSy:Attach(self.Ycontrol, self.Ccontrol)
    self.TransformSy:Attach(self.Zcontrol, self.Ccontrol)
    self.TransformSy:Attach(self.XYcontrol, self.Ccontrol)
    self.TransformSy:Attach(self.XZcontrol, self.Ccontrol)
    self.TransformSy:Attach(self.YZcontrol, self.Ccontrol)
        
    -- prevent null meshes
    self:SetMode(TRANSFORM_MODE.SCALE)
    self:SetMode(TRANSFORM_MODE.NONE)

    self.mode = TRANSFORM_MODE.NONE
    self.hoverControl = ""
    self.isLocal = false
    self.active = false
    
    self.camPos = Vector3.Zero
    self.frustID = 0
    self.farClip = 0

    self.currentPos = Vector3.Zero
    self.currentRot = Quaternion.Identity
    self.currentScale = 0
end

function TransformControls:CreatePart()
    local result = self.world:CreateEntity()
    if result:IsNull() then
        error("Cant init TransformControls entity")
    end 
    self.world:SetEntityType(result, EDITOR_VARS.TYPE)
    self.TransformSy:AddComponent(result)
    self.VisSy:AddComponent(result)
    self.MeshSy:AddComponent(result)
    self.world:SetEntityEnable(result, false)
    return result
end

function TransformControls:Tick(camEnt)
    if not self.world then return end
    
    self.camPos = self.CameraSy:GetPos(camEnt)    
    self.camDir = self.CameraSy:GetLookDir(camEnt)    
    self.camTang = self.CameraSy:GetLookTangent(camEnt)    
    self.camUp = self.CameraSy:GetUp(camEnt)    
    self.frustID = self.CameraSy:GetFrustumId(camEnt)
    self.farClip = self.CameraSy:GetFar(camEnt)

    self:UpdateScale()
end

function TransformControls:UpdateTransform(selectionSet)
    if selectionSet == nil then 
		self:Deactivate()
		return
	end
    if #selectionSet == 0 then 
		self:Deactivate()
		return
	end
	
    if self.active == false then self:Activate() end

    self.currentPos = Vector3.Zero
    self.currentRot = Quaternion.Zero

	for i, ent in ipairs(selectionSet) do
        self.currentPos = self.currentPos + self.TransformSy:GetPosition_W(ent)
        self.currentRot = self.currentRot + self.TransformSy:GetRotation_W(ent)
	end

    self.currentPos = Vector3.MulScalar(self.currentPos, 1.0 / #selectionSet)
    self.currentRot:Normalize()

    self.TransformSy:SetPosition_L(self.Ccontrol, self.currentPos)
    if self.isLocal then
        self.TransformSy:SetRotation_L(self.Ccontrol, self.currentRot)
    else 
        self.TransformSy:SetRotationPYR_L3F(self.Ccontrol, 0, 0, 0)
    end

    self:UpdateScale()
end

function TransformControls:UpdateScale()
    local distVect = Vector3.Sub(self.camPos, self.currentPos)
    self.currentScale = distVect:Length() * self.scaleMul
    self.TransformSy:SetScale_L3F(self.Ccontrol, self.currentScale, self.currentScale, self.currentScale)
end

function TransformControls:Close()
    self.world = nil
    self.Xcontrol = nil
    self.Ycontrol = nil
    self.Zcontrol = nil
    self.XZcontrol = nil
    self.XYcontrol = nil
    self.YZcontrol = nil
    self.Ccontrol = nil
end

function TransformControls:Unhover()
    -- TODO: material better access
    self.MeshSy:GetMaterialObject(self.Xcontrol, 0):SetVectorByID(self.XColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.Ycontrol, 0):SetVectorByID(self.YColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.Zcontrol, 0):SetVectorByID(self.ZColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.XYcontrol, 0):SetVectorByID(self.XYColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.XZcontrol, 0):SetVectorByID(self.XZColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.YZcontrol, 0):SetVectorByID(self.YZColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.Ccontrol, 0):SetVectorByID(self.CColor, 0, SHADERS.PS)

    self.hoverControl = ""
end

function TransformControls:CheckRadial(ray, dist)
    if self.mode ~= TRANSFORM_MODE.ROT then return true end

    local radialVect = Vector3.Sub( Vector3.Add(self.camPos, Vector3.MulScalar(ray, dist)), self.currentPos )
    radialVect = Vector3.MulScalar(radialVect, 1.0/self.currentScale)
    local radialDistSq = radialVect:LengthSq()
    if radialDistSq <= self.radialCollisionMaxSq and radialDistSq >= self.radialCollisionMinSq then return true
    else return false end
end

function TransformControls:CheckHover(ray)
    if self.mode == TRANSFORM_MODE.NONE then return false end
    
    self.hoverControl = ""
    local minDist = self.farClip

    local tempDist = self.VisSy:CollideRaySingleEntity(self.Xcontrol, self.camPos, ray, self.frustID)
    if tempDist >= 0.0 and tempDist < minDist and self:CheckRadial(ray, tempDist) then 
        self.hoverControl = "X" 
        minDist = tempDist
    end
    tempDist = self.VisSy:CollideRaySingleEntity(self.Ycontrol, self.camPos, ray, self.frustID)
    if tempDist >= 0.0 and tempDist < minDist and self:CheckRadial(ray, tempDist) then 
        self.hoverControl = "Y" 
        minDist = tempDist
    end
    tempDist = self.VisSy:CollideRaySingleEntity(self.Zcontrol, self.camPos, ray, self.frustID)
    if tempDist >= 0.0 and tempDist < minDist and self:CheckRadial(ray, tempDist) then 
        self.hoverControl = "Z" 
        minDist = tempDist
    end

    if self.mode ~= TRANSFORM_MODE.ROT then
        tempDist = self.VisSy:CollideRaySingleEntity(self.XYcontrol, self.camPos, ray, self.frustID)
        if tempDist >= 0.0 and tempDist < minDist then 
            self.hoverControl = "XY" 
            minDist = tempDist
        end
        tempDist = self.VisSy:CollideRaySingleEntity(self.XZcontrol, self.camPos, ray, self.frustID)
        if tempDist >= 0.0 and tempDist < minDist then 
            self.hoverControl = "XZ" 
            minDist = tempDist
        end
        tempDist = self.VisSy:CollideRaySingleEntity(self.YZcontrol, self.camPos, ray, self.frustID)
        if tempDist >= 0.0 and tempDist < minDist then 
            self.hoverControl = "YZ" 
            minDist = tempDist
        end
    end

    if self.mode ~= TRANSFORM_MODE.MOVE then
        tempDist = self.VisSy:CollideRaySingleEntity(self.Ccontrol, self.camPos, ray, self.frustID)
        if tempDist >= 0.0 and tempDist < minDist then 
            self.hoverControl = "CNT" 
            minDist = tempDist
        end
    end

    -- TODO: material better access
    self.MeshSy:GetMaterialObject(self.Xcontrol, 0):SetVectorByID(self.XColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.Ycontrol, 0):SetVectorByID(self.YColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.Zcontrol, 0):SetVectorByID(self.ZColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.XYcontrol, 0):SetVectorByID(self.XYColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.XZcontrol, 0):SetVectorByID(self.XZColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.YZcontrol, 0):SetVectorByID(self.YZColor, 0, SHADERS.PS)
    self.MeshSy:GetMaterialObject(self.Ccontrol, 0):SetVectorByID(self.CColor, 0, SHADERS.PS)

    if self.hoverControl:len() == 0 then return false end
    
    if self.hoverControl == "X" then
        self.MeshSy:GetMaterialObject(self.Xcontrol, 0):SetVectorByID(self.hoverColor, 0, SHADERS.PS)
    elseif self.hoverControl == "Y" then
        self.MeshSy:GetMaterialObject(self.Ycontrol, 0):SetVectorByID(self.hoverColor, 0, SHADERS.PS)        
    elseif self.hoverControl == "Z" then
        self.MeshSy:GetMaterialObject(self.Zcontrol, 0):SetVectorByID(self.hoverColor, 0, SHADERS.PS) 
    elseif self.hoverControl == "XY" then
        self.MeshSy:GetMaterialObject(self.XYcontrol, 0):SetVectorByID(self.hoverColor, 0, SHADERS.PS) 
    elseif self.hoverControl == "XZ" then
        self.MeshSy:GetMaterialObject(self.XZcontrol, 0):SetVectorByID(self.hoverColor, 0, SHADERS.PS) 
    elseif self.hoverControl == "YZ" then
        self.MeshSy:GetMaterialObject(self.YZcontrol, 0):SetVectorByID(self.hoverColor, 0, SHADERS.PS) 
    elseif self.hoverControl == "CNT" then
        self.MeshSy:GetMaterialObject(self.Ccontrol, 0):SetVectorByID(self.hoverColor, 0, SHADERS.PS) 
    end
    
    return true
end

function TransformControls:ApplyTransform(rayNext, rayPrev, selectionSet)
    local hoverType = self.hoverControl:len()
    if self.mode == TRANSFORM_MODE.NONE or hoverType == 0 or #selectionSet == 0 then return end
    
    local rayNextFar = Vector3.MulScalar(rayNext, self.farClip)
    local rayPrevFar = Vector3.MulScalar(rayPrev, self.farClip)

    if self.mode == TRANSFORM_MODE.MOVE then
        local move = Vector3.Zero

        if hoverType == 1 then
            local viewVect = Vector3.Sub(self.currentPos, self.camPos)
            viewVect:Normalize()

            local axis = Vector3(1,0,0)
            if self.hoverControl == "Y" then axis = Vector3(0,1,0)
            elseif self.hoverControl == "Z" then axis = Vector3(0,0,1) end
 
            local dir = axis
            if self.isLocal then
                dir = Vector3.Rotate(dir, self.currentRot)
            end

            local normal = dir:Cross( dir:Cross(viewVect) )
            normal:Normalize()

            local plane = Vector4.CreatePlane(self.currentPos, normal)
            local fromP = Vector4.PlaneLineCollide(plane, self.camPos, rayPrevFar)
            local toP = Vector4.PlaneLineCollide(plane, self.camPos, rayNextFar)

            local moveVect = Vector3.Sub(toP, fromP)
            move = Vector3.MulScalar(dir, moveVect:Dot(dir))

        elseif hoverType == 2 then
            local axis = Vector3(0,0,1)
            if self.hoverControl == "XZ" then axis = Vector3(0,1,0)
            elseif self.hoverControl == "YZ" then axis = Vector3(1,0,0) end
            
            local normal = axis
            if self.isLocal then
                normal = Vector3.Rotate(normal, self.currentRot)
            end

            local plane = Vector4.CreatePlane(self.currentPos, normal)
            local fromP = Vector4.PlaneLineCollide(plane, self.camPos, rayPrevFar)
            local toP = Vector4.PlaneLineCollide(plane, self.camPos, rayNextFar)

            move = Vector3.Sub(toP, fromP)
            
        end

        self.TransformSy:AddPosition_L(self.Ccontrol, move)
        self.currentPos = self.TransformSy:GetPosition_L(self.Ccontrol)
        
        for i, ent in ipairs(selectionSet) do
			self.TransformSy:AddPosition_W(ent, move)
		end

    elseif self.mode == TRANSFORM_MODE.ROT then
        local normal = Vector3.Zero

        if hoverType == 1 then
            local axis = Vector3(1,0,0)
            if self.hoverControl == "Y" then axis = Vector3(0,1,0)
            elseif self.hoverControl == "Z" then axis = Vector3(0,0,1) end 

            if self.isLocal then
                normal = Vector3.Rotate(axis, self.currentRot)
            else
                normal = axis
            end
        else
            normal = Vector3.Inverse(self.camDir)
            normal:Normalize()
        end
        
        local plane = Vector4.CreatePlane(self.currentPos, normal)

        local fromP = Vector4.PlaneLineCollide(plane, self.camPos, rayPrevFar)
        local toP = Vector4.PlaneLineCollide(plane, self.camPos, rayNextFar)
        fromP = Vector3.Sub(fromP, self.currentPos)
        toP = Vector3.Sub(toP, self.currentPos)

        local angle = Vector3.AngleBetween(fromP, toP)
        local rotDirTest = fromP:Cross(toP)
        if normal:Dot(rotDirTest) < 0 then angle = -angle end

        if self.isLocal then
            self.TransformSy:AddRotationAxis_L(self.Ccontrol, normal, angle)
            self.currentRot = self.TransformSy:GetRotation_L(self.Ccontrol)
        end
        
        -- TODO: group rotation
        for i, ent in ipairs(selectionSet) do
			self.TransformSy:AddRotationAxis_W(ent, normal, angle)
		end  

    elseif self.mode == TRANSFORM_MODE.SCALE then
        local scale = Vector3.Zero
        local plane = Vector4.Zero
        local dir = Vector3.Zero
        local axis = Vector3.Zero

        if hoverType == 1 then
            local viewVect = Vector3.Sub(self.currentPos, self.camPos)
            viewVect:Normalize()

            axis = Vector3(1,0,0)
            if self.hoverControl == "Y" then axis = Vector3(0,1,0)
            elseif self.hoverControl == "Z" then axis = Vector3(0,0,1) end
 
            dir = Vector3.Rotate(axis, self.currentRot)
            local normal = dir:Cross( dir:Cross(viewVect) )
            normal:Normalize()
            plane = Vector4.CreatePlane(self.currentPos, normal)

        elseif hoverType == 2 then
            axis = Vector3(1,1,0)
            local planeAxis = Vector3(0,0,1)
            if self.hoverControl == "XZ" then 
                axis = Vector3(1,0,1)
                planeAxis = Vector3(0,1,0)
            elseif self.hoverControl == "YZ" then 
                axis = Vector3(0,1,1)
                planeAxis = Vector3(1,0,0)
            end
            
            dir = Vector3.Rotate(axis, self.currentRot)
            local normal = Vector3.Rotate(planeAxis, self.currentRot)
            plane = Vector4.CreatePlane(self.currentPos, normal)

        elseif hoverType == 3 then -- TODO: scale from delta mouse coords
            axis = Vector3(1,1,1)    
            plane = Vector4.CreatePlane(self.currentPos, Vector3.Inverse(self.camDir))
            dir = Vector3.Add( Vector3.Inverse(self.camTang), self.camUp )
            dir:Normalize()  
                
        end
        
        local fromP = Vector4.PlaneLineCollide(plane, self.camPos, rayPrevFar)
        local toP = Vector4.PlaneLineCollide(plane, self.camPos, rayNextFar)
        local moveVect = Vector3.Sub(toP, fromP)           
        scale = Vector3.MulScalar(axis, moveVect:Dot(dir))   
        scale = Vector3.Add(Vector3(1,1,1), scale)

        for i, ent in ipairs(selectionSet) do
			self.TransformSy:AddScale_L(ent, scale)
		end
    end
end

function TransformControls:Deactivate()
    self.active = false
    self.world:SetEntityEnable(self.Xcontrol, false)
    self.world:SetEntityEnable(self.Ycontrol, false)
    self.world:SetEntityEnable(self.Zcontrol, false)
    self.world:SetEntityEnable(self.XZcontrol, false)
    self.world:SetEntityEnable(self.XYcontrol, false)
    self.world:SetEntityEnable(self.YZcontrol, false)
    self.world:SetEntityEnable(self.Ccontrol, false)
end

function TransformControls:Activate()
    self.active = true
    self:SetMode(self.mode)
end

function TransformControls:SetLocal(lcl)
    self.isLocal = lcl
end

function TransformControls:SetMode(m)
    self.mode = m

    if self.mode == TRANSFORM_MODE.NONE then
        self:Deactivate()
            
    elseif self.mode == TRANSFORM_MODE.MOVE then
        self.world:SetEntityEnable(self.Xcontrol, true)
        self.world:SetEntityEnable(self.Ycontrol, true)
        self.world:SetEntityEnable(self.Zcontrol, true)
        self.world:SetEntityEnable(self.XZcontrol, true)
        self.world:SetEntityEnable(self.XYcontrol, true)
        self.world:SetEntityEnable(self.YZcontrol, true)
        self.world:SetEntityEnable(self.Ccontrol, false)

        self.MeshSy:SetMesh(self.Xcontrol, self.XArrowPath)
        self.MeshSy:SetMaterial(self.Xcontrol, 0, self.XArrowMatPath)
        self.MeshSy:SetMesh(self.Ycontrol, self.YArrowPath)
        self.MeshSy:SetMaterial(self.Ycontrol, 0, self.YArrowMatPath)
        self.MeshSy:SetMesh(self.Zcontrol, self.ZArrowPath)
        self.MeshSy:SetMaterial(self.Zcontrol, 0, self.ZArrowMatPath)

        self.MeshSy:SetMesh(self.XYcontrol, self.XYPlanePath)
        self.MeshSy:SetMaterial(self.XYcontrol, 0, self.XYPlaneMatPath)
        self.MeshSy:SetMesh(self.XZcontrol, self.XZPlanePath)
        self.MeshSy:SetMaterial(self.XZcontrol, 0, self.XZPlaneMatPath)
        self.MeshSy:SetMesh(self.YZcontrol, self.YZPlanePath)
        self.MeshSy:SetMaterial(self.YZcontrol, 0, self.YZPlaneMatPath)
        
    elseif self.mode == TRANSFORM_MODE.ROT then
        self.world:SetEntityEnable(self.Xcontrol, true)
        self.world:SetEntityEnable(self.Ycontrol, true)
        self.world:SetEntityEnable(self.Zcontrol, true)
        self.world:SetEntityEnable(self.XZcontrol, false)
        self.world:SetEntityEnable(self.XYcontrol, false)
        self.world:SetEntityEnable(self.YZcontrol, false)
        self.world:SetEntityEnable(self.Ccontrol, true)
        
        self.MeshSy:SetMesh(self.Xcontrol, self.XRotPath)
        self.MeshSy:SetMaterial(self.Xcontrol, 0, self.XRotMatPath)
        self.MeshSy:SetMesh(self.Ycontrol, self.YRotPath)
        self.MeshSy:SetMaterial(self.Ycontrol, 0, self.YRotMatPath)
        self.MeshSy:SetMesh(self.Zcontrol, self.ZRotPath)
        self.MeshSy:SetMaterial(self.Zcontrol, 0, self.ZRotMatPath)

        self.MeshSy:SetMesh(self.Ccontrol, self.CenterRotPath)
        self.MeshSy:SetMaterial(self.Ccontrol, 0, self.CenterMatPath)
        
    elseif self.mode == TRANSFORM_MODE.SCALE then
        self.world:SetEntityEnable(self.Xcontrol, true)
        self.world:SetEntityEnable(self.Ycontrol, true)
        self.world:SetEntityEnable(self.Zcontrol, true)
        self.world:SetEntityEnable(self.XZcontrol, true)
        self.world:SetEntityEnable(self.XYcontrol, true)
        self.world:SetEntityEnable(self.YZcontrol, true)
        self.world:SetEntityEnable(self.Ccontrol, true)

        self.MeshSy:SetMesh(self.Xcontrol, self.XScalePath)
        self.MeshSy:SetMaterial(self.Xcontrol, 0, self.XArrowMatPath)
        self.MeshSy:SetMesh(self.Ycontrol, self.YScalePath)
        self.MeshSy:SetMaterial(self.Ycontrol, 0, self.YArrowMatPath)
        self.MeshSy:SetMesh(self.Zcontrol, self.ZScalePath)
        self.MeshSy:SetMaterial(self.Zcontrol, 0, self.ZArrowMatPath)

        self.MeshSy:SetMesh(self.XYcontrol, self.XYPlanePath)
        self.MeshSy:SetMaterial(self.XYcontrol, 0, self.XYPlaneMatPath)
        self.MeshSy:SetMesh(self.XZcontrol, self.XZPlanePath)
        self.MeshSy:SetMaterial(self.XZcontrol, 0, self.XZPlaneMatPath)
        self.MeshSy:SetMesh(self.YZcontrol, self.YZPlanePath)
        self.MeshSy:SetMaterial(self.YZcontrol, 0, self.YZPlaneMatPath)

        self.MeshSy:SetMesh(self.Ccontrol, self.CenterPath)
        self.MeshSy:SetMaterial(self.Ccontrol, 0, self.CenterMatPath)
        
    end
end