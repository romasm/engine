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
    width = 1280,
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
            width = 1920,
            height = 1080,
            
            GuiButton({
                styles = {
                    GuiStyles.perf_button,
                },

                left = 25,
                top = 0,
                width = 40,
                height = 40,

                holded = true,

                text = {str = "Q"},
                alt = "Turn on / off collecting of perf data",

                events = {
                    [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                        Profiler:Start()
                        return true
                    end,
                    [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
                        Profiler:Stop()
                        return true
                    end,
                },
            }),

            GuiCheck({
                styles = {GuiStyles.props_check,},
                left = 25,
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

            GuiCheck({
                styles = {GuiStyles.props_check,},
                left = 25,
                top = 95,
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

            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 900,
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

            GuiRect({
                styles = {GuiStyles.ghost,},
                width = 900,
                height = 500,
                left = 250,
                top = 220,
                id = 'frame_rect',

                border = {
                    color = 'text_06',
                    width = 1,
                },
                background = {
                    color = 'bg_02',
                },

                
            }),
             
        }),
    }),
})
end