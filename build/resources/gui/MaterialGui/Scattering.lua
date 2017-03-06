function Gui.MaterialScattering()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "scattering",
    
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
            str = "Subsurface scattering",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 222,
    
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Medium color",
        left = 100,
        top = 35,
    }),

    Gui.Texture({
        width = 265,
        top = 65,
        left = 10,
        id = 'subsurf_texture',
        allow_autoreload = true,
        str = "Medium",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "subsurfTexture", "hasSubsurfTexture", "Medium") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "subsurfTexture", "hasSubsurfTexture", "Medium") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "subsurfTexture", "hasSubsurfTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Medium color",
        left = 10,
        top = 187,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 185,
        width = 155,
        alt = "Pick medium color / multiplier",

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialProps.StartColorPicking(self, "subsurfaceColor", "Medium", false) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialProps.ColorPicking(self, "subsurfaceColor") end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialProps.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdColor(self, "subsurfaceColor") end,
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