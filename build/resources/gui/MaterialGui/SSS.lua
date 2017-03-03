function Gui.MaterialSSS()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "sss",
    
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
            str = "Subsurface",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 412,
    
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Subsurface color",
        left = 90,
        top = 35,
    }),

    Gui.Texture({
        width = 265,
        top = 65,
        left = 10,
        id = 'subsurf_texture',
        allow_autoreload = true,
        str = "Subsurface",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "subsurfTexture", "hasSubsurfTexture", "Subsurface") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "subsurfTexture", "hasSubsurfTexture", "Subsurface") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "subsurfTexture", "hasSubsurfTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Subsurface color",
        left = 10,
        top = 187,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 185,
        width = 155,
        alt = "Pick subsurface color / multiplier",

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialProps.StartColorPicking(self, "subsurfaceColor", "Subsurface", false) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialProps.ColorPicking(self, "subsurfaceColor") end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialProps.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdColor(self, "subsurfaceColor") end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Thickness",
        left = 110,
        top = 225,
    }),

    Gui.Texture({
        width = 265,
        top = 255,
        left = 10,
        id = 'thickness_texture',
        allow_autoreload = true,
        str = "Thickness",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "thicknessTexture", "hasThicknessTexture", "Thickness") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "thicknessTexture", "hasThicknessTexture", "Thickness") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "thicknessTexture", "hasThicknessTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Thickness",
        left = 10,
        top = 377,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 375,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Thickness value / multiplier",
        id = 'thickness_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "thicknessValue", "Thickness") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "thicknessValue") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "thicknessValue") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "thicknessValue") end,
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