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

    height = 190,

    Gui.Texture({
        width = 265,
        top = 30,
        left = 10,
        id = 'subsurf_texture',
        allow_autoreload = true,
        str = "Transmittance",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "subsurfTexture", "hasSubsurfTexture", "Transmittance") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "subsurfTexture", "hasSubsurfTexture", "Transmittance") end,
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
        alt = "Pick transmittance color / multiplier",

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialProps.StartColorPicking(self, "subsurfaceColor", "Medium", false) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialProps.ColorPicking(self, "subsurfaceColor") end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialProps.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdColor(self, "subsurfaceColor") end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Extinction coefficient ( 10 ^ -6 ) ",
        left = 10,
        top = 140,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 160,
        left = 10,
        width = 265,
        data = {
            min = 0,
            max = 100,
            decimal = 5,
        },
        alt = "Extinction coefficient ( 10 ^ -6 )",
        id = 'extinction_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return TransmittanceCallback.StartExtinction(self) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return TransmittanceCallback.DragExtinction(self) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return TransmittanceCallback.EndExtinction(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransmittanceCallback.UpdExtinction(self) end,
        },
    }),
})
end