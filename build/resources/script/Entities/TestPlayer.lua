if not EntityTypes.TestPlayer then EntityTypes.TestPlayer = class(EntityTypes.BaseEntity) end

function EntityTypes.TestPlayer:init(world, ent)
    if not self:base(EntityTypes.TestPlayer).init(self, world, ent) then 
        self:initVars()
        return false
    end

    self:initVars()

    -- init
    self.world:SetEntityType(self.ent, "TestPlayer")
    self.transformSys:AddComponent(self.ent)
    
    -- script add after params defined
    self.world.script:AddComponent(self.ent, self)

    -- controller after script
    self.world.controller:AddComponent(self.ent, "FirstPerson")
    
    -- temp
    self.world.visibility:AddComponent(self.ent)
    self.world.staticMesh:AddComponentMesh(self.ent, "")
        
    self.world.physics:AddComponent(self.ent)
    self.world.physics:SetType(self.ent, PHYSICS_TYPES.KINEMATIC)

    -- camera attach
    self.camera = EntityTypes.Camera(self.world)
    self.camera:SetPosition(0.0, 1.8, 0.0)
    self.camera:SetFov(1.3)
    self.camera:Attach(self)

    return true
end

function EntityTypes.TestPlayer:initVars()
    -- params (ref in c++) "p_" - is a key
    self.p_jump_power = 0.3
    self.p_jump_speed = 2.0
    self.p_move_speed = 4.0
    self.p_rot_sence = 0.02

    -- lifetime only exist vars
    self.jumping = false
    self.jumpProgress = 0

    self.forward = false
    self.backward = false
    self.right = false
    self.left = false

    self.dYaw = 0
    self.dPitch = 0
end

function EntityTypes.TestPlayer:Activate()
    self.world.controller:SetActive(self.ent, true)
end

function EntityTypes.TestPlayer:Deactivate()
    self.world.controller:SetActive(self.ent, false)
end

-- tick
function EntityTypes.TestPlayer:onTick(dt)
    local dtSeconds = dt * 0.001

    if self.jumping == true then
        if self.jumpProgress > 1 then
            self.jumpProgress = 1
            self.jumping = false
        end
        
        local jumpDelta = self.p_jump_power * (0.5 - self.jumpProgress)
        self:AddPosition(0, jumpDelta, 0)

        self.jumpProgress = self.jumpProgress + dtSeconds * self.p_jump_speed
    end

    local moveDist = self.p_move_speed * dtSeconds

    if self.forward then self:AddPositionLocal(0, 0, moveDist) end
    if self.backward then self:AddPositionLocal(0, 0, -moveDist) end
    if self.right then self:AddPositionLocal(-moveDist, 0, 0) end
    if self.left then self:AddPositionLocal(moveDist, 0, 0) end

    if self.dYaw ~= 0 then
        self:AddRotation(0, self.dYaw * self.p_rot_sence, 0)
        self.dYaw = 0
    end
    
    if self.dPitch ~= 0 then
        local rotation = self.camera:GetRotationL()
        rotation.x = rotation.x + self.dPitch * self.p_rot_sence
        rotation.x = math.max( -math.pi * 0.5, math.min( rotation.x, math.pi * 0.5 ) )
        self.camera:SetRotation(rotation.x, 0, 0)
        self.dPitch = 0
    end
end

-- Controls
function EntityTypes.TestPlayer:onAction(key, pressed, x, y, z)
    print("Controller input!!!")
end

function EntityTypes.TestPlayer:onJump(key, pressed, x, y, z)
    if pressed == false then return end

    if self.jumping == false then
        self.jumping = true
        self.jumpProgress = 0
    end
end

function EntityTypes.TestPlayer:onMoveForward(key, pressed, x, y, z)
    self.forward = pressed
end

function EntityTypes.TestPlayer:onMoveBackward(key, pressed, x, y, z)
    self.backward = pressed
end

function EntityTypes.TestPlayer:onMoveLeft(key, pressed, x, y, z)
    self.right = pressed
end

function EntityTypes.TestPlayer:onMoveRight(key, pressed, x, y, z)
    self.left = pressed
end

function EntityTypes.TestPlayer:onTurnYaw(key, pressed, x, y, z)
    self.dYaw = y
end

function EntityTypes.TestPlayer:onTurnPitch(key, pressed, x, y, z)
    self.dPitch = x
end