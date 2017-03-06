function Gui.MaterialThickness()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "thickness",
    
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
            str = "Thickness",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 222,

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Thickness",
        left = 110,
        top = 35,
    }),

    Gui.Texture({
        width = 265,
        top = 65,
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
        top = 187,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 185,
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