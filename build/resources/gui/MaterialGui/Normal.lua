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
        height = 23,

        text = {
            str = "Geometry",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 213,
    
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Normal map",
        left = 107,
        top = 30,
    }),

    Gui.Texture({
        width = 265,
        top = 55,
        left = 10,
        id = 'normal_map',
        allow_autoreload = true,
        str = "Normal map",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = NormalCallback.SetNormalTex,
            [GUI_EVENTS.TEXTURE_DELETE] = NormalCallback.SetNormalTex,
            [GUI_EVENTS.UPDATE] = NormalCallback.UpdNormalTex,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Normals space",
        left = 10,
        top = 177,
        id = 'normal_space_str',
    }),

    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = false,
        left = 120,
        top = 175,
        width = 155,
        height = 21,
        list = {
            "Tangent space",
            "Object space",
        },
        alt = "Normal map space",
        id = 'normal_space',

        events = {
            [GUI_EVENTS.COMBO_SELECT] = NormalCallback.SetNormalSpace,
            [GUI_EVENTS.UPDATE] = NormalCallback.UpdNormalSpace,
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