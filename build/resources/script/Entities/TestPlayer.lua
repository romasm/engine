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
    self.collisionSys = self.world.collision
    self.collisionSys:AddComponent(self.ent, false)
    local capsuleHeight = self.p_player_height - self.p_player_radius * 2
    self.collisionSys:AddCapsuleCollider(self.ent, Vector3.Zero, Quaternion.Identity, self.p_player_radius, capsuleHeight)
    
    self.physicsSys:AddComponent(self.ent) 
    self.physicsSys:SetType(self.ent, PHYSICS_TYPES.DYNAMIC)
    self.physicsSys:SetAngularFactor(self.ent, Vector3(0, 0, 0))
    self.physicsSys:SetMass(self.ent, 80)
    self.physicsSys:SetRestitution(self.ent, 0.0)
    self.physicsSys:SetFriction(self.ent, 1.0)
    self.physicsSys:SetEnable(self.ent, true, true)
    self.physicsSys:UpdateState(self.ent)

    -- camera attach
    self.camera = EntityTypes.Camera(self.world)
    self.camera:SetPosition(0.0, 0.8, 0.0)
    self.camera:SetFov(1.3)
    self.camera:Attach(self)
    
    -- light attach
    self.light = EntityTypes.LocalLight(self.world)
    self.light:SetPosition(0.3, -0.3, 0.1)
    self.light:SetRotation(-1.57, 0, 0)
    self.light:Attach(self.camera)

    self.light.lightSys:SetType(self.light.ent, 3)
    self.light.lightSys:SetCastShadows(self.light.ent, true)

    self.light:SetCone(30, 60)

    self.light.lightSys:UpdateLightProps(self.light.ent)
    self.light.lightSys:UpdateShadows(self.light.ent)
        
    self.light:Enable(false)

    return true
end

function EntityTypes.TestPlayer:initVars()
    -- params (ref in c++) "p_" - is a key
    self.p_jump_accel = 25000.0

    self.p_move_accel = 200.0
    self.p_move_max_speed = 2.0
    self.p_move_in_jump_max_speed = 0.5
    self.p_move_in_jump_accel = 30.0
    self.p_run_max_speed = 4.0
    self.p_run_accel = 400.0

    self.p_rot_sence = 0.002

    self.p_player_radius = 0.3
    self.p_player_height = 1.8

    self.p_onTick = function(self, dt) EntityTypes.TestPlayer.onTick(self, dt) end

    -- lifetime only exist vars
    self.forward = 0
    self.backward = 0
    self.right = 0
    self.left = 0
    self.running = false
    self.inAir = false

    self.dYaw = 0
    self.dPitch = 0
end

function EntityTypes.TestPlayer:Activate()
    self.world.controller:SetActive(self.ent, true)
end

function EntityTypes.TestPlayer:Deactivate()
    self.world.controller:SetActive(self.ent, false)
end

function EntityTypes.TestPlayer:InAir()
    local rayStart = self:GetPositionW()
    rayStart = Vector3.Add(rayStart, Vector3(0, -(self.p_player_height / 2 - 0.1), 0))
    local rayEnd = Vector3.Add(rayStart, Vector3(0, -0.11, 0))

    local rayTest = self.collisionSys:RayCast(rayStart, rayEnd)
    local result = rayTest.hit

    local sideOffset = self.p_player_radius * 0.7
    local tempOffset = - 2 * sideOffset

    rayStart = Vector3.Add(rayStart, Vector3(sideOffset, 0, 0))
    rayEnd = Vector3.Add(rayEnd, Vector3(sideOffset, 0, 0))
    rayTest = self.collisionSys:RayCast(rayStart, rayEnd)
    result = result or rayTest.hit

    rayStart = Vector3.Add(rayStart, Vector3(tempOffset, 0, 0))
    rayEnd = Vector3.Add(rayEnd, Vector3(tempOffset, 0, 0))
    rayTest = self.collisionSys:RayCast(rayStart, rayEnd)
    result = result or rayTest.hit

    rayStart = Vector3.Add(rayStart, Vector3(sideOffset, 0, sideOffset))
    rayEnd = Vector3.Add(rayEnd, Vector3(sideOffset, 0, sideOffset))
    rayTest = self.collisionSys:RayCast(rayStart, rayEnd)
    result = result or rayTest.hit

    rayStart = Vector3.Add(rayStart, Vector3(0, 0, tempOffset))
    rayEnd = Vector3.Add(rayEnd, Vector3(0, 0, tempOffset))
    rayTest = self.collisionSys:RayCast(rayStart, rayEnd)
    result = result or rayTest.hit

    return not result
end

-- tick
function EntityTypes.TestPlayer:onTick(dt) 
    self.inAir = self:InAir()
    
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

        local velocity = self.physicsSys:GetLinearVelocity(self.ent)
        local speedOnMoveDir = velocity:Dot(unitDir)

        local maxSpeed = 0
        local accel = 0
        if self.inAir then
            maxSpeed = self.p_move_in_jump_max_speed
            accel = self.p_move_in_jump_accel
        else
            if self.running == true then 
                maxSpeed = self.p_run_max_speed
                accel = self.p_run_accel
            else 
                maxSpeed = self.p_move_max_speed 
                accel = self.p_move_accel
            end
        end

        if speedOnMoveDir < maxSpeed then
            local moveDir = Vector3.MulScalar(unitDir, accel * dt)
            self.physicsSys:ApplyCentralForce(self.ent, moveDir)
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
    
end

function EntityTypes.TestPlayer:onLight(key, pressed, x, y, z)
    if pressed == true then self.light:Enable(not self.light:IsEnabled()) end
end

function EntityTypes.TestPlayer:onRun(key, pressed, x, y, z)
    if pressed == true then
        self.running = true
    else
        self.running = false
    end
end

function EntityTypes.TestPlayer:onFire(key, pressed, x, y, z)
    if pressed == true then
        Viewport:SpawnPhysics(self.camera)
    end
end

function EntityTypes.TestPlayer:onJump(key, pressed, x, y, z)
    if pressed == false then return end
    if not self.inAir then self.physicsSys:ApplyCentralForce(self.ent, Vector3(0, self.p_jump_accel, 0)) end
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
    self.dYaw = self.dYaw + y
end

function EntityTypes.TestPlayer:onTurnPitch(key, pressed, x, y, z)
    self.dPitch = self.dPitch + x
end