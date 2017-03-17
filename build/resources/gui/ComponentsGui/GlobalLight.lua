loader.require("ComponentsGui.GlobalLightCallback")

function Gui.GlobalLightComp()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    
    id = "global_light",
    
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
            str = "Global light",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 120,

    -- color
    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Color",
        left = 10,
        top = 33,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 110,
        top = 30,
        width = 150,
        alt = "Pick light color",

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return GlobalLightCallback.StartColorPicking(self, ev) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return GlobalLightCallback.ColorPicking(self) end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return GlobalLightCallback.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return GlobalLightCallback.UpdColor(self, ev) end,
        },
    }),

    -- brightness
    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Illuminance, lx",
        left = 10,
        top = 63,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.common_dataslider,
        },
        left = 110,
        width = 150,
        height = 20,
        top = 60,
        data = {
            min = 1,
            max = 120000,
            decimal = 0,
            overflow_max = true,
        },
        alt = "Light illuminance in Lux",

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return GlobalLightCallback.StartBrightness(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return GlobalLightCallback.DragBrightness(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return GlobalLightCallback.EndBrightness(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return GlobalLightCallback.UpdBrightness(self, ev) end,
        },
    }),

    -- area
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Solid angle",
        length = 32,
        static = false,
        left = 10,
        top = 93,
        id = 'area',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.common_dataslider,
        },
        left = 110,
        width = 150,
        height = 20,
        top = 90,
        data = {
            min = 0.1,
            max = 5,
            decimal = 2,
            overflow_max = true,
        },
        id = 'area_slider',
        alt = "Solid angle in degrees",

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return GlobalLightCallback.StartArea(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return GlobalLightCallback.DragArea(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return GlobalLightCallback.EndArea(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return GlobalLightCallback.UpdArea(self, ev) end,
        },
    }),
})
end