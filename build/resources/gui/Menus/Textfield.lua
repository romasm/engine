function Gui.TextfieldMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "tf_menu",
    
    width = 135,
    height = 126, 
    
    -- children
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "tf_copy",

        top = 0,

        text = {
            str = "Копировать",
        },
    }),
    
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "tf_cut",

        top = 25,

        text = {
            str = "Вырезать",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "tf_paste",

        top = 50,

        text = {
            str = "Вставить",
        },
    }),

     GuiRect({
        styles = {
            GuiStyles.menu_dash_default,
        },

        top = 75,
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "tf_copyall",

        top = 76,

        text = {
            str = "Копировать все",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "tf_selectall",

        top = 101,

        text = {
            str = "Выделить все",
        },
    }),
})
end