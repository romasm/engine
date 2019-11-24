function Gui.ToolPropertiesWindow()
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
    
    id = "properties_window",
    
    header = {
        styles = {
			GuiStyles.window_header_dynamic,
        },
        str = lcl.property_header,
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

            str = lcl.no_entity_select,

            static = true,

            align = GUI_ALIGN.CENTER,
            valign = GUI_VALIGN.MIDDLE,

            color = 'text_02',
        }),

        GuiBody({
            width = 100,
			width_percent = true,
			
			height = 100,
			height_percent = true,

			GuiDumb({
				styles = {
					GuiStyles.live_ghost,
				},

				top = 10,
				height = 100,
        
				align = GUI_ALIGN.BOTH,
				left = 0,
				right = 0,

				enable = false,
        
				id = "brush_settings",

				-- brush size
				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.brush_size,
					left = 10,
					top = 0, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 0,

					data = {
						min = 1,
						max = 256,
						decimal = 1,
						overflow_max = true,
					},
					alt = lcl.brush_size,

					events = {
							[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
								Brush:SetBrushSize(self:GetValue())
								return true 
							end,
							[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
								Brush:SetBrushSize(self:GetValue())
								return true 
							end,
							[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
								Brush:SetBrushSize(self:GetValue())
								return true 
							end,
							[GUI_EVENTS.UPDATE] = function(self, ev)
								self:SetValue(Brush.brushSize)
								return true 
							end,
						},
				}),

				-- brush opacity
				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.brush_opacity,
					left = 10,
					top = 30, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 30,

					data = {
						min = 0.001,
						max = 1,
						decimal = 3,
					},
					alt = lcl.brush_opacity,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							Brush:SetBrushOpacity(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							Brush:SetBrushOpacity(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							Brush:SetBrushOpacity(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(Brush.brushColor.w)
							return true 
						end,
					},
				}),

				-- brush color
				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.brush_color,
					left = 10,
					top = 60, 
				}),

				GuiButton({
					styles = {GuiStyles.color_button,},
					left = 120,
					top = 60,
					width = 155,
					alt = lcl.brush_color,

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
								local color = ColorPicker:GetColor()
								Brush:SetBrushColor(color)
								return true 
							end,
						[GUI_EVENTS.COLOR_PICKED]  = function(self, ev) 
								return true 
							end,
						[GUI_EVENTS.UPDATE] = function(self, ev) 
								self.background.color = Vector4(Brush.brushColor.x, Brush.brushColor.y, Brush.brushColor.z, 1)
								self.background.color_hover = self.background.color
								self.background.color_press = self.background.color
								self:UpdateProps()
								return true 
							end,
					},
				}),

			}),

			GuiDumb({
				styles = {
					GuiStyles.live_ghost,
				},

				top = 10,
				height = 100,
        
				align = GUI_ALIGN.BOTH,
				left = 0,
				right = 0,

				enable = false,
        
				id = "plane_settings",
					
				-- enable fade
				GuiCheck({
					styles = {GuiStyles.props_check,},
					left = 120,
					top = 0,
					width = 150,
					height = 18,
					text = { str = lcl.plane_visible },
					alt = lcl.plane_visible,

					events = {
						[GUI_EVENTS.CB_CHECKED] = function(self, ev) 
							WorkingPlane:SetPlaneVisible (true)
							return true 
						end,
						[GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
							WorkingPlane:SetPlaneVisible (false)
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev) 
							self:SetCheck(WorkingPlane.planeVisible)
							return true 
						end,
					}
				}),

				-- enable fade
				GuiCheck({
					styles = {GuiStyles.props_check,},
					left = 120,
					top = 30,
					width = 120,
					height = 18,
					text = { str = lcl.plane_fade_enable },
					alt = lcl.plane_fade_enable_alt,

					events = {
							[GUI_EVENTS.CB_CHECKED] = function(self, ev) 
								WorkingPlane:SetPlaneFadeEnable (true)
								return true 
							end,
							[GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
								WorkingPlane:SetPlaneFadeEnable (false)
								return true 
							end,
							[GUI_EVENTS.UPDATE] = function(self, ev) 
								self:SetCheck(WorkingPlane.planeFadeEnable == 1)
								return true 
							end,
					}
				}),

				-- plane fade
				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.plane_fade,
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
						min = 0,
						max = 100,
						decimal = 0,
						overflow_max = true,
					},
					alt = lcl.brush_size,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							WorkingPlane:SetPlaneFade(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							WorkingPlane:SetPlaneFade(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							WorkingPlane:SetPlaneFade(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(WorkingPlane.planeFade)
							return true 
						end,
					},
				}),

			}),
		}),
    }),
})
end