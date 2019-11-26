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

				height = 200,
    
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

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_vis_density,
					left = 10,
					top = 60, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 60,

					data = {
						min = 0.0,
						max = 100.0,
						decimal = 1,
						overflow_max = true,
					},
					alt = lcl.volume_vis_density,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetDensityScale(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetDensityScale(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetDensityScale(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.densityScale)
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_vis_solid_thresh,
					left = 10,
					top = 90, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 90,

					data = {
						min = 0.0,
						max = 1.0,
						decimal = 2,
						overflow_max = false,
					},
					alt = lcl.volume_vis_solid_alt,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetSolidThreshold(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetSolidThreshold(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetSolidThreshold(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.solidThreshold)
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_vis_absorp,
					left = 10,
					top = 120, 
				}),

				GuiButton({
					styles = {GuiStyles.color_button,},
					left = 120,
					top = 120,
					width = 155,
					alt = lcl.volume_vis_absorp,

					background = {
						color_nonactive = 'bg_03',
					},

					events = {
						[GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) 
							if self.picker then 
								self.picker = false
								return true
							else
								ColorPicker:Show(self, self.background.color, false)
								self.picker = true
							end
							return true end,
						[GUI_EVENTS.COLOR_PICKING]  = function(self, ev) 
							VolumeWorld:SetAbsorptionColor(ColorPicker:GetColor())
							return true 
						end,
						[GUI_EVENTS.COLOR_PICKED]  = function(self, ev) 
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev) 
							self.background.color = Vector4(VolumeWorld.absorptionColor.x, VolumeWorld.absorptionColor.y, VolumeWorld.absorptionColor.z, 1)
							self.background.color_hover = self.background.color
							self.background.color_press = self.background.color
							self:UpdateProps()
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_vis_absorp_scale,
					left = 10,
					top = 150, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 150,

					data = {
						min = 0.0,
						max = 50.0,
						decimal = 1,
						overflow_max = true,
					},
					alt = lcl.volume_vis_absorp_scale,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetAbsorptionScale(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetAbsorptionScale(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetAbsorptionScale(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.absorptionScale)
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_vis_asymmerty,
					left = 10,
					top = 180, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 180,

					data = {
						min = -0.6,
						max = 0.6,
						decimal = 2,
						overflow_max = false,
					},
					alt = lcl.volume_vis_asymmerty,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetAsymmetry(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetAsymmetry(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetAsymmetry(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.asymmetry)
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