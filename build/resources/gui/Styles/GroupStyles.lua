GuiStyles.group_button = {
    styles = {
        GuiStyles.solid_button,
    },
    
    background = {
        color = 'bg_01_v3',
        color_hover = 'act_01',
        color_press = 'act_01',
        color_nonactive = 'bg_03',
    },

    icon = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02',
        material = GuiMaterials.group_arrow,
        rect = { l = 1, t = 1, w = 19, h = 19 },
    },

    text = {
        offset = { x = 25, y = 0 },
        center = { x = false, y = true },
        str = "Group",
        font = "../resources/fonts/opensans_normal_18px",
        color = 'text_00',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02'
    },
}

GuiStyles.common_group = {
    ignore_events = false,
    collide_through = false,
    
    anim_time = 100,

    closeable = true,
    stackable = true,

    top = 5,

    header = {
        styles = {
            GuiStyles.group_button,
        },

        left = 1,
        right = 1,
        align = GUI_ALIGN.BOTH,

        top = 1,
        height = 20,
    },

    background = {
        color = 'bg_01',
        color_closed = 'act_01',
        color_nonactive = 'bg_03',
    },

    border = {
        width = 0
    },
}

GuiStyles.dumb_group = {
    ignore_events = false,
    collide_through = false,
    anim_time = 0,
    closeable = false,
    stackable = false,
}