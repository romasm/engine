GuiStyles.resize_button = {
    styles = {
        GuiStyles.solid_button,
    },

    icon = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02',
        material = GuiMaterials.group_arrow,
        rect = { l = 0, t = 0, w = 15, h = 15 },
    },
}

GuiStyles.scroll_slider = {
    slider = {
        background = {
            color = 'text_06',
            color_hover = 'text_01',
            color_press = 'act_00',
            color_nonactive = 'text_02'
        }
    },
    axis = {
        background = {
            color = 'bg_08',
            color_hover = 'bg_08',
            color_press = 'bg_08',
            color_nonactive = 'bg_08',
        }
    },
    
    slider_size = 5,
    axis_size = 10,
}

GuiStyles.window_header = {
    styles = {
        GuiStyles.string_18,
    },

    color = 'text_01',
    color_nonactive = 'text_01',

    height = 24,
    width = 100,
    width_percent = true,
    center = { x = true, y = true },
}

GuiStyles.window_header_dynamic = {
    styles = {
        GuiStyles.window_header,
    },

    static = false,
    length = 128,
}

GuiStyles.close_button = {
    styles = {
        GuiStyles.solid_button,
    },

    text = {
        font = "",
    },

    align = GUI_ALIGN.RIGHT,
    valign = GUI_VALIGN.TOP,
    height = 25,
    width = 25,

    background = {
        color = 'bg_08',
        color_hover = 'act_01',
        color_press = 'act_01',
    },

    icon = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_02',
        material = GuiMaterials.swin_close,
        rect = { l = 0, t = 0, w = 25, h = 25 },
    },
}

GuiStyles.close_button_alpha = {
    styles = {
        GuiStyles.close_button,
    },
    
    background = {
        color = 'null',
        color_hover = 'act_01',
        color_press = 'act_01',
    },
}

GuiStyles.window_colors = {
    background = {
        color = 'bg_01',
        color_live = 'bg_01',
        color_nonactive = 'bg_01',
    },

    border = {
        color = 'bg_08',
        color_live = 'bg_08',
        color_nonactive = 'bg_08',
    },
    
    shadow = {
        color = 'bg_05',
        color_live = 'bg_05',
        color_nonactive = 'bg_05',
    },
}

GuiStyles.common_window = {
    ignore_events = false,
    collide_through = false,

    scrollable = {x = true, y = true},

    header_size = 25,

    cleintarea_padding = { l = 2, t = 2, r = 2, b = 2 },

    focus_mode = GUI_FOCUS_MODE.NORMAL,

    scrollX = {
        styles = {
            GuiStyles.common_slider,
            GuiStyles.scroll_slider,
        },
        orient = GUI_SLIDER_ORIENT.HORZ,
        height = 10,    
        left = 0,
        right = 0,     
    },
    scrollY = {
        styles = {
            GuiStyles.common_slider,
            GuiStyles.scroll_slider,
        },
        orient = GUI_SLIDER_ORIENT.VERT,
        width = 10, 
        top = 0,
        bottom = 0,
    },
}

GuiStyles.separete_window = {
    styles = {
        GuiStyles.common_window,
    },
    
    closeable = true,
    dragable = true,

    resizeable = {x = true, y = true},
    clamp_resize = {x = true, y = true},    

    independent = true,

    close = {
        styles = {
            GuiStyles.close_button,
        },
    },

    resize = {
        styles = {
            GuiStyles.resize_button,
        },
        
        text = {
            font = "",
        },

        width = 15,
        height = 15
    },

    border = {width = 1,},

    shadow = {width = 5,},
}

GuiStyles.integrated_window = {
    styles = {
        GuiStyles.common_window,
    },
    
    closeable = false,
    dragable = false,

    resizeable = {x = false, y = false},
    clamp_resize = {x = false, y = false},    

    independent = false,

    close = {},

    resize = {},

    border = {width = 0,},

    shadow = {width = 0,},
}

GuiStyles.props_window = {
    styles = {
        GuiStyles.integrated_window,
    },
    
    scrollable = {x = false},
}