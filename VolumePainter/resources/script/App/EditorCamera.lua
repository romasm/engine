if not EditorCamera then EditorCamera = {} end

function EditorCamera:Init( world )
    print("EditorCamera:Init") 
    
    self.world = world

    local nodeName = EDITOR_VARS.INVIS.."CameraNode"
    local nodeEnt = self.world:GetEntityByName(nodeName)
    if nodeEnt:IsNull() then 
        self.cameraNode = EntityTypes.Node(self.world)
        self.world:RenameEntity(self.cameraNode.ent, nodeName)
    else
        self.cameraNode = EntityTypes.wrap(self.world, nodeEnt)
	end
	
	local worldNodeName = EDITOR_VARS.INVIS.."WorldNode"
	local worldNodeEnt = self.world:GetEntityByName(worldNodeName)
	if worldNodeEnt:IsNull() then 
        self.worldNode = EntityTypes.Node(self.world)
		self.world:RenameEntity(self.worldNode.ent, worldNodeName)
	else
		self.worldNode = EntityTypes.wrap(self.world, worldNodeEnt)
    end
    
    self.camera = EntityTypes.Camera(self.world)
    self.world:RenameEntity(self.camera.ent, EDITOR_VARS.TYPE.."Camera")
    self.camera:Attach(self.cameraNode)
    self.camera:SetFov(1.0)
    self.camera:SetFar(10000.0)
    self.camera:SetNear(0.1)

	self.cameraEntity = self.camera.ent
		
	-- from config
	self.movespeed = 0.005
	self.rotspeed = 0.01

	self.orbitRotSpeed = 0.004
	self.orbitZoomSpeed = 0.1

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

	self.orbitZoom = 1.5 
	   
	self:SwitchMode(0)
end

function EditorCamera:SwitchMode(mode) -- 0 - orbit, 1 - free
	self.mode = mode

	if self.mode == 0 then
		self.camera:Attach(self.worldNode)
		self.camera:SetPosition_L3F(0.0, 0.0, -self.orbitZoom)
		self.worldNode:SetRotationPYR_L3F(0.68, -2.34, 0.0)
	else
		self.camera:Attach(self.cameraNode)
		self.camera:SetPosition_L3F(0.0, 0.0, 0.0)
		self.cameraNode:SetPosition_L3F(1.0, 1.0, 1.0)
		self.cameraNode:SetRotationPYR_L3F(0.68, -2.34, 0.0)
	end


end

function EditorCamera:GetPosition()
    return self.camera:GetPosition_W()
end

function EditorCamera:GetDirection()
    return self.camera:GetLookDir()
end

function EditorCamera:Close()
    self.world = nil
    self.cameraEntity = nil
    self.camera = nil
	self.cameraNode = nil
	self.worldNode = nil
end

function EditorCamera:onStartMove(key)
	if self.mode == 0 then return end

	if key == KEYBOARD_CODES.KEY_W then self.states.forward = true
    elseif key == KEYBOARD_CODES.KEY_S then self.states.backward = true
    elseif key == KEYBOARD_CODES.KEY_A then self.states.left = true
    elseif key == KEYBOARD_CODES.KEY_D then self.states.right = true
    elseif key == KEYBOARD_CODES.KEY_E then self.states.up = true
    elseif key == KEYBOARD_CODES.KEY_Q then self.states.down = true
    end
end

function EditorCamera:onStopMove(key)
	if self.mode == 0 then return end

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

function EditorCamera:onWheel(up_down)
	if self.mode == 0 then
		self.orbitZoom = self.orbitZoom + up_down * self.orbitZoomSpeed
		self.orbitZoom = math.max( 0.1, math.min( self.orbitZoom, 3.0 ) )
		self.camera:SetPosition_L3F( 0.0, 0.0, -self.orbitZoom )
	else
		if up_down > 0 then
			self.movespeed = self.movespeed * 1.25
			self.movespeed = math.min(self.movespeed, 2.0)
		else
			self.movespeed = self.movespeed * 0.75
			self.movespeed = math.max(self.movespeed, 0.00001)
		end
	end
end

function EditorCamera:Tick(dt)
	if self.mode == 0 then
		local rotation = self.worldNode:GetRotationPYR_L()
		rotation.x = rotation.x - self.rot_coords.pitch * self.orbitRotSpeed
		rotation.y = rotation.y + self.rot_coords.yaw * self.orbitRotSpeed
		rotation.x = math.max( -math.pi * 0.49, math.min( rotation.x, math.pi * 0.49 ) )

		self.worldNode:SetRotationPYR_L( rotation )
	else
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
	end
	
	self.rot_coords.yaw = 0
    self.rot_coords.pitch = 0
end
