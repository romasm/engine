function Gui.MaterialAlbedo()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "albedo",
    
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
            str = "Albedo",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 187,
    
    Gui.Texture({
        width = 265,
        top = 35,
        left = 10,
        id = 'albedo_texture',
        allow_autoreload = true,
        str = "Albedo",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "albedoTexture", "hasAlbedoTexture", "Albedo") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "albedoTexture", "hasAlbedoTexture", "Albedo") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "albedoTexture", "hasAlbedoTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Albedo color",
        left = 10,
        top = 152,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 150,
        width = 155,
        alt = "Pick albedo color (multiplier)",

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialProps.StartColorPicking(self, "albedoColor", "Albedo", false) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialProps.ColorPicking(self, "albedoColor") end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialProps.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdColor(self, "albedoColor") end,
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
