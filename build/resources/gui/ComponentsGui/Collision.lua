function Gui.AddCollisionGroup(self, group)
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.collision:AddCollisionMask(ent, group)
    end
    return true
end

function Gui.RemCollisionGroup(self, group)
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.collision:RemoveCollisionMask(ent, group)
    end
    return true
end

function Gui.HasCollisionGroup(self, group)
    local hasGroup = false
    for i, ent in ipairs(Viewport.selection_set) do
        local cast = Viewport.lua_world.world.collision:HasCollisionMask(ent, group)
        if i > 1 and hasGroup ~= cast then
            self:SetCheck(nil)
            return true
        else hasGroup = cast end
    end
    self:SetCheck(hasGroup)
    return true
end

function Gui.CollisionComp()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    
    id = "collision",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 21,

        text = {
            str = "Collision",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 265,

    -- color
    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Group",
        left = 10,
        top = 33,
    }),
    
    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = true,
        left = 120,
        top = 30,
        width = 155,
        height = 21,
        list = {
            "None",
            "Default",
            "Static",
            "Kinematic",
            "Debris",
            "Trigger",
            "Character",
            "Gamelogic",
            "User0",
            "User1",
            "User2",
            "User3",
            "User4",
            "User5",
        },
        alt = "Collision group of entity",

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev)
                local selected = self:GetSelected()
                if selected < 0 then return true end
                selected = math.pow(2, selected - 1)                
                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.collision:SetCollisionGroup(ent, selected)
                end
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                    local val = 0
                    local undef = false
                    for i, ent in ipairs(Viewport.selection_set) do
                        local group = Viewport.lua_world.world.light:GetCollisionGroup(ent)
                        if i > 1 and val ~= group then 
                            self:SetSelected(0)
                            return true
                        else val = group end
                    end
                    self:SetSelected(val)
                return true 
            end,
        },
    }),

    -- brightness
    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Collide with:",
        left = 10,
        top = 63,
    }),
        -- TODO: help hover btn "Collision mask (with what collision groups checking should be performed)",

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 85,
        width = 60,
        height = 18,
        text = { str = "Default" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionGroup(self, COLLISION_GROUPS.Default) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionGroup(self, COLLISION_GROUPS.Default) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionGroup(self, COLLISION_GROUPS.Default) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 110,
        width = 60,
        height = 18,
        text = { str = "Static" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionGroup(self, COLLISION_GROUPS.Static) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionGroup(self, COLLISION_GROUPS.Static) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionGroup(self, COLLISION_GROUPS.Static) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 135,
        width = 60,
        height = 18,
        text = { str = "Kinematic" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionGroup(self, COLLISION_GROUPS.Kinematic) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionGroup(self, COLLISION_GROUPS.Kinematic) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionGroup(self, COLLISION_GROUPS.Kinematic) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 160,
        width = 60,
        height = 18,
        text = { str = "Debris" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionGroup(self, COLLISION_GROUPS.Debris) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionGroup(self, COLLISION_GROUPS.Debris) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionGroup(self, COLLISION_GROUPS.Debris) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 185,
        width = 60,
        height = 18,
        text = { str = "Trigger" },
        
        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionGroup(self, COLLISION_GROUPS.Trigger) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionGroup(self, COLLISION_GROUPS.Trigger) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionGroup(self, COLLISION_GROUPS.Trigger) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 210,
        width = 60,
        height = 18,
        text = { str = "Character" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionGroup(self, COLLISION_GROUPS.Character) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionGroup(self, COLLISION_GROUPS.Character) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionGroup(self, COLLISION_GROUPS.Character) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 235,
        width = 60,
        height = 18,
        text = { str = "Gamelogic" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionGroup(self, COLLISION_GROUPS.Gamelogic) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionGroup(self, COLLISION_GROUPS.Gamelogic) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionGroup(self, COLLISION_GROUPS.Gamelogic) end,
        }
    }),
})
end