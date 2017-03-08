loader.require("MaterialGui.AOCallback")

function Gui.MaterialAO()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "ao",
    
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
            str = "Ambient Occlusion",
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
        id = 'ao_texture',
        allow_autoreload = true,
        str = "AO",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "aoTexture", "hasAOTexture", "AO") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "aoTexture", "hasAOTexture", "AO") end,
            [GUI_EVENTS.UPDATE] = AOCallback.UpdAOTex,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Power",
        static = false,
        length = 16,
        left = 120,
        top = 93,
        id = 'power_str',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 110,
        data = {
            min = 0,
            max = 3,
            decimal = 3,
        },
        alt = "Ambient Occlusion power",
        id = 'power_sld',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "aoPower", "Ambient Occlusion power") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "aoPower") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "aoPower") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "aoPower") end,
        },
    }),
})
end