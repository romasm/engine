function Gui.ViewportMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "vp_menu",
    
    width = 135,
    height = 50, 
    
    -- children
    
    GuiSubmenuButton({
        styles = {GuiStyles.menu_button_default,},
        id = "vp_create",
        top = 0,
        text = { str = "Create", },

         GuiMenu({
            styles = {
                GuiStyles.menu_topbar,
            },

            id = "vp_create_sub",
    
            width = 135,
            height = 100, 
    
            offset = {x = 250, y = -25},

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_create_static",
                top = 0,
                text = {str = "Static model",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_create_light",
                top = 25,
                text = {str = "Light",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_create_glight",
                top = 50,
                text = {str = "Global light",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_player",
                top = 75,
                text = {str = "Player",},
            }),
        }),
    }),

    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "vp_dumb",
        top = 25,
        text = { str = "Dumb", },
    }),
})
end