function Gui.AssetMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "asset_menu",
    
    width = 135,
    height = 100, 
    
    -- children
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_new",

        top = 0,

        text = {
            str = "Create new",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_rename",

        top = 25,

        text = {
            str = "Rename",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_copy",

        top = 50,

        text = {
            str = "Make copy",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_delete",

        top = 75,

        text = {
            str = "Delete",
        },
    }),
})
end