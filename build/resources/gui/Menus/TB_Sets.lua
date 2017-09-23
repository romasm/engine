function Gui.TB_sets()
return GuiMenu({
    styles = {
        GuiStyles.menu_topbar,
    },
    
    id = "tb_sets_menu",
    
    height = 78, 

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_colors",
        top = 1,
        text = { str = "Color scheme", },
    }),

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_config",
        top = 26,
        text = { str = "Configuration", },
    }),
    
    GuiRect({
        styles = {GuiStyles.menu_dash_default,},
        top = 51,
    }),

    ----
    GuiSubmenuButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_sub_dev",
        top = 52,
        text = { str = "DEV", },

         GuiMenu({
            styles = {
                GuiStyles.menu_topbar,
            },

            id = "dev_sub_menu",
    
            height = 103, 
    
            offset = {x = 250, y = -25},

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "tb_dev_shbuf",
                top = 1,
                text = {str = "Shadow buffer",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "tb_dev_skyrebake",
                top = 26,
                text = {str = "Rebake sky prob",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "tb_dev_convert",
                top = 51,
                text = {str = "Convert selection to STM",},
            }),

            GuiRect({
                styles = {GuiStyles.menu_dash_default,},
                top = 76,
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "tb_dev_profiler",
                top = 77,
                text = {str = "Profiler",},
            }),
        }),
    }),
})
end