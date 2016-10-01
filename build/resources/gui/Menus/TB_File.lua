function Gui.TB_File()
return GuiMenu({
    styles = {
        GuiStyles.menu_topbar,
    },
    
    id = "tb_file_menu",
    
    width = 250,
    height = 153, 

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_create",
        top = 1,
        text = { str = "Create scene", },
    }),

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_open",
        top = 26,
        text = { str = "Open scene", },
    }),

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_save",
        top = 51,
        text = { str = "Save scene", },

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
        text = { str = "Save scene as...", },

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
        text = { str = "Close scene", },
    }),

    ----
    
    GuiRect({
        styles = {GuiStyles.menu_dash_default,},
        top = 126,
    }),

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_exit",
        top = 127,
        text = { str = "Выход", },
    }),
})
end