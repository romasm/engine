if not EntityTypes.TestPlayer then EntityTypes.TestPlayer = class(EntityTypes.BaseEntity) end

function EntityTypes.TestPlayer:init(world, ent)
    if not self:base(EntityTypes.TestPlayer).init(self, world, ent) then 
        self:initVars()
        return false
    end

    self:initVars()
    
    self.physicsSys = self.world.physics

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
        
    -- collision
    self.physicsSys:AddComponent(self.ent)    
    self.physicsSys:AddCapsuleShape(self.ent, Vector3.Zero, Quaternion.Identity, 75.0, 0.3, 1.8)
    self.physicsSys:SetNonRotatable(self.ent, true)
    self.physicsSys:SetUnsleepable(self.ent, true)
    self.physicsSys:SetBounciness(self.ent, 0.0)
    self.physicsSys:SetFriction(self.ent, 0.9)
    self.physicsSys:SetVelocityDamping(self.ent, 0.5)

    self.physicsSys:SetActive(self.ent, true)

    -- camera attach
    self.camera = EntityTypes.Camera(self.world)
    self.camera:SetPosition(0.0, 0.8, 0.0)
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
        self.physicsSys:ApplyForceToCenterOfMass(self.ent, Vector3(0, 25000.0, 0))
        self.physicsSys:SetVelocity(self.ent, Vector3(0, 0, self.p_move_speed))
        self.jumping = false
    end
    
    local camDir = self.camera:GetDirectionL()
    camDir.y = 0
    local sideDir = camDir
    sideDir.z = -sideDir.z



    if self.forward then self.physicsSys:SetVelocity(self.ent, Vector3(0, 0, self.p_move_speed)) end
    if self.backward then self.physicsSys:SetVelocity(self.ent, Vector3(0, 0, -self.p_move_speed)) end
    if self.right then self.physicsSys:SetVelocity(self.ent, Vector3(-self.p_move_speed, 0, 0)) end
    if self.left then self.physicsSys:SetVelocity(self.ent, Vector3(self.p_move_speed, 0, 0)) end
        
    if self.dPitch ~= 0 or self.dYaw ~= 0 then
        local rotation = self.camera:GetRotationL()
        rotation.x = rotation.x + self.dPitch * self.p_rot_sence
        rotation.x = math.max( -math.pi * 0.5, math.min( rotation.x, math.pi * 0.5 ) )
        rotation.y = rotation.y + self.dYaw * self.p_rot_sence
        self.camera:SetRotation(rotation.x, rotation.y, 0)
        self.dPitch = 0
        self.dYaw = 0
    end
end

-- Controls
function EntityTypes.TestPlayer:onAction(key, pressed, x, y, z)
    print("Controller input!!!")
end

function EntityTypes.TestPlayer:onJump(key, pressed, x, y, z)
    if pressed == false then return end
    self.jumping = true
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