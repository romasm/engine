function Gui.AssetBrowserWindow()
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
    top = 4,

    id = "asset_browser_window",

    header = {
        styles = {
            GuiStyles.window_header,
        },
        str = "Asset Browser"
    },
    
    events = {
        [GUI_EVENTS.WIN_SCROLL_WHEEL] = function(self, ev) 
            self.entity:SetHierarchyFocusOnMe(false)
            return false
        end,
    },
    
    GuiClientarea({
        
        GuiBody({
            width = 100,
            width_percent = true,
            groupstack = true,
        }),
    }),
})
end