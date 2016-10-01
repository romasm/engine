if not TestEntCallback then TestEntCallback = {} end

function TestEntCallback.StartSpeed(self, ev)
    self.history = {
        s_oldval = {},
        s_newval = {},

        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
                    lua_entity.p_rot_speed = self.s_oldval[i]
                end
                Properties:UpdateData(false, COMPONENTS.SCRIPT)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
                    lua_entity.p_rot_speed = self.s_newval[i]
                end
                Properties:UpdateData(false, COMPONENTS.SCRIPT)
            end,
        msg = "Speed changed"
    }

    local speed = self:GetValue()
    for i, ent in ipairs(Viewport.selection_set) do
        local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)

        self.history.s_oldval[i] = lua_entity.p_rot_speed

        if speed then
            lua_entity.p_rot_speed = speed
        end
    end
    return true 
end

function TestEntCallback.DragSpeed(self, ev)
    local speed = self:GetValue()
    if speed == nil then return true end

    for i, ent in ipairs(Viewport.selection_set) do
        local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
        lua_entity.p_rot_speed = speed
    end
    return true
end

function TestEntCallback.EndSpeed(self, ev)
    local speed = self:GetValue()
    if speed == nil then return true end
    
    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        self.history.s_newval[i] = speed
        is_change = is_change or not CMath.IsNearlyEq(self.history.s_oldval[i], self.history.s_newval[i], 0.001)
    end
    if not is_change then return true end
    
    for i, ent in ipairs(Viewport.selection_set) do
        local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
        lua_entity.p_rot_speed = self.history.s_newval[i]
    end
    
    History:Push(self.history)
    return true
end

function TestEntCallback.UpdSpeed(self, ev)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
        local speed = lua_entity.p_rot_speed
        if i > 1 and val ~= speed then
            self:SetValue(nil)
            return true
        else val = speed end
    end
    self:SetValue( val )
    return true 
end