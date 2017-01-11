if not EntityTypes.TestPlayer then EntityTypes.TestPlayer = class(EntityTypes.BaseEntity) end

function EntityTypes.TestPlayer:init(world, ent)
    if not self._base.init(self, world, ent) then 
        self:initVars()
        return 
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
    self.world.staticMesh:SetEditor(self.ent, true)

    self:Activate()
end

function EntityTypes.TestPlayer:initVars()
    -- params (ref in c++) "p_" - is a key
    self.p_jump_power = 0.15

    -- lifetime only exist vars
    self.jumping = false
    self.jumpProgress = 0
end

function EntityTypes.TestPlayer:Activate()
    self.world.controller:SetActive(self.ent, true)
end

function EntityTypes.TestPlayer:Deactivate()
    self.world.controller:SetActive(self.ent, false)
end

-- tick
function EntityTypes.TestPlayer:onTick(dt)
    if self.jumping == true then
        self.jumpProgress = self.jumpProgress + dt / 1000
        if self.jumpProgress > 1 then
            self.jumpProgress = 1
            self.jumping = false
        end
        
        local jumpDelta = self.p_jump_power * math.sin( (0.5 - self.jumpProgress) * math.pi )
        self:AddPosition(0, jumpDelta, 0)
    end
end

-- Controls
function EntityTypes.TestPlayer:onAction(key, pressed, x, y, z)
    print("Controller input")
end

function EntityTypes.TestPlayer:onJump(key, pressed, x, y, z)
    if pressed == false then return end
    self.jumping = true
    self.jumpProgress = 0
end