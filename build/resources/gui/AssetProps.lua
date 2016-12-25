function Gui.AssetPropsWindow(header)
return GuiWindow({
    styles = {
        GuiStyles.props_window,
        GuiStyles.window_colors,
    },
    
    cleintarea_padding = { t = 120, },

    align = GUI_ALIGN.BOTH,
    valign = GUI_VALIGN.BOTH,

    left = 4,
    bottom = 4,
    right = 4,
    top = 4,

    id = "asset_props",

    header = {
        styles = {
            GuiStyles.window_header_dynamic,
        },
        str = header
    },
    
    events = {
        [GUI_EVENTS.WIN_SCROLL_WHEEL] = function(self, ev) 
            self.entity:SetHierarchyFocusOnMe(false)
            return false
        end,
    },

    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_25,
        },

        id = 'none_msg',

        str = header .. " isn\'t selected",

        static = true,

        align = GUI_ALIGN.CENTER,
        valign = GUI_VALIGN.MIDDLE,

        color = 'text_02',
    }),
    
    GuiRect({
        styles = {
            GuiStyles.live,
        },
        enable = false,
        material = GuiMaterials.viewport,

        id = "asset_viewport",

        top = 29,
        left = 4,
        width = 284,
        height = 112,

        events = {            
            [GUI_EVENTS.MOUSE_DOWN] = function(self, ev) return true end,
            [GUI_EVENTS.MOUSE_UP] = function(self, ev) return true end,
            [GUI_EVENTS.MOUSE_MOVE] = function(self, ev) return true end,
        },
    }),

    GuiClientarea({

        GuiBody({
            width = 100,
            width_percent = true,
            groupstack = true,
        }),
    }),
})
end