function Gui.EntityPropsWindow()
return GuiWindow({
    styles = {
        GuiStyles.props_window,
        GuiStyles.window_colors,
    },
    
    align = GUI_ALIGN.BOTH,
    valign = GUI_VALIGN.BOTH,

    cleintarea_padding = { l = 0, t = 0, r = 0, b = 0 },

    left = 4,
    bottom = 4,
    top = 4,
    right = 0,
    
    id = "properties_window",
    
    header = {
        styles = {
            GuiStyles.window_header,
        },
        str = lcl.property_header,
    },
    
    events = {
        [GUI_EVENTS.WIN_SCROLL_WHEEL] = function(self, ev) 
            self.entity:SetHierarchyFocusOnMe(false)
            return false
        end,
    },
    
	GuiButton({
		styles = {
			GuiStyles.topmenu_button_colors,
		},

		id = "ep_addcomp",
		holded = true,
	
		text = {
			str = lcl.add_comp,
		},
		alt = lcl.add_comp_help,
		
		width = 136,
		height = 25,        
        height_percent = false,
		align = GUI_ALIGN.RIGHT,

		events = {
			[GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return Properties:OpenAddCompMenu(self) end,			
			[GUI_EVENTS.MENU_CLOSE] = function(self, ev) Properties:AddMenuClose(self) end,
			[GUI_EVENTS.MENU_CLICK] = function(self, ev) Properties:AddMenuClick(self, ev) end, 
		},
	}),
	
    GuiClientarea({
        GuiString({
            styles = {
                GuiStyles.ghost,
                GuiStyles.string_autosize,
                GuiStyles.string_25,
            },

            id = 'none_msg',

            str = lcl.no_entity_select,

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