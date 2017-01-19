loader.require("MaterialGui.RoughnessCallback")

function Gui.MaterialRoughness()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "roughness",
    
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
            str = "Microfacets",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 277,

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
            "Roughness",
            "Glossiness",
        },
        alt = "Roughness or glossiness (inverted roughness) parametrization",

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev) return MaterialPropsCallback.SetRoughnessType(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdRoughnessType(self, ev) end,
        },
    }),

    Gui.Texture({
        width = 265,
        top = 65,
        left = 10,
        id = 'roughness_texture',
        allow_autoreload = true,
        str = "Microfacets",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetRoughnessTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetRoughnessTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdRoughnessTex(self, ev) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 180,
        width = 180,
        height = 18,
        text = { str = "Two channels, anisotropic" },
        alt = "Separate for U and V",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return MaterialPropsCallback.SetAnisoRG(self, ev, true) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return MaterialPropsCallback.SetAnisoRG(self, ev, false) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdAnisoRG(self, ev) end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Microfacets U",
        static = false,
        length = 16,
        left = 10,
        top = 212,
        id = 'roughness_u_str',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 210,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Roughness / Glossiness for U",
        id = 'roughness_u',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialPropsCallback.StartRoughU(self) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialPropsCallback.DragRoughU(self) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialPropsCallback.EndRoughU(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdRoughU(self) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Microfacets V",
        static = false,
        length = 16,
        left = 10,
        top = 242,
        id = 'roughness_v_str',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 240,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Roughness / Glossiness for V",
        id = 'roughness_v',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialPropsCallback.StartValue(self, 4, "Microfacets V") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialPropsCallback.DragValue(self, 4) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialPropsCallback.EndValue(self, 4) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdValue(self, 4) end,
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