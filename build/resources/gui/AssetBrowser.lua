GuiStyles.material_button = {
    styles = {
        GuiStyles.solid_button,
    },

    holded = true,

    height = 87,
    width = 87,
    
    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'act_00',
        color_nonactive = 'bg_01',
    },

    text = {
        offset = { x = 2, y = 67 },
        center = { x = false, y = false },
        font = "../resources/fonts/opensans_normal_16px",
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02',
    },

    icon = {
        rect = { l = 0, t = 0, w = 87, h = 67 },
    },
}

function Gui.AssetBrowserWindow()
return GuiWindow({
    styles = {
        GuiStyles.props_window,
        GuiStyles.window_colors,
    },

    cleintarea_padding = { t = 50, },
    
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
        }),
    }),
})
end