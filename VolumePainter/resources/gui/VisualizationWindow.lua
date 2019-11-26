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

				height = 230,
    
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
					top = 70, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 70,

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
					top = 100, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 100,

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
					top = 140, 
				}),

				GuiButton({
					styles = {GuiStyles.color_button,},
					left = 120,
					top = 140,
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
					top = 170, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 170,

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
					top = 200, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 200,

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

				height = 330,
    
				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_lit_color,
					left = 10,
					top = 30, 
				}),

				GuiButton({
					styles = {GuiStyles.color_button,},
					left = 120,
					top = 30,
					width = 155,
					alt = lcl.volume_lit_color,

					background = {
						color_nonactive = 'bg_03',
					},

					events = {
						[GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) 
							if self.picker then 
								self.picker = false
								return true
							else
								ColorPicker:Show(self, self.background.color, true)
								self.picker = true
							end
							return true end,
						[GUI_EVENTS.COLOR_PICKING]  = function(self, ev) 
							VolumeWorld:SetLightColor(CMath.Color4GammaToLin(ColorPicker:GetColor()))
							return true 
						end,
						[GUI_EVENTS.COLOR_PICKED]  = function(self, ev) 
							return true 
						end,
							[GUI_EVENTS.UPDATE] = function(self, ev) 
							self.background.color = CMath.Color4LinToGamma(Vector4(VolumeWorld.lightColor.x, VolumeWorld.lightColor.y, VolumeWorld.lightColor.z, 1))
							self.background.color_hover = self.background.color
							self.background.color_press = self.background.color
							self:UpdateProps()
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_lit_intens,
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
						max = 20.0,
						decimal = 2,
						overflow_max = true,
					},
					alt = lcl.volume_lit_intens,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetLightIntensity(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetLightIntensity(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetLightIntensity(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.lightIntensity)
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_lit_pitch,
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
						min = -90.0,
						max = 90.0,
						decimal = 1,
						overflow_max = false,
					},
					alt = lcl.volume_lit_pitch,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetLightPitchYaw(self:GetValue(), VolumeWorld.lightYaw)
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetLightPitchYaw(self:GetValue(), VolumeWorld.lightYaw)
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetLightPitchYaw(self:GetValue(), VolumeWorld.lightYaw)
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.lightPitch)
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_lit_yaw,
					left = 10,
					top = 120, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 120,

					data = {
						min = 0.0,
						max = 360.0,
						decimal = 1,
						overflow_max = false,
					},
					alt = lcl.volume_lit_yaw,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetLightPitchYaw(VolumeWorld.lightPitch, self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetLightPitchYaw(VolumeWorld.lightPitch, self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetLightPitchYaw(VolumeWorld.lightPitch, self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.lightYaw)
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_env_color,
					left = 10,
					top = 160, 
				}),

				GuiButton({
					styles = {GuiStyles.color_button,},
					left = 120,
					top = 160,
					width = 155,
					alt = lcl.volume_env_color,

					background = {
						color_nonactive = 'bg_03',
					},

					events = {
						[GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) 
							if self.picker then 
								self.picker = false
								return true
							else
								ColorPicker:Show(self, self.background.color, true)
								self.picker = true
							end
							return true end,
						[GUI_EVENTS.COLOR_PICKING]  = function(self, ev) 
							VolumeWorld:SeEnvColor(CMath.Color4GammaToLin(ColorPicker:GetColor()))
							return true 
						end,
						[GUI_EVENTS.COLOR_PICKED]  = function(self, ev) 
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev) 
							self.background.color = CMath.Color4LinToGamma(Vector4(VolumeWorld.envColor.x, VolumeWorld.envColor.y, VolumeWorld.envColor.z, 1))
							self.background.color_hover = self.background.color
							self.background.color_press = self.background.color
							self:UpdateProps()
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_env_opacity,
					left = 10,
					top = 190, 
				}),

				GuiDataSlider({
					styles = {
						GuiStyles.common_dataslider,
					},

					left = 120,
					width = 155,
					height = 20,
					top = 190,

					data = {
						min = 0.00,
						max = 1.00,
						decimal = 2,
						overflow_max = false,
					},
					alt = lcl.volume_env_opacity,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetEnvOpacity(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetEnvOpacity(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetEnvOpacity(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.envMapOpacity)
							return true 
						end,
					},
				}),

				Gui.Texture({
					width = 265,
					top = 220,
					left = 10,
					id = 'env_texture',
					allow_autoreload = false,
					str = lcl.volume_env_tex,

					events = {
							[GUI_EVENTS.TEXTURE_SET] = function(self, ev) 
								VolumeWorld:SetEnvTexture (self:GetTexture()) 
								return true
							end,
							[GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) 
								VolumeWorld:SetEnvTexture (self:GetTexture()) 
								return true
							end,
							[GUI_EVENTS.UPDATE] = function(self, ev) 
								self:SetTexture( VolumeWorld.materialEnv:GetTextureName("skyTexture", SHADERS.PS) ) 
								return true
							end,
					}
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
						str = lcl.rendering_quality,
						offset = { x = 22, y = 1 },
						center = { x = false, y = false },
					},
				},

				width = 100,
				width_percent = true,

				height = 140,
    
				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_perf_steps,
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
						min = 20,
						max = 100,
						decimal = 0,
						step = 1,
						overflow_max = false,
					},
					alt = lcl.volume_perf_steps,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetStepsCount(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetStepsCount(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetStepsCount(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.stepsCount)
							return true 
						end,
					},
				}),

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.volume_perf_shadow,
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
						min = 5,
						max = 40,
						decimal = 0,
						step = 1,
						overflow_max = false,
					},
					alt = lcl.volume_perf_shadow,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							VolumeWorld:SetShadowStepsCount(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							VolumeWorld:SetShadowStepsCount(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							VolumeWorld:SetShadowStepsCount(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(VolumeWorld.shadowStepsCount)
							return true 
						end,
					},
				}),
			}),
		}),
    }),
})
end