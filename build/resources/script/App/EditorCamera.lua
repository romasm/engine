if not EditorCamera then EditorCamera = {} end

function EditorCamera:Init( world )
    print("EditorCamera:Init") 
    
    self.world = world

    local nodeName = EDITOR_VARS.INVIS.."CameraNode"
    local nodeEnt = self.world:GetEntityByName(nodeName)
    if nodeEnt:IsNull() then 
        self.cameraNode = EntityTypes.Node(self.world)
        self.world:RenameEntity(self.cameraNode.ent, nodeName)
        self.cameraNode:SetPosition_L3F(0.0, 1.0, 0.0)
    else
        self.cameraNode = EntityTypes.wrap(self.world, nodeEnt)
    end
    
    self.camera = EntityTypes.Camera(self.world)
    self.world:RenameEntity(self.camera.ent, EDITOR_VARS.TYPE.."Camera")
    self.camera:Attach(self.cameraNode)
    self.camera:SetFov(1.0)
    self.camera:SetFar(10000.0)
    self.camera:SetNear(0.1)

    self.cameraEntity = self.camera.ent
    
    -- editor plane
    self:EditorPlane()

    -- from config
    self.movespeed = 0.005
    self.rotspeed = 0.01

    -- private
    self.rot_coords = {pitch = 0, yaw = 0}

    self.states = {
        forward = false,
        backward = false,
        left = false,
        right = false,
        up = false,
        down = false,
    }
end

function EditorCamera:GetPosition()
    return self.camera:GetPosition_W()
end

function EditorCamera:GetDirection()
    return self.camera:GetLookDir()
end

function EditorCamera:EditorPlane()
    self.planeEnt = self.world:CreateEntity()
    if not self.planeEnt:IsNull() then 
        self.world:SetEntityType(self.planeEnt, EDITOR_VARS.TYPE)
        self.world.transform:AddComponent(self.planeEnt)

        if self.world.staticMesh:AddComponent(self.planeEnt) then
            self.world.staticMesh:SetMesh(self.planeEnt, PATH.EDITOR_MESHES.. "unit_plane"..EXT.MESH)
            self.world.staticMesh:SetMaterial(self.planeEnt, 0, PATH.EDITOR_MESHES.. "unit_plane"..EXT.MATERIAL)
            self.world.transform:SetScale_L3F(self.planeEnt, 100.0, 100.0, 100.0)
            return
        end

        self.world:DestroyEntity(self.planeEnt)
    end
    error("Cant init editor plane")
end

function EditorCamera:Close()
    self.world = nil
    self.cameraEntity = nil
    self.camera = nil
    self.cameraNode = nil
    self.planeEnt = nil
end

function EditorCamera:onStartMove(key)
    if key == KEYBOARD_CODES.KEY_W then self.states.forward = true
    elseif key == KEYBOARD_CODES.KEY_S then self.states.backward = true
    elseif key == KEYBOARD_CODES.KEY_A then self.states.left = true
    elseif key == KEYBOARD_CODES.KEY_D then self.states.right = true
    elseif key == KEYBOARD_CODES.KEY_E then self.states.up = true
    elseif key == KEYBOARD_CODES.KEY_Q then self.states.down = true
    end
end

function EditorCamera:onStopMove(key)
    if key == nil then
        self.states.forward = false
        self.states.backward = false
        self.states.left = false
        self.states.right = false
        self.states.up = false
        self.states.down = false
        return
    end

    if key == KEYBOARD_CODES.KEY_W then self.states.forward = false
    elseif key == KEYBOARD_CODES.KEY_S then self.states.backward = false
    elseif key == KEYBOARD_CODES.KEY_A then self.states.left = false
    elseif key == KEYBOARD_CODES.KEY_D then self.states.right = false
    elseif key == KEYBOARD_CODES.KEY_E then self.states.up = false
    elseif key == KEYBOARD_CODES.KEY_Q then self.states.down = false
    end
end

function EditorCamera:onDeltaRot(dx, dy)
    self.rot_coords.yaw = dx
    self.rot_coords.pitch = dy
end

function EditorCamera:onMoveSpeed(up_down)
    if up_down > 0 then
        self.movespeed = self.movespeed * 1.25
        self.movespeed = math.min(self.movespeed, 2.0)
    else
        self.movespeed = self.movespeed * 0.75
        self.movespeed = math.max(self.movespeed, 0.0001)
    end

end

function EditorCamera:Tick(dt)
    local move_dist = dt * self.movespeed

    local move_z = (self.states.forward and move_dist or 0.0) - (self.states.backward and move_dist or 0.0)
    local move_y = (self.states.up and move_dist or 0.0) - (self.states.down and move_dist or 0.0)
    local move_x = (self.states.right and move_dist or 0.0) - (self.states.left and move_dist or 0.0)

    local transformMat = self.cameraNode:GetTransform_L()
    local rotation = self.cameraNode:GetRotationPYR_L()

    local moveMat = Matrix.CreateTranslation(Vector3(move_x, move_y, move_z))
    moveMat = Matrix.Mul(moveMat, transformMat)

    self.cameraNode:SetTransform_L(moveMat)

    rotation.x = rotation.x - self.rot_coords.pitch * self.rotspeed
    rotation.y = rotation.y + self.rot_coords.yaw * self.rotspeed
    rotation.x = math.max( -math.pi * 0.49, math.min( rotation.x, math.pi * 0.49 ) )

    self.cameraNode:SetRotationPYR_L( rotation )

    self.rot_coords.yaw = 0
    self.rot_coords.pitch = 0
end
