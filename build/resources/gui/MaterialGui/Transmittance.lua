loader.require("MaterialGui.TransmittanceCallback")

function Gui.MaterialTransmittance()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "transmittance",
    
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
            str = "Transmittance",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 285,

    Gui.Texture({
        width = 265,
        top = 30,
        left = 10,
        id = 'subsurf_texture',
        allow_autoreload = true,
        str = "Transmittance",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "absorptionTexture", "hasAbsorptionTexture", "Transmittance") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "absorptionTexture", "hasAbsorptionTexture", "Transmittance") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "absorptionTexture", "hasAbsorptionTexture") end,
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
        alt = "Pick transmittance color / multiplier",

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialProps.StartColorPicking(self, "absorptionColor", "Transmittance", false) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialProps.ColorPicking(self, "absorptionColor") end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialProps.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdColor(self, "absorptionColor") end,
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
            min = 0,
            max = 2,
            decimal = 3,
            overflow_max = true,
        },
        alt = "Half-energy distance, m",
        id = 'extinction_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return TransmittanceCallback.StartAttenuation(self) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return TransmittanceCallback.DragAttenuation(self) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return TransmittanceCallback.EndAttenuation(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransmittanceCallback.UpdAttenuation(self) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "IOR",
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
            min = 1,
            max = 3,
            decimal = 5,
            overflow_max = true,
        },
        alt = "Index of refraction",
        id = 'ior_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return TransmittanceCallback.StartIOR(self) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return TransmittanceCallback.DragIOR(self) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return TransmittanceCallback.EndIOR(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransmittanceCallback.UpdIOR(self) end,
        },
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 120,
        top = 195,
        width = 140,
        height = 18,
        id = 'iorasspec_check',
        text = { str = "Use for reflection" },
        alt = "IOR overwrites reflectivity",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return TransmittanceCallback.SetIORAsSpec(self, true) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return TransmittanceCallback.SetIORAsSpec(self, false) end,
            [GUI_EVENTS.UPDATE] = TransmittanceCallback.UpdIORAsSpec,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Abbe number",
        left = 10,
        top = 228,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 225,
        left = 120,
        width = 155,
        data = {
            min = 0.01,
            max = 70,
            decimal = 2,
        },
        alt = "Dispersion Abbe number",
        id = 'abbe_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return TransmittanceCallback.StartAbbe(self) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return TransmittanceCallback.DragAbbe(self) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return TransmittanceCallback.EndAbbe(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransmittanceCallback.UpdAbbe(self) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Critical angle fade",
        left = 10,
        top = 258,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 255,
        left = 120,
        width = 155,
        data = {
            min = 0,
            max = 1,
            decimal = 2,
        },
        alt = "Amount of refraction faded on critical angle",
        id = 'tir_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "tirAmount", "Critical angle fade") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "tirAmount") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "tirAmount") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "tirAmount") end,
        },
    }),
})
end