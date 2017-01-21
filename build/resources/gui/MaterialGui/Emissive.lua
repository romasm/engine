function Gui.MaterialEmissive()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "emissive",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "Emissive",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    events = {
        [GUI_EVENTS.MOUSE_DOWN] = function(self, ev) 
            self.entity:SetHierarchyFocusOnMe(false)
            self.entity:SetFocus(HEntity())

            if ev.key == KEYBOARD_CODES.KEY_RBUTTON then
                MaterialProps:OpenMenu(self, ev.coords)
            end
            return true
        end,
        [GUI_EVENTS.MENU_CLICK] = function(self, ev)
            MaterialProps:MenuClick(ev.entity:GetID())
            return true
        end,
    },

    width = 100,
    width_percent = true,

    height = 222,
    
    Gui.Texture({
        width = 265,
        top = 35,
        left = 10,
        id = 'emissive_texture',
        allow_autoreload = true,
        str = "Emissive",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "emissiveTexture", "hasEmissiveTexture", "Emissive") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "emissiveTexture", "hasEmissiveTexture", "Emissive") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "emissiveTexture", "hasEmissiveTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Intensity",
        left = 10,
        top = 157,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 155,
        data = {
            min = 0,
            max = 50,
            decimal = 2,
            overflow_max = true,
        },
        alt = "Emissive light intensity",
        id = 'emissive_intensity',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "emissiveIntensity", "Emissive intensity") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "emissiveIntensity") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "emissiveIntensity") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "emissiveIntensity") end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Emissive color",
        left = 10,
        top = 187,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 185,
        width = 155,
        alt = "Pick emissive color (multiplier)",
        id = 'emissive_color',

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialProps.StartColorPicking(self, "emissiveColor", "Emissive") end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialProps.ColorPicking(self, "emissiveColor") end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialProps.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdColor(self, "emissiveColor") end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
})
end