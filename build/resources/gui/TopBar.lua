GuiStyles.topmenu_button = {
    styles = {
        GuiStyles.solid_button,
    },

    holded = true,

    height = 100,
    height_percent = true,

    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'bg_05',
    },
    
    text = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_02',
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

        left = 12,
        width = 62,

        text = {str = "File"},
        id = 'tb_file',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return MainWindow:FileMenuPress(ev.entity) end,
            [GUI_EVENTS.BUTTON_HOVER] = function(self, ev) return MainWindow:FileMenuHover(ev.entity) end,

            [GUI_EVENTS.MENU_CLOSE] = function(self, ev) return MainWindow:FileMenuClose(self) end,
            [GUI_EVENTS.MENU_CLICK] = function(self, ev) return MainWindow:FileMenuClick(self, ev) end, 
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.topmenu_button,
        },

        left = 85,
        width = 102,

        text = {str = "Configs"},
        id = 'tb_settings',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return MainWindow:SetsMenuPress(ev.entity) end,
            [GUI_EVENTS.BUTTON_HOVER] = function(self, ev) return MainWindow:SetsMenuHover(ev.entity) end,

            [GUI_EVENTS.MENU_CLOSE] = function(self, ev) return MainWindow:SetsMenuClose(self) end,
            [GUI_EVENTS.MENU_CLICK] = function(self, ev) return MainWindow:SetsMenuClick(self, ev) end,

            [GUI_EVENTS.MENU_SUB_OPEN] = function(self, ev) return MainWindow:SetsMenuSub(ev) end,
        },
    }),
})
end