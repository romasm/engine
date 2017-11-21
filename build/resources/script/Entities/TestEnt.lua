if not EntityTypes.TestEnt then EntityTypes.TestEnt = class(EntityTypes.BaseEntity) end

function EntityTypes.TestEnt:init(world, ent)
    if not self:base(EntityTypes.TestEnt).init(self, world, ent) then 
        self:initVars()
        return false
    end

    self:initVars()

    -- init
    self.world:SetEntityType(self.ent, "TestEnt")
    self.transformSys:AddComponent(self.ent)
    
    -- script add after params defined
    self.world.script:AddComponent(self.ent, self)

    -- collision
    self.world.collision:AddComponent(self.ent, false)
    self.world.collision:AddBoxCollider(self.ent, Vector3.Zero, Quaternion.Identity, Vector3(2, 2, 2))

    self.world.trigger:AddComponent(self.ent)
    self.world.trigger:SetFilterType(self.ent, 1)
    self.world.trigger:SetFilterString(self.ent, "TestPlayer")

    self.world.trigger:UpdateState(self.ent)

    return true
end

function EntityTypes.TestEnt.testTrigger(state)
    local light = Viewport.lua_world.world:GetLuaEntity(Viewport.lua_world.world:GetEntityByName("trig_light"))
    if state == true then light:Enable()
    else light:Disable() end
end

function EntityTypes.TestEnt:initVars()
    -- params (ref in c++) "p_" - is a key
    self.p_onStartTouch = function(self, activator, time) end
    self.p_onEndTouch = function(self, activator, time) end
    self.p_onEndTouchAll = function(self) end

    -- lifetime only exist vars
    self.current_time = 0
end
--[[
function EntityTypes.TestEnt:onTick(dt)
    self.current_time = self.current_time + 0.001 * dt
    self:SetPosition(math.sin(self.current_time) * 5.0, 0, 0)
end
--]]
