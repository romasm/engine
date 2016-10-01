if not EntityTypes.TestEnt then EntityTypes.TestEnt = class(EntityTypes.BaseEntity) end

function EntityTypes.TestEnt:init(world, ent)
    if not self._base.init(self, world, ent) then 
        self:initVars()
        return 
    end

    self:initVars()

    -- init
    self.world:SetEntityType(self.ent, "TestEnt")
    self.transformSys:AddComponent(self.ent)
    
    -- script add after params defined
    self.world.script:AddComponent(self.ent, self)

    -- temp
    self.world.visibility:AddComponent(self.ent)
    self.world.staticMesh:AddComponentMesh(self.ent, "")
    self.world.staticMesh:SetEditor(self.ent, true)
end

function EntityTypes.TestEnt:initVars()
    -- params (ref in c++) "p_" - is a key
    self.p_rot_speed = 1.0

    -- lifetime only exist vars
    self.current_rot = 0
end

function EntityTypes.TestEnt:onTick(dt)
    local rot = self:GetRotationL()
    self.current_rot = rot.y + self.p_rot_speed * 0.001 * dt
    self:SetRotation(rot.x, self.current_rot, rot.z)
end