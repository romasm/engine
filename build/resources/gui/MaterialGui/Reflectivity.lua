loader.require("MaterialGui.ReflectivityCallback")

function Gui.MaterialReflectivity()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "reflectivity",
    
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
            str = "Reflectivity",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 222,

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Parametrization",
        left = 10,
        top = 37,
    }),

    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = false,
        left = 120,
        top = 35,
        width = 155,
        height = 21,
        list = {
            "Specular (F0)",
            "Metalness",
        },
        alt = "Specular or Metalness (F0) parametrization",

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev) return MaterialProps.SetSelector(self, "isMetalPipeline", "Reflectivity type") end,
            [GUI_EVENTS.UPDATE] = ReflectivityCallback.UpdReflectivityType,
        },
    }),

    Gui.Texture({
        width = 265,
        top = 65,
        left = 10,
        allow_autoreload = true,
        str = "Reflectivity",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "reflectivityTexture", "hasReflectivityTexture", "Reflectivity") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "reflectivityTexture", "hasReflectivityTexture", "Reflectivity") end,
            [GUI_EVENTS.UPDATE] = ReflectivityCallback.UpdReflectivityTex,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Metalness",
        static = false,
        length = 16,
        left = 10,
        top = 187,
        id = 'reflectivity_header',
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
        alt = "Metalness",
        id = 'metalness_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "metalnessValue", "Metalness") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "metalnessValue") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "metalnessValue") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "metalnessValue") end,
        },
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 185,
        width = 155,
        alt = "Pick specular color (multiplier)",
        id = 'specular_picker',

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialProps.StartColorPicking(self, "reflectivityColor", "Specular", false) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialProps.ColorPicking(self, "reflectivityColor") end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialProps.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdColor(self, "reflectivityColor") end,
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