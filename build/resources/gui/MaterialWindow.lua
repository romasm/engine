function Gui.MaterialWindow()
return GuiWindow({
    styles = {
        GuiStyles.props_window,
        GuiStyles.window_colors,
    },
    
    align = GUI_ALIGN.BOTH,
    valign = GUI_VALIGN.BOTH,

    left = 4,
    bottom = 4,
    right = 4,

    id = "material_window",

    header = {
        styles = {
            GuiStyles.window_header,
        },
        str = "Material"
    },
    
    events = {
        [GUI_EVENTS.WIN_SCROLL_WHEEL] = function(self, ev) 
            self.entity:SetHierarchyFocusOnMe(false)
            return false
        end,
    },
    
    GuiClientarea({
        GuiString({
            styles = {
                GuiStyles.ghost,
                GuiStyles.string_autosize,
                GuiStyles.string_25,
            },

            id = 'none_msg',

            str = "No material selected",

            static = true,

            align = GUI_ALIGN.CENTER,
            valign = GUI_VALIGN.MIDDLE,

            color = 'text_02',
        }),

        GuiBody({
            width = 100,
            width_percent = true,
            groupstack = true,

            
        }),
    }),
})
end