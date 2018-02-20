function Gui.AddCompMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "ac_menu",
    
    width = 135,
    height = 126, 
    
    -- children
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "ac_transform",

        top = 0,

        text = {
            str = lcl.comp_transform,
        },
    }),
    
    GuiButton({
        styles = {
            GuiStyles.menu_button_default,
        },

        id = "ac_vis",

        top = 25,

        text = {
            str = lcl.comp_vis,
        },
    }),

   
})
end