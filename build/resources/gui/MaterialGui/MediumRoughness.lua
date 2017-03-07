function Gui.MaterialMediumRoughness()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "medium_roughness",
    
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
            str = "Medium Roughness",
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
        id = 'medium_roughness_texture',
        allow_autoreload = true,
        str = "Roughness",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "insideRoughnessTexture", "hasInsideRoughnessTexture", "Medium Roughness") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "insideRoughnessTexture", "hasInsideRoughnessTexture", "Medium Roughness") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "insideRoughnessTexture", "hasInsideRoughnessTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Value / Multiplier",
        left = 120,
        top = 85,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 110,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Roughness value / multiplier",
        id = 'medium_roughness_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "insideRoughnessValue", "Medium Roughness") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "insideRoughnessValue") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "insideRoughnessValue") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "insideRoughnessValue") end,
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