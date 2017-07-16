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
            
    -- collision
    self.physicsSys:AddComponent(self.ent)    
    self.physicsSys:AddCapsuleShape(self.ent, Vector3.Zero, Quaternion.Identity, 75.0, 0.3, 1.2)
    self.physicsSys:SetNonRotatable(self.ent, true)
    self.physicsSys:SetUnsleepable(self.ent, true)
    self.physicsSys:SetBounciness(self.ent, 0.0)
    self.physicsSys:SetFriction(self.ent, 1.0)
    --self.physicsSys:SetVelocityDamping(self.ent, 0.9)

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
    self.p_jump_accel = 25000.0
    self.p_move_accel = 100.0
    self.p_move_max_speed = 1.5
    self.p_rot_sence = 0.02

    -- lifetime only exist vars
    self.forward = 0
    self.backward = 0
    self.right = 0
    self.left = 0

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
    if self.forward + self.backward + self.left + self.right > 0 then
        local forwardDir = self.camera:GetLookDir()
        forwardDir.y = 0
        forwardDir:Normalize()

        local rightDir = self.camera:GetLookTangent()
        rightDir.y = 0
        rightDir:Normalize()

        local backwardDir = Vector3.Inverse(forwardDir)
        local leftDir = Vector3.Inverse(rightDir)
    
        forwardDir = Vector3.MulScalar(forwardDir, self.forward)
        backwardDir = Vector3.MulScalar(backwardDir, self.backward)
        leftDir = Vector3.MulScalar(leftDir, self.left)
        rightDir = Vector3.MulScalar(rightDir, self.right)
    
        local unitDir = Vector3.Add(forwardDir, backwardDir)
        unitDir = Vector3.Add(unitDir, leftDir)
        unitDir = Vector3.Add(unitDir, rightDir)
        unitDir:Normalize()

        local velocity = self.physicsSys:GetVelocity(self.ent)
        local speedOnMoveDir = velocity:Dot(unitDir)

        if speedOnMoveDir < self.p_move_max_speed then
            local moveDir = Vector3.MulScalar(unitDir, self.p_move_accel * dt)
            self.physicsSys:ApplyForceToCenterOfMass(self.ent, moveDir)
        end
    end
    
    if self.dPitch ~= 0 or self.dYaw ~= 0 then
        local rotation = self.camera:GetRotationL()
        rotation.x = rotation.x + self.dPitch * self.p_rot_sence
        rotation.x = math.max( -math.pi * 0.499, math.min( rotation.x, math.pi * 0.499 ) )
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
    self.physicsSys:ApplyForceToCenterOfMass(self.ent, Vector3(0, self.p_jump_accel, 0))
end

function EntityTypes.TestPlayer:onMoveForward(key, pressed, x, y, z)
    self.forward = pressed and 1 or 0
end

function EntityTypes.TestPlayer:onMoveBackward(key, pressed, x, y, z)
    self.backward = pressed and 1 or 0
end

function EntityTypes.TestPlayer:onMoveLeft(key, pressed, x, y, z)
    self.right = pressed and 1 or 0
end

function EntityTypes.TestPlayer:onMoveRight(key, pressed, x, y, z)
    self.left = pressed and 1 or 0
end

function EntityTypes.TestPlayer:onTurnYaw(key, pressed, x, y, z)
    self.dYaw = y
end

function EntityTypes.TestPlayer:onTurnPitch(key, pressed, x, y, z)
    self.dPitch = x
end