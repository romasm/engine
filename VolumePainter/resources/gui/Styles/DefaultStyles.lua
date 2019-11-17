GuiStyles.live = {
    ignore_events = false,
    collide_through = false
}

GuiStyles.ghost = {
    ignore_events = true,
    collide_through = true
}

GuiStyles.dead = {
    ignore_events = true,
    collide_through = false
}

GuiStyles.live_ghost = {
    ignore_events = false,
    collide_through = true
}

GuiStyles.no_border = {
    border = {
        width = 0,
    },
}

-- BUTTON
GuiStyles.common_button = {
    ignore_events = false,
    collide_through = false,
    hover_only = false,
    cursor = SYSTEM_CURSORS.ARROW,
    fadein_time = 50,
    fadeout_time = 300,
}

GuiStyles.solid_button = {
    styles = {
        GuiStyles.common_button,
        GuiStyles.no_border,
    },
    
    text = {
        offset = { x = 0, y = 0 },
        center = { x = true, y = true },
        font = "../resources/fonts/opensans_normal_18px",
    },
}

GuiStyles.empty_button = {
    styles = {
        GuiStyles.common_button,
        GuiStyles.no_border,
    },
    
    text = {
        font = "",
    },
}

GuiStyles.color_button = {
    styles = {
        GuiStyles.common_button,
    },
    
    text = {
        font = "",
    },

    height = 20,

    border = {
        color = 'act_05',
        color_hover = 'act_02',
        color_press = 'act_02',
        color_nonactive = 'text_02',
        width = 1,
    },

    background = {
        color = 'bg_03',
        color_hover = 'bg_03',
        color_press = 'bg_03',
        color_nonactive = 'bg_03'
    },
}

GuiStyles.colorwin_button = {
    styles = {GuiStyles.solid_button,},
    background = {
        color = 'bg_08',
        color_hover = 'act_01',
        color_press = 'act_00',
        color_nonactive = 'bg_01',
    },
    text = {
        font = "../resources/fonts/opensans_normal_20px",
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02',
    },
    right = 10,
    width = 100,
    height = 30,
}

GuiStyles.tool_button = {
    styles = {
        GuiStyles.common_button,
    },

	holded = true,
	stay_holded = true,

    height = 40,
    width = 40,
    top = 0,

    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'act_00',
        color_nonactive = 'bg_01',
    },

    text = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02',
        font = '',
    },

    border = {
        width = 0,
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'act_02',
        color_nonactive = 'bg_01',
    },

    icon = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02',
        rect = { l = 0, t = 0, w = 40, h = 40 },
    },
}

GuiStyles.alt_button = {
    ignore_events = false,
    collide_through = true,
    hover_only = true,
    cursor = SYSTEM_CURSORS.ARROW,
    fadein_time = 0,
    fadeout_time = 0,
    holded = false,

    background = {
        color = 'null',
        color_hover = 'null',
        color_press = 'null',
        color_nonactive = 'null',
    },
    text = {
        font = '',
    },
    border = {
        width = 0,
    },
    icon = {
        rect = { l = 0, t = 0, w = 0, h = 0 },
    },
}

-- STRING
GuiStyles.string_simple = {
    center = { x = true, y = true },
    offset = { x = 0, y = 0 },
}

GuiStyles.string_autosize = {
    center = { x = true, y = true },
    offset = { x = 0, y = 0 },
    padding = { x = 0, y = 0 },
}

GuiStyles.string_16 = {
    font = "../resources/fonts/opensans_normal_16px",
}
GuiStyles.string_18 = {
    font = "../resources/fonts/opensans_normal_18px",
}
GuiStyles.string_20 = {
    font = "../resources/fonts/opensans_normal_20px",
}
GuiStyles.string_25 = {
    font = "../resources/fonts/opensans_normal_25px",
}
GuiStyles.string_40 = {
    font = "../resources/fonts/opensans_normal_40px",
}
GuiStyles.string_70 = {
    font = "../resources/fonts/opensans_normal_70px",
}
GuiStyles.string_25_l = {
    font = "../resources/fonts/opensans_light_25px",
}
GuiStyles.string_40_l = {
    font = "../resources/fonts/opensans_light_40px",
}
GuiStyles.string_70_l = {
    font = "../resources/fonts/opensans_light_70px",
}

GuiStyles.string_props_01 = {
    styles = {
        GuiStyles.ghost,
        GuiStyles.string_autosize,
        GuiStyles.string_18,
    },
    static = true,
    color = 'text_01',
    color_nonactive = 'text_02',
}

GuiStyles.string_props_02 = {
    styles = {
        GuiStyles.ghost,
        GuiStyles.string_autosize,
        GuiStyles.string_18,
    },
    static = true,
    color = 'text_06',
    color_nonactive = 'text_02',
}

GuiStyles.string_props_03 = {
    styles = {
        GuiStyles.ghost,
        GuiStyles.string_autosize,
        GuiStyles.string_16,
    },
    static = true,
    color = 'text_01',
    color_nonactive = 'text_02',
}

GuiStyles.string_props_04 = {
    styles = {
        GuiStyles.ghost,
        GuiStyles.string_autosize,
        GuiStyles.string_16,
    },
    static = true,
    color = 'text_06',
    color_nonactive = 'text_02',
}

-- SLIDER
GuiStyles.common_slider = {
    ignore_events = false,
    collide_through = false,

    slider_bound = true,

    slider_size_percent = false,
    axis_size_percent = false,

    slider = {
        styles = {
            GuiStyles.solid_button,
        },

        text = {
            font = ""
        }
    },

    axis = {
        styles = {
            GuiStyles.solid_button,
        },

        text = {
            font = ""
        }
    },
}

-- TEXTFIELD
GuiStyles.common_textfield = {
    ignore_events = false,
    collide_through = false,

    selectable = true,
    
    show_tail = false,
    allow_none = true,
    dbclick_activation = false,
    
    text = {
        offset = { x = 3, y = 0 },
        center = { x = false, y = true },
        str = "",
        length = 64,
        font = "../resources/fonts/opensans_normal_16px",
    },

    data = {
        d_type = GUI_TEXTFIELD.TEXT,
        min = 0,
        max = 0,
        decimal = 0
    },
}

GuiStyles.float_textfield = {
    data = {
        d_type = GUI_TEXTFIELD.FLOAT,
        decimal = 2,
        min = -10000000,
        max = 10000000,
    },

    text = {
        length = 16,
    },

    width = 60,
    height = 20,
}

GuiStyles.int_textfield = {
    data = {
        d_type = GUI_TEXTFIELD.INT,
        decimal = 0,
        min = -10000000,
        max = 10000000,
    },

    text = {
        length = 16,
    },

    width = 60,
    height = 20,
}

GuiStyles.text_textfield = {
    data = {
        d_type = GUI_TEXTFIELD.TEXT,
    },

    text = {
        length = 256,
    },

    width = 60,
    height = 20,
}

GuiStyles.props_textfield = {
    styles = {
        GuiStyles.common_textfield,
    },

    color_selection = 'act_00',
    color_cursor = 'bg_05',

    background = {
        color = 'bg_05',
        color_live = 'act_02',
        color_nonactive = 'bg_08'
    },

    border = {
        color = 'act_05',
        color_live = 'bg_05',
        color_nonactive = 'text_02',
        width = 1
    },

    text = {
        color = 'text_01',
        color_live = 'bg_01',
        color_nonactive = 'text_02'
    },
}

-- COMBO
GuiStyles.common_combo = {
    ignore_events = false,
    collide_through = false,
    
    cursor = SYSTEM_CURSORS.ARROW,

    fadein_time = 50,
    fadeout_time = 300,

    allow_none = false,

    str_height = 20,
    
    icon = {
        material = GuiMaterials.combo_arrow,
    },

    text = {
        offset = { x = 5, y = 0 },
        center = { x = false, y = true },
        font = "../resources/fonts/opensans_normal_16px",
        length = 64
    },
}

GuiStyles.props_combo = {    
    styles = {
        GuiStyles.common_combo,
    },

    icon = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_00',
        color_nonactive = 'text_02',

        background = {
            color = 'bg_05',
            color_hover = 'act_01',
            color_press = 'bg_05',
            color_nonactive = 'bg_03',
        },
    },

    background = {
        color = 'bg_05',
        color_hover = 'act_01',
        color_press = 'bg_05',
        color_nonactive = 'bg_03',
    },

    border = {
        color = 'act_05',
        color_hover = 'act_02',
        color_press = 'act_02',
        color_nonactive = 'text_02',
        width = 1
    },

    text = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'text_01',
        color_nonactive = 'text_02',
    },
}

GuiStyles.combo_button = {
    styles = {
        GuiStyles.solid_button,
    },

    icon = {
        material = {},
        rect = { l = 0, t = 0, w = 0, h = 0 },
    },
}

-- CHECK
GuiStyles.common_check = {
    ignore_events = false,
    collide_through = false,

    fadein_time = 50,
    fadeout_time = 300,
    
    check = {
        padding = { x = 5, y = 5 },
    },

    text = {
        offset = { x = 5, y = 0 },
        center = true,
        font = "../resources/fonts/opensans_normal_16px",
    },
}

GuiStyles.props_check = {
    styles = {
        GuiStyles.common_check,
    },
    
    check = {
        color = 'act_00',
        color_hover = 'act_00',
        color_nonactive = 'text_02',
    },

    box = {
        color = 'bg_05',
        color_hover = 'act_01',
        color_nonactive = 'bg_03'
    },

    border = {
        color = 'act_05',
        color_hover = 'act_02',
        color_nonactive = 'text_02',
        width = 1
    },

    text = {
        color = 'text_01',
        color_hover = 'act_03',
        color_nonactive = 'text_02'
    },
}

-- dataslider
GuiStyles.common_dataslider = {
    ignore_events = false,
    collide_through = false,

    cursor = SYSTEM_CURSORS.ARROW,

    fadein_time = 50,
    fadeout_time = 300,
    
    bar = {
        color = 'act_01',
        color_hover = 'act_01',
        color_press = 'act_00',
        color_nonactive = 'bg_01',
    },

    background = {
        color = 'bg_05',
        color_hover = 'bg_01_v1',
        color_press = 'bg_05',
        color_nonactive = 'bg_03',
        color_text = 'act_03'
    },

    border = {
        color = 'act_05',
        color_hover = 'act_02',
        color_press = 'act_02',
        color_nonactive = 'text_02',
        width = 1
    },

    text = {
        offset = {x = 0, y = 0},
        center = {x = true, y = true},
        length = 10,
        font = "../resources/fonts/opensans_normal_16px",
        color = 'act_02',
        color_live = 'bg_01',
        color_nonactive = 'text_02',
        color_selection = 'act_00',
        color_cursor = 'bg_05'
    },

    data = {
        min = -10,
        max = 10,
        decimal = 2,
        step = 0
    },
}

GuiStyles.mat_dataslider = {
    styles = {
        GuiStyles.common_dataslider,
    },

    left = 120,
    width = 155,
    height = 20,
}

-- filefield
GuiStyles.filefield_button = {
    styles = {
        GuiStyles.solid_button,
    }, 
    text = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_02',
    },
    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'bg_05',
    },
}

GuiStyles.common_filefield = {
    ignore_events = false,
    collide_through = false,

    allow_edit = true,
    allow_none = true,
    browse_header = "Choose file",
    filetypes = {
        {"All", "*.*"},
    },

    width = 195,
    height = 20,
        
    ChildFilepath = {
        styles = {
            GuiStyles.props_textfield,
        },
    },

    ChildBrowse = {
        styles = {
            GuiStyles.filefield_button,
        },
        text = {str = "...",},
        alt = "Browse",
    },

    ChildReload = {
        styles = {
            GuiStyles.filefield_button,
        },
        text = {str = "R",},
        alt = "Reload",
    },

    ChildDelete = {
        styles = {
            GuiStyles.filefield_button,
        },
        text = {str = "X",},
        alt = "Clean",
    },
}

-- color styles
GuiStyles.vp_header_colors = {
    background = {
        color = 'bg_05_a4',
        color_hover = 'bg_05_a7',
        color_press = 'bg_05_a6',
    },

    text = {
        color = 'act_02',
        color_hover = 'act_03',
        color_press = 'act_02',
        font = "../resources/fonts/opensans_normal_18px",
    },
}

GuiStyles.topmenu_button_colors = {
    styles = {
        GuiStyles.solid_button,
    },

    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'bg_05',
    },
    
    text = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_02',
    },
}

GuiStyles.topmenu_button = {
    styles = {
        GuiStyles.topmenu_button_colors,
    },

    holded = true,
    
    height = 100,
    height_percent = true,
    
    text = {
        font = "../resources/fonts/opensans_normal_20px",
    },
}

GuiStyles.sys_button = {
    styles = {
        GuiStyles.solid_button,
    },

    holded = false,

    height = 25,
    width = 25,

    align = GUI_ALIGN.RIGHT,

    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'bg_05',
    },

    text = {
        font = '',
    },

    icon = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_02',
        rect = { l = 0, t = 0, w = 25, h = 25 },
    },
}