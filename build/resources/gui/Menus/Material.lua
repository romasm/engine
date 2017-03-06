function Gui.MaterialMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "material_menu",
    
    width = 180,
    height = 277, 
    
    -- children
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "albedo",

        top = 1,

        text = {
            str = "+ Albedo",
        },
    }),

    
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "roughness",

        top = 26,

        text = {
            str = "+ Microfacets",
        },
    }),

    
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "reflectivity",

        top = 51,

        text = {
            str = "+ Reflectivity",
        },
    }),

    
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "normal",

        top = 76,

        text = {
            str = "+ Normal map",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "ao",

        top = 101,

        text = {
            str = "+ Ambient Occlusion",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "emissive",

        top = 126,

        text = {
            str = "+ Emission",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "scattering",

        top = 151,

        text = {
            str = "+ Scattering",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "transmittance",

        top = 176,

        text = {
            str = "+ Transmittance",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "opacity",

        top = 201,

        text = {
            str = "+ Opacity",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "thickness",

        top = 226,

        text = {
            str = "+ Thickness",
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "alphatest",

        top = 251,

        text = {
            str = "+ Alphatest",
        },
    }),
})
end