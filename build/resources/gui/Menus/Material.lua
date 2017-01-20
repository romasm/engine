function Gui.MaterialMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "material_menu",
    
    width = 180,
    height = 102, 
    
    -- children
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "add_ao",

        top = 1,

        text = {
            str = "+ Ambient Occlusion",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "add_emissive",

        top = 26,

        text = {
            str = "+ Emission",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "add_sss",

        top = 51,

        text = {
            str = "+ Light Transmittance",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "add_alphatest",

        top = 76,

        text = {
            str = "+ Alphatest",
        },
    }),
})
end