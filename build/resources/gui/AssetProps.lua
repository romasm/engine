function Gui.AssetPropsWindow(header)
return GuiWindow({
    styles = {
        GuiStyles.props_window,
        GuiStyles.window_colors,
    },
    
    cleintarea_padding = { t = 130, },

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

        top = 27,
        left = 2,   
        width = GUI_PREVIEW_SIZE.LIVE_X,   
        height = GUI_PREVIEW_SIZE.LIVE_Y,  

        GuiButton({
            styles = {GuiStyles.empty_button,},
            background = {
                color = 'null',
                color_hover = 'null',
                color_press = 'null',
            },
            cursor = SYSTEM_CURSORS.CROSSARROW,
            fadein_time = 0,
            fadeout_time = 0,
            width_percent = true,   
            height_percent = true,   
            width = 100,   
            height = 100,  
            alt = "Drag to rotate, scroll to zoom",

            events = {
                [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                    self.entity:SetHierarchyFocusOnMe(true)
                    MaterialProps:ProcessPreviewStartMove(self, ev) 
                    return true 
                end,
                [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) 
                    self.entity:SetHierarchyFocusOnMe(false)
                    MaterialProps:ProcessPreviewStopMove(self, ev) 
                    return true 
                end,
                [GUI_EVENTS.BUTTON_MOVE] = function(self, ev) 
                    MaterialProps:ProcessPreviewMove(self, ev) 
                    return true 
                end,
                [GUI_EVENTS.MOUSE_WHEEL] = function(self, ev) 
                    MaterialProps:ProcessPreviewZoom(self, ev) 
                    return true 
                end,
            },
        }),
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