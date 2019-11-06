function Gui.AltString()
return GuiRect({
    styles = {
        GuiStyles.ghost,
        GuiStyles.no_border,
    },
    
    background = {
        color = 'bg_10',
    },

    focus_mode = GUI_FOCUS_MODE.ONTOP,

    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_16,
        },
        id = 'altstring',
        static = false,
        length = 256,
        padding = {x = 6, y = 2},

        color = 'act_02',
    }),
})
end