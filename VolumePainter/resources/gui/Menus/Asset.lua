function Gui.AssetMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "asset_menu",
    
    width = 135,
    height = 102, 
    
    -- children
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_new",

        top = 1,

        text = {
            str = "Create new",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_rename",

        top = 26,

        text = {
            str = "Rename",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_copy",

        top = 51,

        text = {
            str = "Make copy",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_delete",

        top = 76,

        text = {
            str = "Delete",
        },
    }),
})
end