function Gui.MaterialOpacity()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = 'opacity',
    
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
            str = "Opacity",
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
        id = 'opacity_texture',
        allow_autoreload = true,
        str = "Opacity",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "opacityTexture", "hasOpacityTexture", "Opacity") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "opacityTexture", "hasOpacityTexture", "Opacity") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "opacityTexture", "hasOpacityTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Value / Multiplier",
        left = 120,
        top = 93,
        id = 'opacity_str',
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
        alt = "Opacity value / multiplier",
        id = 'opacity_ref',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "opacityValue", "Opacity") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "opacityValue") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "opacityValue") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "opacityValue") end,
        },
    }),
})
end