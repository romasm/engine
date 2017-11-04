function Gui.AddCollisionMask(self, group)
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.collision:AddCollisionMask(ent, group)
        Viewport.lua_world.world:UpdateCollision(ent)
    end
    return true
end

function Gui.RemCollisionMask(self, group)
    for i, ent in ipairs(Viewport.selection_set) do
        Viewport.lua_world.world.collision:RemoveCollisionMask(ent, group)
        Viewport.lua_world.world:UpdateCollision(ent)
    end
    return true
end

function Gui.HasCollisionMask(self, group)
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

GuiStyles.coll_check1 = {
    styles = {GuiStyles.props_check,},
    left = 10,
    width = 100,
    height = 18,
}

GuiStyles.coll_check2 = {
    styles = {GuiStyles.props_check,},
    left = 150,
    width = 100,
    height = 18,
}

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

    height = 242,

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
            "NoCollide",
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

                if selected == 1 then selected = COLLISION_GROUPS.NoCollide 
                else selected = math.pow(2, selected - 2) end

                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.collision:SetCollisionGroup(ent, selected)
                    Viewport.lua_world.world:UpdateCollision(ent)
                end
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                    local value = 0
                    local undef = false
                    for i, ent in ipairs(Viewport.selection_set) do
                        local group = Viewport.lua_world.world.collision:GetCollisionGroup(ent)
                        if i > 1 and value ~= group then 
                            self:SetSelected(0)
                            return true
                        else value = group end
                    end
                    
                    if value ~= 0 then
                        if value ~= COLLISION_GROUPS.NoCollide then value = math.floor( math.log10(value) / math.log10(2) + 0.5 ) + 2 
                        else value = 1 end
                    end

                    self:SetSelected( value )
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
        styles = {GuiStyles.coll_check1,},
        top = 83,
        text = { str = "Default" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Default) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Default) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Default) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check1,},
        top = 104,
        text = { str = "Static" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Static) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Static) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Static) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check1,},
        top = 126,
        text = { str = "Kinematic" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Kinematic) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Kinematic) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Kinematic) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check1,},
        top = 148,
        text = { str = "Debris" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Debris) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Debris) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Debris) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check1,},
        top = 170,
        text = { str = "Trigger" },
        
        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Trigger) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Trigger) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Trigger) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check1,},
        top = 192,
        text = { str = "Character" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Character) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Character) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Character) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check1,},
        top = 214,
        text = { str = "Gamelogic" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Gamelogic) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Gamelogic) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Gamelogic) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check2,},
        top = 83,
        text = { str = "User0" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Special0) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Special0) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Special0) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check2,},
        top = 104,
        text = { str = "User1" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Special1) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Special1) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Special1) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check2,},
        top = 126,
        text = { str = "User2" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Special2) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Special2) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Special2) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check2,},
        top = 148,
        text = { str = "User3" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Special3) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Special3) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Special3) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check2,},
        top = 170,
        text = { str = "User4" },
        
        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Special4) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Special4) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Special4) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.coll_check2,},
        top = 192,
        text = { str = "User5" },

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return Gui.AddCollisionMask(self, COLLISION_GROUPS.Special5) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return Gui.RemCollisionMask(self, COLLISION_GROUPS.Special5) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return Gui.HasCollisionMask(self, COLLISION_GROUPS.Special5) end,
        }
    }),
})
end