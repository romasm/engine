GuiStyles.color_textfield = {
    styles = {
        GuiStyles.props_textfield,
        GuiStyles.int_textfield,
    },
    left = 289,
    width = 55,
    allow_none = false,
}

GuiStyles.color_string = {
    styles = {GuiStyles.string_props_01,},
    left = 270,
}

function Gui.ColorPicker()
return GuiWindow({
    styles = {
        GuiStyles.separete_window,
        GuiStyles.window_colors,
    },

    resizeable = {x = false, y = false},
    scrollable = {x = false, y = false},
    independent = false,
    
    width = 354,
    height = 340,

    id = 'color_picker',
    
    header = {
        styles = {
            GuiStyles.window_header,
        },
        str = "Color picker"
    },

    events = {
        [GUI_EVENTS.UNFOCUS] = function(self, ev) 
            if not self.entity:is_eq(ev.entity) then return false end
            self:Close()
            ColorPicker:SendEnd()
            local cursor = CoreGui.GetCursorPos()
            if not ColorPicker.caller.entity:IsCollide(cursor.x, cursor.y) then 
                if ColorPicker.caller.picker then ColorPicker.caller.picker = false end
            end
            return true
        end,
        [GUI_EVENTS.WIN_CLOSE] = function(self, ev) 
            if ColorPicker.caller.picker then ColorPicker.caller.picker = false end
            ColorPicker:SendEnd()
            return true
        end,
        [GUI_EVENTS.KEY_DOWN] = function(self, ev) 
            if ev.key == KEYBOARD_CODES.KEY_ESCAPE or
               ev.key == KEYBOARD_CODES.KEY_RETURN then 
                ColorPicker.caller.entity:SetHierarchyFocusOnMe(false)
                if ColorPicker.caller.picker then ColorPicker.caller.picker = false end
                CoreGui.SetHCursor(SYSTEM_CURSORS.ARROW)
                return true
            end
            return false
        end,
    },

    GuiClientarea({
    GuiBody({
        width = 100,
        width_percent = true,
        height = 100,
        height_percent = true,

    -- saturation value
    GuiRect({
        styles = {GuiStyles.ghost,},
        width = 198,
        height = 198,
        left = 5,
        top = 6,
        material = GuiMaterials.sv_picker,
        id = 'SV_rect',
    }),

    GuiButton({
        styles = {GuiStyles.common_button,},
        border = {
            color = 'text_06',
            color_hover = 'text_06',
            color_press = 'text_06',
            width = 1,
        },
        background = {
            color = 'null',
            color_hover = 'null',
            color_press = 'null',
        },
        text = {
            font = "",
        },
        cursor = SYSTEM_CURSORS.CROSS,
        fadein_time = 0,
        fadeout_time = 0,
        width = 200,
        height = 200,
        left = 4,
        top = 5,
        id = 'SV_btn',
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(true)
                ColorPicker.state_sv = true
                ColorPicker:onSVMove(self, ev)
                return true
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                ColorPicker.state_sv = false
                return true
            end,
            [GUI_EVENTS.BUTTON_MOVE] = function(self, ev) 
                ColorPicker:onSVMove(self, ev)
                return true
            end,
        },

        GuiRect({
            styles = {
                GuiStyles.ghost,
                GuiStyles.no_border,
            },
            width = 8,
            height = 8,
            border = {
                color = Vector4(1,1,1,1),
                width = 1,
            },
            background = {
                color = 'null',
            },
            id = 'SV_select',
        }),
    }),

    -- hue
    GuiRect({
        styles = {GuiStyles.ghost,},
        width = 28,
        height = 198,
        left = 215,
        top = 6,
        material = GuiMaterials.h_picker,
        id = 'H_rect',
    }),

    GuiButton({
        styles = {GuiStyles.common_button,},
        border = {
            color = 'text_06',
            color_hover = 'text_06',
            color_press = 'text_06',
            width = 1,
        },
        background = {
            color = 'null',
            color_hover = 'null',
            color_press = 'null',
        },
        text = {
            font = "",
        },
        cursor = SYSTEM_CURSORS.CROSS,
        fadein_time = 0,
        fadeout_time = 0,
        width = 30,
        height = 200,
        left = 214,
        top = 5,
        id = 'H_btn',
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(true)
                ColorPicker.state_h = true
                ColorPicker:onHMove(self, ev)
                return true
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                ColorPicker.state_h = false
                return true
            end,
            [GUI_EVENTS.BUTTON_MOVE] = function(self, ev) 
                ColorPicker:onHMove(self, ev)
                return true
            end,
        },

        GuiRect({
            styles = {
                GuiStyles.ghost,
                GuiStyles.no_border,
            },
            width = 30,
            height = 5,
            left = 0,
            border = {
                color = Vector4(1,1,1,1),
                width = 1,
            },
            background = {
                color = 'null',
            },
            id = 'H_select',
        }),
    }),

    -- colors
    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },
        width = 80,
        height = 40,
        left = 264,
        top = 5,
        border = {
            color = 'text_06',
            width = 1,
        },
        background = {
            color = 'null',
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },
        width = 39,
        height = 38,
        left = 265,
        top = 6,
        id = 'picked_color',
    }),

    GuiButton({
        styles = {
            GuiStyles.common_button,
            GuiStyles.no_border,
        },
        background = {
            color = 'null',
            color_hover = 'null',
            color_press = 'null',
        },
        icon = {
            material = GuiMaterials.pipet,
            rect = { l = 4, t = 4, w = 30, h = 30 },
        },
        text = {
            font = "",
        },
        fadein_time = 300,
        width = 39,
        height = 38,
        left = 265,
        top = 6,
        id = 'pipet',
        cursor = SYSTEM_CURSORS.CROSS,
        alt = "Hold and drag to pick color from screen",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                self.entity:SetHierarchyFocusOnMe(true)
                ColorPicker:StartPipet()
                return true
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
                self.entity:SetHierarchyFocusOnMe(false)
                ColorPicker:EndPipet()
                return true
            end,
            [GUI_EVENTS.BUTTON_MOVE] = function(self, ev)
                ColorPicker:GetPipetColor()
                return true
            end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },
        width = 39,
        height = 38,
        left = 304,
        top = 6,
        id = 'old_color',
    }),

    GuiButton({
        styles = {
            GuiStyles.common_button,
            GuiStyles.no_border,
        },
        background = {
            color = 'null',
            color_hover = 'null',
            color_press = 'null',
        },
        icon = {
            material = GuiMaterials.reset,
            rect = { l = 5, t = 4, w = 30, h = 30 },
        },
        text = {
            font = "",
        },
        fadein_time = 300,
        width = 39,
        height = 38,
        left = 304,
        top = 6,
        id = 'reset',
        alt = "Reset color",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                ColorPicker:Reset()
                return true
            end,
        },
    }),

    -- manual input
    GuiString({
        styles = {GuiStyles.color_string,},
        str = "H",
        top = 57,
    }),
    GuiTextfield({
        styles = {GuiStyles.color_textfield,},
        id = 'h_text',
        top = 55,
        data = {min = 0, max = 359,},

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 
                    ColorPicker:SetH(self:GetNum()) 
                    return true
                end,
        },
    }),

    GuiString({
        styles = {GuiStyles.color_string,},
        str = "S",
        top = 82,
    }),
    GuiTextfield({
        styles = {GuiStyles.color_textfield,},
        id = 's_text',
        top = 80,
        data = {min = 0, max = 100,},

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 
                    ColorPicker:SetS(self:GetNum()) 
                    return true
                end,
        },
    }),
    
    GuiString({
        styles = {GuiStyles.color_string,},
        str = "V",
        top = 107,
    }),
    GuiTextfield({
        styles = {GuiStyles.color_textfield,},
        id = 'v_text',
        top = 105,
        data = {min = 0, max = 100,},

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 
                    ColorPicker:SetV(self:GetNum()) 
                    return true
                end,
        },
    }),

    GuiString({
        styles = {GuiStyles.color_string,},
        str = "R",
        top = 137,
    }),
    GuiTextfield({
        styles = {GuiStyles.color_textfield,},
        id = 'r_text',
        top = 135,
        data = {min = 0, max = 255,},

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 
                    ColorPicker:SetR(self:GetNum()) 
                    return true
                end,
        },
    }),

    GuiString({
        styles = {GuiStyles.color_string,},
        str = "G",
        top = 162,
    }),
    GuiTextfield({
        styles = {GuiStyles.color_textfield,},
        id = 'g_text',
        top = 160,
        data = {min = 0, max = 255,},

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 
                    ColorPicker:SetG(self:GetNum()) 
                    return true
                end,
        },
    }),

    GuiString({
        styles = {GuiStyles.color_string,},
        str = "B",
        top = 187,
    }),
    GuiTextfield({
        styles = {GuiStyles.color_textfield,},
        id = 'b_text',
        top = 185,
        data = {min = 0, max = 255,},

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 
                    ColorPicker:SetB(self:GetNum()) 
                    return true
                end,
        },
    }),

    -- color temperature
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Color temperature, K",
        left = 5,
        top = 225,
    }),
    GuiTextfield({
        styles = {GuiStyles.color_textfield,},
        id = 't_text',
        top = 223,
        left = 144,
        width = 60,
        data = {min = 800, max = 40000,},

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 
                    ColorPicker:SetT(self:GetNum()) 
                    return true
                end,
        },
    }),

    GuiRect({
        styles = {GuiStyles.ghost,},
        width = 338,
        height = 38,
        left = 5,
        top = 251,
        material = GuiMaterials.t_picker,
        id = 'T_rect',
    }),

    GuiButton({
        styles = {GuiStyles.common_button,},
        border = {
            color = 'text_06',
            color_hover = 'text_06',
            color_press = 'text_06',
            width = 1,
        },
        background = {
            color = 'null',
            color_hover = 'null',
            color_press = 'null',
        },
        text = {
            font = "",
        },
        cursor = SYSTEM_CURSORS.CROSS,
        fadein_time = 0,
        fadeout_time = 0,
        width = 340,
        height = 40,
        left = 4,
        top = 250,
        id = 'T_btn',
        alt = "Corresponding colors for temperatures ( sRGB )",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(true)
                ColorPicker.state_t = true
                ColorPicker:onTMove(self, ev)
                return true
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                ColorPicker.state_t = false
                return true
            end,
            [GUI_EVENTS.BUTTON_MOVE] = function(self, ev) 
                ColorPicker:onTMove(self, ev)
                return true
            end,
        },

        GuiRect({
            styles = {
                GuiStyles.ghost,
                GuiStyles.no_border,
            },
            width = 5,
            height = 40,
            top = 0,
            border = {
                color = Vector4(0.5,0.5,0.5,1),
                width = 1,
            },
            background = {
                color = 'null',
            },
            id = 'T_select',
        }),
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "1000 K",
        left = 5,
        top = 291,
    }),
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "15000 K",
        right = 5,
        top = 291,
        align = GUI_ALIGN.RIGHT,
    }),

    }),
    }),
})
end