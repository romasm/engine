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
    self.physicsSys:SetFriction(self.ent, 0.5)
    self.physicsSys:SetLinearDamping(self.ent, self.p_air_dumping)
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
        
    self.light:Disable()

    return true
end

function EntityTypes.TestPlayer:initVars()
    -- params (ref in c++) "p_" - is a key
    self.p_jump_impulse = 450.0

    self.p_move_accel = 2000.0
    self.p_move_in_jump_accel = 180.0
    self.p_run_accel = 3500.0

    self.p_floor_dumping = 0.9999
    self.p_air_dumping = 0.25

    self.p_ground_sence = 0.04

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
    self.jumping = false
    self.inAir = false

    self.dYaw = 0
    self.dPitch = 0
end

-- TODO: check normals of ground
function EntityTypes.TestPlayer:InAir()
    local rayStart0 = Vector3.Add(self:GetPosition_W(), Vector3(0, -(self.p_player_height * 0.5 - self.p_player_radius), 0))
    local rayEnd0 = Vector3.Add(rayStart0, Vector3(0, -(self.p_player_radius + self.p_ground_sence), 0))

    local rayTest = self.collisionSys:RayCast(rayStart0, rayEnd0, COLLISION_GROUPS.Character, COLLISION_GROUPS.RayCastPhysicsNoChar)
    local result = rayTest.hit
    
    local sideOffset = self.p_player_radius * 0.7
    
    local offset1 = Vector3(sideOffset, 0, 0)
    local rayStart1 = Vector3.Add(rayStart0, offset1)
    local rayEnd1 = Vector3.Add(rayEnd0, offset1)
    rayTest = self.collisionSys:RayCast(rayStart1, rayEnd1, COLLISION_GROUPS.Character, COLLISION_GROUPS.RayCastPhysicsNoChar)
    result = result or rayTest.hit

    local offset2 = Vector3(-sideOffset, 0, 0)
    local rayStart2 = Vector3.Add(rayStart0, offset2)
    local rayEnd2 = Vector3.Add(rayEnd0, offset2)
    rayTest = self.collisionSys:RayCast(rayStart2, rayEnd2, COLLISION_GROUPS.Character, COLLISION_GROUPS.RayCastPhysicsNoChar)
    result = result or rayTest.hit
    
    local offset3 = Vector3(0, 0, sideOffset)
    local rayStart3 = Vector3.Add(rayStart0, offset3)
    local rayEnd3 = Vector3.Add(rayEnd0, offset3)
    rayTest = self.collisionSys:RayCast(rayStart3, rayEnd3, COLLISION_GROUPS.Character, COLLISION_GROUPS.RayCastPhysicsNoChar)
    result = result or rayTest.hit
    
    local offset4 = Vector3(0, 0, -sideOffset)
    local rayStart4 = Vector3.Add(rayStart0, offset4)
    local rayEnd4 = Vector3.Add(rayEnd0, offset4)
    rayTest = self.collisionSys:RayCast(rayStart4, rayEnd4, COLLISION_GROUPS.Character, COLLISION_GROUPS.RayCastPhysicsNoChar)
    result = result or rayTest.hit
    
    return not result
end

-- tick
function EntityTypes.TestPlayer:onTick(dt) 
    self.inAir = self:InAir()
    
    if self.jumping == true then 
        self.physicsSys:SetLinearDamping(self.ent, self.p_air_dumping)
        self.physicsSys:ApplyCentralImpulse(self.ent, Vector3(0, self.p_jump_impulse, 0)) 
        self.jumping = false
        self.inAir = true
    end

    if self.inAir == true then
        self.physicsSys:SetLinearDamping(self.ent, self.p_air_dumping)
    else
        self.physicsSys:SetLinearDamping(self.ent, self.p_floor_dumping)
    end
        
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

        local accel = 0
        if self.inAir then
            accel = self.p_move_in_jump_accel
        else
            if self.running == true then 
                accel = self.p_run_accel
            else 
                accel = self.p_move_accel
            end
        end

        local moveDir = Vector3.MulScalar(unitDir, accel)
        self.physicsSys:ApplyCentralForce(self.ent, moveDir)
    end

    if self.dPitch ~= 0 or self.dYaw ~= 0 then
        local rotation = self.camera:GetRotationPYR_L()
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
    if pressed == true then 
        if self.light:IsEnabled() then self.light:Disable()
        else self.light:Enable() end
    end
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
    if not self.inAir then self.jumping = true end
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