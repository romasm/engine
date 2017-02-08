function Gui.MaterialAlphatest()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = 'alphatest',
    
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
            str = "Alphatest",
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
        id = 'alphatest_texture',
        allow_autoreload = true,
        str = "Alphatest",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "alphaTexture", "hasAlphaTexture", "Alphatest") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "alphaTexture", "hasAlphaTexture", "Alphatest") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "alphaTexture", "hasAlphaTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Threshold",
        left = 10,
        top = 157,
        id = 'alpha_ref_str',
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
        alt = "Alphatest threshold (cut off value)",
        id = 'alpha_ref',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "alphatestThreshold", "Alphatest threshold") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "alphatestThreshold") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "alphatestThreshold") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "alphatestThreshold") end,
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