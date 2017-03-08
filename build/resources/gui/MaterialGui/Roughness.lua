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
        height = 21,

        text = {
            str = "Microfacets",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 195,

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Parametrization",
        left = 10,
        top = 32,
    }),

    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = false,
        left = 120,
        top = 30,
        width = 155,
        height = 21,
        list = {
            "Roughness",
            "Glossiness",
        },
        alt = "Roughness or Glossiness(inverted roughness) parametrization",

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev) return MaterialProps.SetSelector(self, "isGlossiness", "Microfacets type") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdSelector(self, "isGlossiness") end,
        },
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 120,
        top = 55,
        width = 180,
        height = 18,
        text = { str = "Anisotropic" },
        alt = "Two channels, Separate for U and V",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return RoughnessCallback.SetRoughnessAniso(self, true) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return RoughnessCallback.SetRoughnessAniso(self, false) end,
            [GUI_EVENTS.UPDATE] = RoughnessCallback.UpdRoughnessAniso,
        }
    }),

    Gui.Texture({
        width = 265,
        top = 85,
        left = 10,
        id = 'roughness_texture',
        allow_autoreload = true,
        str = "Microfacets",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "roughnessTexture", "hasRoughnessTexture", "Microfacets") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "roughnessTexture", "hasRoughnessTexture", "Microfacets") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdTexture(self, "roughnessTexture", "hasRoughnessTexture") end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Microfacets U",
        static = false,
        length = 16,
        left = 120,
        top = 108,
        id = 'roughness_u_str',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 125,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Roughness / Glossiness for U",
        id = 'roughness_u',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "roughnessX", "Roughness / Glossiness U") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "roughnessX") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "roughnessX") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "roughnessX") end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Microfacets V",
        static = false,
        length = 16,
        left = 120,
        top = 148,
        id = 'roughness_v_str',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 165,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Roughness / Glossiness for V",
        id = 'roughness_v',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialProps.StartValue(self, "roughnessY", "Roughness / Glossiness V") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialProps.DragValue(self, "roughnessY") end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialProps.EndValue(self, "roughnessY") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdValue(self, "roughnessY") end,
        },
    }),
})
end