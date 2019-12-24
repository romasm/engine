function Gui.TopBar()
return GuiRect({
    styles = {
        GuiStyles.live,
        GuiStyles.no_border,
    },
    
    background = {
        color = 'bg_01',
    },
    
    width = 100,
    width_percent = true,
    height = 25,

    top = 0,

    id = 'topbar',

    focus_mode = GUI_FOCUS_MODE.NONE,

    GuiButton({
        styles = {
            GuiStyles.topmenu_button,
        },

        left = 10,
        width = 64,

        text = {str = lcl.file_title},
        id = 'tb_file',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return MainWindow:MenuPress(ev.entity, "file") end,
            [GUI_EVENTS.BUTTON_HOVER] = function(self, ev) return MainWindow:MenuHover(ev.entity, "file") end,

            [GUI_EVENTS.MENU_CLOSE] = function(self, ev) return MainWindow:MenuClose(self, "file") end,
            [GUI_EVENTS.MENU_CLICK] = function(self, ev) return MainWindow:FileMenuClick(self, ev) end, 
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.topmenu_button,
        },

		left = 84,
        width = 64,

        text = {str = lcl.settings_title},
        id = 'tb_settings',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return MainWindow:MenuPress(ev.entity, "sets") end,
            [GUI_EVENTS.BUTTON_HOVER] = function(self, ev) return MainWindow:MenuHover(ev.entity, "sets") end,

            [GUI_EVENTS.MENU_CLOSE] = function(self, ev) return MainWindow:MenuClose(self, "sets") end,
            [GUI_EVENTS.MENU_CLICK] = function(self, ev) return MainWindow:SetsMenuClick(self, ev) end,

            [GUI_EVENTS.MENU_SUB_OPEN] = function(self, ev) return MainWindow:MenuSubOpen(ev) end,
        },
    }),
})
end