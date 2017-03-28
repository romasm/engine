loader.require("MaterialGui.ScatteringCallback")

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
        height = 21,

        text = {
            str = "Subsurface scattering",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 260,

    Gui.Texture({
        width = 265,
        top = 30,
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
        styles = {GuiStyles.string_props_03,},
        str = "Color / Multiplier",
        left = 120,
        top = 93,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 110,
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
        styles = {GuiStyles.string_props_03,},
        str = "Attenuation dist",
        left = 10,
        top = 143,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 140,
        left = 120,
        width = 155,
        data = {
            min = 0.0,
            max = 0.25,
            decimal = 4,
            overflow_max = true,
        },
        alt = "Half-energy distance, m",
        id = 'attenuation_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return ScatteringCallback.StartAttenuation(self) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return ScatteringCallback.DragAttenuation(self) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return ScatteringCallback.EndAttenuation(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return ScatteringCallback.UpdAttenuation(self) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Tinting",
        left = 10,
        top = 173,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 170,
        left = 120,
        width = 155,
        data = {
            min = 0.0,
            max = 1.0,
            decimal = 3,
        },
        alt = "Subsurface tinting",
        id = 'tint_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartDeffered(self, "ssTint", "Tinting") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragDeffered(self, "ssTint") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndDeffered(self, "ssTint") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdDeffered(self, "ssTint") end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Asymmetry",
        left = 10,
        top = 203,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 200,
        left = 120,
        width = 155,
        data = {
            min = -0.5,
            max = 0.5,
            decimal = 3,
        },
        alt = "Asymmetry (Phase), Forward / Backward scattering",
        id = 'asymmetry_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartDeffered(self, "asymmetry", "Asymmetry") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragDeffered(self, "asymmetry") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndDeffered(self, "asymmetry") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdDeffered(self, "asymmetry") end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "IOR",
        left = 10,
        top = 233,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 230,
        left = 120,
        width = 155,
        data = {
            min = 1,
            max = 3,
            decimal = 5,
            overflow_max = true,
        },
        alt = "Index of refraction",
        id = 'ior_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return ScatteringCallback.StartIOR(self) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return ScatteringCallback.DragIOR(self) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return ScatteringCallback.EndIOR(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return ScatteringCallback.UpdIOR(self) end,
        },
    }),
})
end