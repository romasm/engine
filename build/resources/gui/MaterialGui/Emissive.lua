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
        height = 21,

        text = {
            str = "Emissive",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 140,
    
    Gui.Texture({
        width = 265,
        top = 30,
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
        styles = {GuiStyles.string_props_03,},
        str = "Emissive color",
        left = 120,
        top = 53,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 70,
        width = 155,
        alt = "Pick emissive color / multiplier",
        id = 'emissive_color',

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialProps.StartColorPicking(self, "emissiveColor", "Emissive", true) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialProps.ColorPicking(self, "emissiveColor") end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialProps.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdColor(self, "emissiveColor") end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Intensity",
        left = 120,
        top = 93,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 110,
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
})
end