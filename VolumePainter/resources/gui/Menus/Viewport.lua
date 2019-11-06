function Gui.ViewportMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "vp_menu",
    
    width = 135,
    height = 75, 
    
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
            height = 150, 
    
            offset = {x = 250, y = -25},

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_create_static",
                top = 0,
                text = {str = "Static model",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_create_physics",
                top = 25,
                text = {str = "Physics model",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_create_light",
                top = 50,
                text = {str = "Light",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_create_glight",
                top = 75,
                text = {str = "Global light",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_player",
                top = 100,
                text = {str = "Player",},
            }),

            GuiButton({
                styles = {GuiStyles.menu_button_default,},
                id = "vp_envprob",
                top = 125,
                text = {str = "EnvProb",},
            }),
        }),
    }),

    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "vp_newentity",
        top = 25,
        text = { str = "Create Entity", },
    }),

    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "vp_dumb",
        top = 50,
        text = { str = "Dumb", },
    }),
})
end