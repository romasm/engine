function Gui.VisualizationSettingsWindow()
return GuiWindow({
    styles = {
        GuiStyles.props_window,
        GuiStyles.window_colors,
    },
    
    align = GUI_ALIGN.BOTH,
    valign = GUI_VALIGN.BOTH,

    cleintarea_padding = { l = 0, t = 0, r = 0, b = 0 },

    left = 0,
    bottom = 4,
    top = 4,
    right = 4,
    
    id = "visualization_window",
    
    header = {
        styles = {
			GuiStyles.window_header,
        },
		str = lcl.vis_header,
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
				
			height = 100,
			height_percent = true,
		
			GuiGroup({
				styles = {
					GuiStyles.common_group,
				},
    
				header = {
					styles = {
						GuiStyles.group_button,
					},
					align = GUI_ALIGN.BOTH,
					top = 0,
					left = 0,
					right = 0,
					height = 21,

					text = {
						str = lcl.volume_vis,
						offset = { x = 22, y = 1 },
						center = { x = false, y = false },
					},
				},

				width = 100,
				width_percent = true,

				height = 140,
    
				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_vis_type,
					left = 10,
					top = 30,
				}),

				GuiCombo({
					styles = {GuiStyles.props_combo,},   
					allow_none = false,
					left = 120,
					top = 30,
					width = 155,
					height = 21,
					list = {
							lcl.volume_vis_shaded,
							lcl.volume_vis_color,
							lcl.volume_vis_solid,
						},
						alt = lcl.volume_vis_alt,

					events = {
							[GUI_EVENTS.COMBO_SELECT] = function(self, ev) 
								VolumeWorld:SetVisualizationType(self:GetSelected()) 
								return true 
							end,
							[GUI_EVENTS.UPDATE] = function(self, ev) 
								self:SetSelected(VolumeWorld.visualizationType)
								return true 
							end,
					},
				}),
			}),

			GuiGroup({
				styles = {
					GuiStyles.common_group,
				},
    
				header = {
					styles = {
						GuiStyles.group_button,
					},
					align = GUI_ALIGN.BOTH,
					top = 0,
					left = 0,
					right = 0,
					height = 21,

					text = {
						str = lcl.lighting_settings,
						offset = { x = 22, y = 1 },
						center = { x = false, y = false },
					},
				},

				width = 100,
				width_percent = true,

				height = 140,
    
				
			}),

			GuiGroup({
				styles = {
					GuiStyles.common_group,
				},
    
				header = {
					styles = {
						GuiStyles.group_button,
					},
					align = GUI_ALIGN.BOTH,
					top = 0,
					left = 0,
					right = 0,
					height = 21,

					text = {
						str = lcl.rendering_quality,
						offset = { x = 22, y = 1 },
						center = { x = false, y = false },
					},
				},

				width = 100,
				width_percent = true,

				height = 140,
    
				
			}),
		}),
    }),
})
end