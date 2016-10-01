GuiStyles.topmenu_button = {
    styles = {
        GuiStyles.solid_button,
    },

    holded = false,

    height = 24,

    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'bg_05',
    },
    
    text = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_02',
        font = "../content/materials/fonts/opensans_normal_18px",
    },
}

function Gui.ProfilerWindow()
return GuiWindow({
    styles = {
        GuiStyles.integrated_window,
        GuiStyles.window_colors,
    },

    independent = true,

    closeable = true,
    close = {
        styles = {
            GuiStyles.close_button,
        },
    },

    --scrollable = {x = false, y = false},

    left = 0,
    top = 0,
    width = 1920,
    height = 1080,

    id = "profiler_window",

    header = {
        styles = {
            GuiStyles.window_header,
        },
        str = "Profiler"
    },
    
    events = {
        [GUI_EVENTS.KILL] = function(self, ev) 
            MainWindow.profiler_win = nil
            return true
        end,
    },
        
    GuiClientarea({
        GuiBody({
            width = 1920,
            height = 1080,
            
            GuiButton({
                styles = {
                    GuiStyles.topmenu_button,
                },

                left = 20,
                top = 20,
                width = 75,

                holded = true,

                text = {str = "Turn on"},
                alt = "Turn on / off collecting of perf data",

                events = {
                    [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return true end,
                    [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) return true end,
                },
            }),
             
        }),
    }),
})
end