GuiStyles.menu_dash_default = {
    styles = {
        GuiStyles.ghost,
        GuiStyles.no_border,
    },

    background = {
        color = 'text_02',
    },

    width = 100,
    width_percent = true,
    height = 1,

    align = GUI_ALIGN.CENTER,
}

GuiStyles.menu_button_default = {
    styles = {
        GuiStyles.solid_button,
    },
    
    align = GUI_ALIGN.BOTH,
    left = 1,
    right = 1,
    height = 25,

    icon = {
        material = {},
        rect = { l = 0, t = 0, w = 0, h = 0 },
    },

    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'act_01',
        color_nonactive = 'bg_01'
    },

    text = {
        offset = { x = 20, },
        center = { x = false, },
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02'
    },
}

GuiStyles.menu_hot_default = {
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_simple,
            GuiStyles.string_18,
        },

        padding = {x = 0, y = -1},

        height = 25,

        color = 'text_01',
        color_nonactive = 'text_02',
    }

GuiStyles.menu_default = {
    styles = {
        GuiStyles.live,
        --GuiStyles.no_border,
    },

    adapt_horz = true,
    adapt_vert = true,

    offset = {x = 0, y = 0},

    background = {
        color = 'bg_01',
    },

    border = {
        color = 'bg_05',
        width = 1,
    },
}

GuiStyles.menu_topbar = {
    styles = {
        GuiStyles.menu_default,
    },

    adapt_horz = false,
    adapt_vert = false,
    width = 250,

    background = {
        color = 'null',
    },
}