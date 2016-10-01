loader.require("ComponentsGui.LightCallback")

GuiStyles.area_dataslider = {
    styles = {
        GuiStyles.common_dataslider,
    },

    left = 110,
    width = 150,
    height = 20,

    data = {
        min = 0.05,
        max = 4,
        decimal = 2,
        overflow_max = true,
    },
}

GuiStyles.cone_dataslider = {
    styles = {
        GuiStyles.common_dataslider,
    },

    left = 110,
    width = 150,
    height = 20,

    data = {
        min = 1,
        max = 175,
        decimal = 2,
    },
}

function Gui.LightComp()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    
    id = "light",
    
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
            str = "Light",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 450,

    -- shadows
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Shadows",
        left = 10,
        top = 35,
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 110,
        top = 35,
        width = 60,
        height = 18,
        text = { str = "Cast" },
        alt = "Cast shadows",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return LightCallback.SetCastShadows(self, ev, true) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return LightCallback.SetCastShadows(self, ev, false) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdCastShadows(self, ev) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 110,
        top = 63,
        width = 160,
        height = 18,
        text = { str = "Transparent shadows" },
        alt = "Cast shadows from transparent objects",
        id = 'alpha_shadows',

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return LightCallback.SetTransparentShadows(self, ev, true) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return LightCallback.SetTransparentShadows(self, ev, false) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdTransparentShadows(self, ev) end,
        }
    }),

    -- color
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Color",
        left = 10,
        top = 103,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 110,
        top = 101,
        width = 150,
        alt = "Pick light color",

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return LightCallback.StartColorPicking(self, ev) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return LightCallback.ColorPicking(self) end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return LightCallback.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdColor(self, ev) end,
        },
    }),

    -- brightness
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Lum flux, lm",
        left = 10,
        top = 133,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.area_dataslider,
        },
        top = 131,
        data = {
            min = 0.01,
            max = 5000,
            overflow_max = true,
        },
        alt = "Light luminous flux in Lumens",

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return LightCallback.StartBrightness(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return LightCallback.DragBrightness(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return LightCallback.EndBrightness(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdBrightness(self, ev) end,
        },
    }),

    -- range
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Max distance",
        left = 10,
        top = 173,
    }),
    
    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },
        data = {
            min = 0,
            max = 1000,
        },
        left = 110,
        top = 171,
        id = 'tf_range',

        events = {
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev) return LightCallback.SetRange(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdRange(self, ev) end,
        },
    }),
    
    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 190,
        top = 172,
        width = 120,
        height = 18,
        text = { str = "Auto" },
        id = 'auto_range',
        alt = "Automatic distance calculation",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return LightCallback.SetAutoRange(self, ev, true) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return LightCallback.SetAutoRange(self, ev, false) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdAutoRange(self, ev) end,
        }
    }),

    -- shape
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Source shape",
        left = 10,
        top = 212,
    }),

    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = true,
        left = 110,
        top = 211,
        width = 150,
        height = 21,
        list = {
            "Point",
            "Sphere",
            "Tube",
            "Spot",
            "Disk",
            "Rectangle",
        },
        alt = "Shape of the light source",

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev) return LightCallback.SetType(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdType(self, ev) end,
        },
    }),
    
    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 110,
        top = 242,
        width = 125,
        height = 18,
        text = { str = "Draw source" },
        alt = "Draw light source in scene",
        id = 'draw_shape',
    }),

    -- area
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Sphere radius",
        length = 32,
        static = false,
        left = 10,
        top = 282,
        id = 'area_x',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.area_dataslider,
        },
        top = 280,
        id = 'area_x_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return LightCallback.StartAreaX(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return LightCallback.DragAreaX(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return LightCallback.EndAreaX(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdAreaX(self, ev) end,
        },
    }),
    
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Tube length",
        length = 32,
        static = false,
        left = 10,
        top = 312,
        id = 'area_y',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.area_dataslider,
        },
        top = 310,
        id = 'area_y_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return LightCallback.StartAreaY(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return LightCallback.DragAreaY(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return LightCallback.EndAreaY(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdAreaY(self, ev) end,
        },
    }),

    -- cone
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Outer angle",
        left = 10,
        top = 352,
        id = 'cone_out',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.cone_dataslider,
        },
        top = 350,
        id = 'cone_out_slider',
        alt = "Cone outer angle in degrees",

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return LightCallback.StartConeOut(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return LightCallback.DragConeOut(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return LightCallback.EndConeOut(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdConeOut(self, ev) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Inner angle",
        left = 10,
        top = 382,
        id = 'cone_in',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.cone_dataslider,
        },
        top = 380,
        id = 'cone_in_slider',
        alt = "Cone inner angle in degrees",
        data = {
            min = 1,
            max = 174,
        },
        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return LightCallback.StartConeIn(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return LightCallback.DragConeIn(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return LightCallback.EndConeIn(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return LightCallback.UpdConeIn(self, ev) end,
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