function Gui.AddCompMenu()
return GuiMenu({
    styles = {
        GuiStyles.menu_default,
    },

    id = "ac_menu",
    
    width = 135,
    height = 226, 
    
    -- children
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "ac_transform",
        top = 0,
        text = {str = lcl.comp_transform,},
    }),
    
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "ac_vis",
        top = 25,
        text = {str = lcl.comp_vis,},
    }),
    
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "ac_earlyvis",
        top = 50,
        text = {str = lcl.comp_earlyvis,},
    }),
    
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "ac_mesh",
        top = 75,
        text = {str = lcl.comp_mesh,},
    }),
    
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "ac_collision",
        top = 100,
        text = {str = lcl.comp_collision,},
    }),
    
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "ac_script",
        top = 125,
        text = {str = lcl.comp_script,},
    }),
    
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "ac_light",
        top = 150,
        text = {str = lcl.comp_light,},
    }),
    
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "ac_glight",
        top = 175,
        text = {str = lcl.comp_glight,},
    }),
    
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "ac_envprob",
        top = 200,
        text = {str = lcl.comp_envprob,},
    }),
})
end