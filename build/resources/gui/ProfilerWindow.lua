GuiStyles.perf_button = {
    styles = {
        GuiStyles.solid_button,
    },

    holded = false,
    
    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'act_00',
    },
    
    text = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        font = "../content/materials/fonts/opensans_normal_25px",
    },
}

GuiStyles.perf_id = {
    styles = {
        GuiStyles.common_button,
    },
    
    text = {
        color = 'act_03',
        color_hover = 'act_03',
        color_press = 'act_03',
        offset = { x = 2, y = 1 },
        center = { x = false, y = false },
        font = "../content/materials/fonts/opensans_normal_12px",
    },
    
    background = {
        color_hover = 'act_00',
        color_press = 'act_00',
    },

    border = {
        color = 'text_06',
        color_hover = 'act_02',
        color_press = 'act_02',
        width = 1,
    },
}

GuiStyles.bg_str = {
    styles = {
        GuiStyles.ghost,
        GuiStyles.string_autosize,
        GuiStyles.string_16,
    },
    static = true,
    color = 'text_02',
    top = 225,
}

GuiStyles.bg_str_normal = {
    styles = {
        GuiStyles.ghost,
        GuiStyles.string_autosize,
        GuiStyles.string_18,
    },
    static = true,
    color = 'text_02',
}

function Gui.ProfilerWindow()
return GuiWindow({
    styles = {
        GuiStyles.integrated_window,
    },

    background = {
        color = 'bg_01',
        color_live = 'bg_01',
        color_nonactive = 'bg_01',
    },

    border = {
        color = 'bg_01',
        color_live = 'bg_01',
        color_nonactive = 'bg_01',
    },

    independent = true,

    closeable = true,
    close = {
        styles = {
            GuiStyles.close_button,
        },
    },

    --scrollable = {x = false, y = false},

    left = 400,
    top = 400,
    width = 1505,
    height = 900,

    id = "profiler_window",

    header = {
        styles = {
            GuiStyles.window_header,
        },

        str = "Profiler"
    },
    
    events = {
        [GUI_EVENTS.KILL] = function(self, ev) 
            Profiler:SysClose()
            return true
        end,
    },
        
    GuiClientarea({
        GuiBody({
            width = 1491,
            height = 1080,

            GuiCheck({
                styles = {GuiStyles.props_check,},
                left = 25,
                top = 0,
                width = 160,
                height = 18,
                text = { str = "Real-time visualization" },
                alt = "Draw parfomance data in real-time",
                id = 'isrealtime',

                events = {
                    [GUI_EVENTS.CB_CHECKED] = function(self, ev)
                        Profiler:SetRealtime(true)
                        return true
                    end,
                    [GUI_EVENTS.CB_UNCHECKED] = function(self, ev)
                        Profiler:SetRealtime(false)
                        return true
                    end,
                }
            }),
            
            GuiCheck({
                styles = {GuiStyles.props_check,},
                left = 25,
                top = 35,
                width = 150,
                height = 18,
                text = { str = "Dump data on disc" },
                alt = "Write perf data in stats/ every minute",
                id = 'isdump',

                events = {
                    [GUI_EVENTS.CB_CHECKED] = function(self, ev) 
                        Profiler:SetDump(true) 
                        return true 
                    end,
                    [GUI_EVENTS.CB_UNCHECKED] = function(self, ev)
                        Profiler:SetDump(false) 
                        return true 
                    end,
                }
            }),
            
            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1200,
                height = 200,
                left = 250,
                top = 0,
                id = 'graph_rect',

                border = {
                    color = 'text_06',
                    width = 1,
                },
                background = {
                    color = 'bg_02',
                },
            }),

            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "0 %",
                left = 250,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "10 %",
                left = 370,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "20 %",
                left = 490,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "30 %",
                left = 610,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "40 %",
                left = 730,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "50 %",
                left = 850,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "60 %",
                left = 970,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "70 %",
                left = 1090,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "80 %",
                left = 1210,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "90 %",
                left = 1330,
                top = 225,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "100 %",
                left = 1450,
                top = 225,
            }),

            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1200,
                height = 500,
                left = 250,
                top = 250,
                background = {
                    color = 'bg_02',
                },                
            }),

            GuiDumb({
                styles = {GuiStyles.live,},
                width = 1200,
                height = 500,
                left = 250,
                top = 250,
                id = 'frame_rect',
            }),

            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1200,
                height = 500,
                left = 250,
                top = 250,
                border = {
                    color = 'text_06',
                    width = 1,
                },
                background = {
                    color = 'null',
                },                
            }),
            
            GuiDumb({
                styles = {GuiStyles.live,},
                width = 200,
                height = 500,
                left = 25,
                top = 500,
                id = 'param_stats',

                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "Param:",
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 32,
                    left = 50,
                    id = 'param_name',
                }),
            
                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "Cur",
                    align = GUI_ALIGN.RIGHT,
                    right = 175,
                    top = 50,
                }),

                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "Avg",
                    align = GUI_ALIGN.RIGHT,
                    right = 175,
                    top = 75,
                }),

                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "Max",
                    align = GUI_ALIGN.RIGHT,
                    right = 175,
                    top = 100,
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 8,
                    align = GUI_ALIGN.RIGHT,
                    right = 100,
                    top = 50,
                    id = 'cur_ms',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 8,
                    align = GUI_ALIGN.RIGHT,
                    right = 100,
                    top = 75,
                    id = 'avg_ms',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 8,
                    align = GUI_ALIGN.RIGHT,
                    right = 100,
                    top = 100,
                    id = 'max_ms',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 8,
                    align = GUI_ALIGN.RIGHT,
                    right = 40,
                    top = 50,
                    id = 'cur_prc',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 8,
                    align = GUI_ALIGN.RIGHT,
                    right = 40,
                    top = 75,
                    id = 'avg_prc',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 8,
                    align = GUI_ALIGN.RIGHT,
                    right = 40,
                    top = 100,
                    id = 'max_prc',
                }),
            }),            
        }),
    }),
})
end