function Gui.AssetMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "asset_menu",
    
    width = 135,
    height = 50, 
    
    -- children
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_copy",

        top = 0,

        text = {
            str = "Make copy",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "asset_delete",

        top = 25,

        text = {
            str = "Delete",
        },
    }),
})
end