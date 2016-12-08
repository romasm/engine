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
        font = "../resources/fonts/opensans_normal_25px",
    },
}

GuiStyles.perf_id = {
    styles = {
        GuiStyles.common_button,
    },
    
    holded = true,

    text = {
        color = 'bg_00',
        color_hover = 'act_03',
        color_press = 'act_03',
        offset = { x = 2, y = 1 },
        center = { x = false, y = false },
        font = "../resources/fonts/opensans_normal_12px",
    },
    
    background = {
        color_hover = 'act_00',
        color_press = 'act_00',
    },

    border = {
        color = 'bg_05_a4',
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
            GuiStyles.close_button_alpha,
        },
    },

    --scrollable = {x = false, y = false},

    left = 400,
    top = 400,
    width = 1505,
    height = 910,

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
            height = 881,

            GuiCheck({
                styles = {GuiStyles.props_check,},
                left = 20,
                top = 25,
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
                left = 20,
                top = 60,
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

            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "0 %",
                left = 250,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "10 %",
                left = 370,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "20 %",
                left = 490,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "30 %",
                left = 610,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "40 %",
                left = 730,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "50 %",
                left = 850,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "60 %",
                left = 970,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "70 %",
                left = 1090,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "80 %",
                left = 1210,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "90 %",
                left = 1330,
                top = 0,
            }),
            GuiString({
                styles = {GuiStyles.bg_str,},
                str = "100 %",
                left = 1450,
                top = 0,
            }),

            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1200,
                height = 500,
                left = 250,
                top = 25,
                background = {
                    color = 'bg_02',
                },                
            }),

            GuiDumb({
                styles = {GuiStyles.live,},
                width = 1200,
                height = 500,
                left = 250,
                top = 25,
                id = 'frame_rect',
            }),

            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1200,
                height = 500,
                left = 250,
                top = 25,
                border = {
                    color = 'act_01',
                    width = 1,
                },
                background = {
                    color = 'null',
                },                
            }),

            GuiString({
                styles = {GuiStyles.bg_str_normal,},
                str = "GPU",
                left = 210,
                top = 560,
            }),

            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1200,
                height = 50, 
                left = 250,
                top = 545,
                background = {
                    color = 'bg_02',
                },               
            }),

            GuiDumb({
                styles = {GuiStyles.live,},
                width = 1200,
                height = 50,
                left = 250,
                top = 545,
                id = 'gru_rect',
            }),

            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1200,
                height = 50,
                left = 250,
                top = 545,
                border = {
                    color = 'act_01',
                    width = 1,
                },
                background = {
                    color = 'null',
                },                
            }),
            
            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1280,
                height = 1,
                left = 200,
                top = 625,
                background = {
                    color = 'act_01',
                },                
            }),


            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1200,
                height = 200,
                left = 250,
                top = 655,
                id = 'graph_rect',

                border = {
                    color = 'act_01',
                    width = 1,
                },
                background = {
                    color = 'bg_02',
                },
            }),
                 
            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1,
                height = 100,
                height_percent = true,
                left = 200,
                top = 0,
                background = {
                    color = 'act_01',
                },                
            }),

            GuiDumb({
                styles = {GuiStyles.live,},
                width = 200,
                height = 500,
                left = 20,
                top = 440,
                id = 'common_stats',
                
                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "Frame time",
                    top = 50,
                }),

                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "CPU",
                    top = 80,
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 16,
                    align = GUI_ALIGN.RIGHT,
                    right = 100,
                    top = 82,
                    id = 'cpu_frame_ms',
                }),

                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "GPU",
                    top = 105,
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 16,
                    align = GUI_ALIGN.RIGHT,
                    right = 100,
                    top = 105,
                    id = 'gpu_frame_ms',
                }),

                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "Real",
                    top = 130,
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 16,
                    align = GUI_ALIGN.RIGHT,
                    right = 100,
                    top = 130,
                    id = 'frame_ms',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 16,
                    align = GUI_ALIGN.RIGHT,
                    right = 33,
                    top = 130,
                    id = 'frame_fps',
                }), 
            }),
                 
            GuiDumb({
                styles = {GuiStyles.live,},
                width = 200,
                height = 500,
                left = 20,
                top = 655,
                id = 'param_stats',

                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "Param",
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
                    top = 30,
                }),

                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "Avg",
                    top = 55,
                }),

                GuiString({
                    styles = {GuiStyles.bg_str_normal,},
                    str = "Max",
                    top = 80,
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 16,
                    align = GUI_ALIGN.RIGHT,
                    right = 100,
                    top = 30,
                    id = 'cur_ms',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 16,
                    align = GUI_ALIGN.RIGHT,
                    right = 100,
                    top = 55,
                    id = 'avg_ms',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 16,
                    align = GUI_ALIGN.RIGHT,
                    right = 100,
                    top = 80,
                    id = 'max_ms',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 8,
                    align = GUI_ALIGN.RIGHT,
                    right = 40,
                    top = 30,
                    id = 'cur_prc',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 8,
                    align = GUI_ALIGN.RIGHT,
                    right = 40,
                    top = 55,
                    id = 'avg_prc',
                }),

                GuiString({
                    styles = {GuiStyles.string_props_01,},
                    str = "",
                    static = false,
                    length = 8,
                    align = GUI_ALIGN.RIGHT,
                    right = 40,
                    top = 80,
                    id = 'max_prc',
                }),
            }),
        }),
    }),
})
end