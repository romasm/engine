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
        height = 23,

        text = {
            str = "Opacity",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 192,
    
    Gui.Texture({
        width = 265,
        top = 35,
        left = 10,
        id = 'opacity_texture',
        allow_autoreload = true,
        str = "Opacity",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "alphaTexture", "hasAlphaTexture", "Opacity") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "alphaTexture", "hasAlphaTexture", "Opacity") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "alphaTexture", "hasAlphaTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Opacity",
        left = 10,
        top = 157,
        id = 'opacity_str',
    }), 
    
    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 155,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Opacity value / multiplier",
        id = 'opacity_ref',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "alphaValue", "Opacity") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "alphaValue") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "alphaValue") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "alphaValue") end,
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