loader.require("MaterialGui.NormalCallback")

function Gui.MaterialNormal()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "normal",
    
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
            str = "Geometry",
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
        id = 'normal_map',
        allow_autoreload = true,
        str = "Normal map",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialProps.SetTexture(self, "normalTexture", "hasNormalTexture", "Normal map") end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialProps.SetTexture(self, "normalTexture", "hasNormalTexture", "Normal map") end,
            [GUI_EVENTS.UPDATE] = NormalCallback.UpdNormalTex,
        }
    }),

    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = false,
        left = 120,
        top = 82,
        width = 155,
        height = 21,
        list = {
            "Tangent space",
            "Object space",
        },
        alt = "Normal map space",
        id = 'normal_space',

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev) return MaterialProps.SetSelector(self, "objectSpaceNormalMap", "Normals space") end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialProps.UpdSelector(self, "objectSpaceNormalMap") end,
        },
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 120,
        top = 112,
        width = 75,
        height = 18,
        id = 'normal_y',
        text = { str = "Invert Y" },
        alt = "Invert Y in normal map",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return NormalCallback.SetInvertY(self, true) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return NormalCallback.SetInvertY(self, false) end,
            [GUI_EVENTS.UPDATE] = NormalCallback.UpdInvertY,
        }
    }),
})
end