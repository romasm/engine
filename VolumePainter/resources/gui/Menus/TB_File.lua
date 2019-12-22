function Gui.TB_file()
return GuiMenu({
    styles = {
        GuiStyles.menu_topbar,
    },
    
    id = "tb_file_menu",
    
    height = 204, 

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_create",
		top = 1,
		text = { str = lcl.file_create, },
    }),

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_open",
        top = 26,
		text = { str = lcl.file_open, },
    }),

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_save",
        top = 51,
		text = { str = lcl.file_save, },

        GuiString({
            styles = {GuiStyles.menu_hot_default,},
            str = "Ctrl+S",
            left = 150,
        }),
    }),    

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_saveas",
        top = 76,
        text = { str = lcl.file_saveas, },

        GuiString({
            styles = {GuiStyles.menu_hot_default,},
            str = "Ctrl+Shift+S",
            left = 150,
        }),
    }),   

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_close",
        top = 101,
		text = { str = lcl.file_close, },
    }),
    
    ----
    
    GuiRect({
        styles = {GuiStyles.menu_dash_default,},
        top = 126,
    }),

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_import",
        top = 127,
        text = { str = lcl.file_import, },
    }),

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_export",
        top = 152,
		text = { str = lcl.file_export, },
    }),
    
    ----
    
    GuiRect({
        styles = {GuiStyles.menu_dash_default,},
        top = 177,
    }),
    
    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_exit",
        top = 178,
		text = { str = lcl.file_quit, },
    }),
})
end