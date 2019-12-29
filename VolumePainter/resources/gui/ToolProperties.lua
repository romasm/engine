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
					GuiStyles.live,
				},

				top = 10,
				height = 130,
        
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

				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.brush_hardness,
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
						min = 0.0,
						max = 1.0,
						decimal = 2,
						overflow_max = false,
					},
					alt = lcl.brush_hardness,

					events = {
						[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
							Brush:SetBrushHardness(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
							Brush:SetBrushHardness(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
							Brush:SetBrushHardness(self:GetValue())
							return true 
						end,
						[GUI_EVENTS.UPDATE] = function(self, ev)
							self:SetValue(Brush.brushHardness)
							return true 
						end,
					},
				}),

				-- brush opacity
				GuiString({
					styles = {GuiStyles.string_props_03,},
					str = lcl.brush_opacity,
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
					top = 90, 
				}),

				GuiButton({
					styles = {GuiStyles.color_button,},
					left = 120,
					top = 90,
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
					GuiStyles.live,
				},

				top = 10,
				height = 210,
        
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

					GuiString({
						styles = {GuiStyles.string_props_03,},
						str = lcl.plane_pos_type,
						left = 10,
						top = 100,
					}),

					GuiCombo({
						styles = {GuiStyles.props_combo,},   
						allow_none = false,
						left = 120,
						top = 100,
						width = 155,
						height = 21,
						list = {
							lcl.plane_pos_abs,
							lcl.plane_pos_cam,
						},
						alt = lcl.plane_pos_type,

						events = {
							[GUI_EVENTS.COMBO_SELECT] = function(self, ev) 
								local type = self:GetSelected() - 1
								WorkingPlane:SetPositioning(type) 
								
								local attachUI = self.entity:GetParent():GetChildById('plane_attach')
								local absUI = self.entity:GetParent():GetChildById('plane_coords')
								if type == 0 then
									attachUI.enable = false
									absUI.enable = true
								else
									attachUI.enable = true
									absUI.enable = false
								end
								return true 
							end,
							[GUI_EVENTS.UPDATE] = function(self, ev) 
								self:SetSelected(WorkingPlane.positioningType + 1)

								local attachUI = self.entity:GetParent():GetChildById('plane_attach')
								local absUI = self.entity:GetParent():GetChildById('plane_coords')
								if WorkingPlane.positioningType == 0 then
									attachUI.enable = false
									absUI.enable = true
								else
									attachUI.enable = true
									absUI.enable = false
								end
								return true 
							end,
						},
					}),

					GuiDumb({
						styles = {
							GuiStyles.live,
						},

						top = 130,
						height = 80,
        
						align = GUI_ALIGN.BOTH,
						left = 0,
						right = 0,

						enable = false,

						id = "plane_attach",

						GuiString({
							styles = {GuiStyles.string_props_03,},
							str = lcl.plane_attach_dist,
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
								min = 0.1,
								max = 2.5,
								decimal = 2,
								overflow_max = true,
							},
							alt = lcl.plane_attach_dist,

							events = {
								[GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
									WorkingPlane:SetAttachDist(self:GetValue())
									return true 
								end,
								[GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
									WorkingPlane:SetAttachDist(self:GetValue())
									return true 
								end,
								[GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
									WorkingPlane:SetAttachDist(self:GetValue())
									return true 
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetValue(WorkingPlane.attachDistance)
									return true 
								end,
							},
						}),
					}),

					GuiDumb({
						styles = {
							GuiStyles.live,
						},

						top = 130,
						height = 80,
        
						align = GUI_ALIGN.BOTH,
						left = 0,
						right = 0,

						enable = true,
        
						id = "plane_coords",
						
						-- position
						GuiString({
							styles = {GuiStyles.string_props_03,},
							str = lcl.plane_pos,
							left = 10,
							top = 3,
						}),

						GuiString({
							styles = {GuiStyles.string_props_04,},
							str = "X",
							left = 70,
							top = 3,
							color = Vector4(1.0, 0.3, 0.3, 1.0),
						}),

						GuiTextfield({
							styles = {
								GuiStyles.props_textfield,
								GuiStyles.float_textfield,
							},
							id = 'pos_x',
							left = 80,
							top = 0,
							width = 55,
							autoupdate_interval = 200.0,

							events = {
								[GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 								
									local pos = VolumeWorld.coreWorld.transform:GetPosition_L (WorkingPlane.planeEnt)
									pos.x = self:GetNum()
									VolumeWorld.coreWorld.transform:SetPosition_L (WorkingPlane.planeEnt, pos)
									TransformControls:UpdateTransform({Tools.transformEntity})
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetNum(VolumeWorld.coreWorld.transform:GetPosition_L(WorkingPlane.planeEnt).x) 
									return true
								end,
							},
						}),

						GuiString({
							styles = {GuiStyles.string_props_04,},
							str = "Y",
							left = 140,
							top = 3,
							color = Vector4(0.3, 1.0, 0.3, 1.0),
						}),

						GuiTextfield({
							styles = {
								GuiStyles.props_textfield,
								GuiStyles.float_textfield,
							},   
							id = 'pos_y',
							left = 150,
							top = 0,
							width = 55,
							autoupdate_interval = 200.0,

							events = {
								[GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 								
									local pos = VolumeWorld.coreWorld.transform:GetPosition_L (WorkingPlane.planeEnt)
									pos.y = self:GetNum()
									VolumeWorld.coreWorld.transform:SetPosition_L (WorkingPlane.planeEnt, pos)
									TransformControls:UpdateTransform({Tools.transformEntity})
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetNum(VolumeWorld.coreWorld.transform:GetPosition_L(WorkingPlane.planeEnt).y) 
									return true
								end,
							},
						}),

						GuiString({
							styles = {GuiStyles.string_props_04,},
							str = "Z",
							left = 210,
							top = 3,
							color = Vector4(0.3, 0.3, 1.0, 1.0),
						}),

						GuiTextfield({
							styles = {
								GuiStyles.props_textfield,
								GuiStyles.float_textfield,
							},  
							id = 'pos_z',
							left = 220,
							top = 0,
							width = 55,
							autoupdate_interval = 200.0,

							events = {
								[GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 								
									local pos = VolumeWorld.coreWorld.transform:GetPosition_L (WorkingPlane.planeEnt)
									pos.z = self:GetNum()
									VolumeWorld.coreWorld.transform:SetPosition_L (WorkingPlane.planeEnt, pos)
									TransformControls:UpdateTransform({Tools.transformEntity})
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetNum(VolumeWorld.coreWorld.transform:GetPosition_L(WorkingPlane.planeEnt).z) 
									return true
								end,
							},
						}),

						-- rotation
						GuiString({
							styles = {GuiStyles.string_props_03,},
							str = lcl.plane_rot,
							left = 10,
							top = 28,
						}),

						GuiString({
							styles = {GuiStyles.string_props_04,},
							str = "P",
							left = 70,
							top = 28,
							color = Vector4(1.0, 0.3, 0.3, 1.0),
						}),

						GuiTextfield({
							styles = {
								GuiStyles.angle_textfield,
							},  
							id = 'rot_x',
							left = 80,
							top = 25,
							width = 55,
							autoupdate_interval = 200.0,

							events = {
								[GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 								
									local rot = VolumeWorld.coreWorld.transform:GetRotationPYR_L (WorkingPlane.planeEnt)
									rot.x = self:GetNum()
									VolumeWorld.coreWorld.transform:SetRotationPYR_L (WorkingPlane.planeEnt, rot)
									TransformControls:UpdateTransform({Tools.transformEntity})
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetNum(VolumeWorld.coreWorld.transform:GetRotationPYR_L(WorkingPlane.planeEnt).x) 
									return true
								end,
							},
						}),

						GuiString({
							styles = {GuiStyles.string_props_04,},
							str = "Y",
							left = 140,
							top = 28,
							color = Vector4(0.3, 1.0, 0.3, 1.0),
						}),

						GuiTextfield({
							styles = {
								GuiStyles.angle_textfield,
							},   
							id = 'rot_y',
							left = 150,
							top = 25,
							width = 55,
							autoupdate_interval = 200.0,

							events = {
								[GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 								
									local rot = VolumeWorld.coreWorld.transform:GetRotationPYR_L (WorkingPlane.planeEnt)
									rot.y = self:GetNum()
									VolumeWorld.coreWorld.transform:SetRotationPYR_L (WorkingPlane.planeEnt, rot)
									TransformControls:UpdateTransform({Tools.transformEntity})
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetNum(VolumeWorld.coreWorld.transform:GetRotationPYR_L(WorkingPlane.planeEnt).y) 
									return true
								end,
							},
						}),

						GuiString({
							styles = {GuiStyles.string_props_04,},
							str = "R",
							left = 210,
							top = 28,
							color = Vector4(0.3, 0.3, 1.0, 1.0),
						}),

						GuiTextfield({
							styles = {
								GuiStyles.angle_textfield,
							},  
							id = 'rot_z',
							left = 220,
							top = 25,
							width = 55,
							autoupdate_interval = 200.0,

							events = {
								[GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 								
									local rot = VolumeWorld.coreWorld.transform:GetRotationPYR_L (WorkingPlane.planeEnt)
									rot.z = self:GetNum()
									VolumeWorld.coreWorld.transform:SetRotationPYR_L (WorkingPlane.planeEnt, rot)
									TransformControls:UpdateTransform({Tools.transformEntity})
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetNum(VolumeWorld.coreWorld.transform:GetRotationPYR_L(WorkingPlane.planeEnt).z) 
									return true
								end,
							},
						}),

						-- scale
						GuiString({
							styles = {GuiStyles.string_props_03,},
							str = lcl.plane_scale,
							left = 10,
							top = 53,
						}),

						GuiString({
							styles = {GuiStyles.string_props_04,},
							str = "X",
							left = 70,
							top = 53,
							color = Vector4(1.0, 0.3, 0.3, 1.0),
						}),

						GuiTextfield({
							styles = {
								GuiStyles.props_textfield,
								GuiStyles.float_textfield,
							},  
							id = 'scale_x',
							left = 80,
							top = 50,
							width = 55,
							autoupdate_interval = 200.0,

							events = {
								[GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 								
									local scale = VolumeWorld.coreWorld.transform:GetScale_L (WorkingPlane.planeEnt)
									scale.x = self:GetNum()
									VolumeWorld.coreWorld.transform:SetScale_L (WorkingPlane.planeEnt, scale)
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetNum(VolumeWorld.coreWorld.transform:GetScale_L(WorkingPlane.planeEnt).x) 
									return true
								end,
							},
						}),

						GuiString({
							styles = {GuiStyles.string_props_04,},
							str = "Y",
							left = 140,
							top = 53,
							color = Vector4(0.3, 1.0, 0.3, 1.0),
						}),

						GuiTextfield({
							styles = {
								GuiStyles.props_textfield,
								GuiStyles.float_textfield,
							},   
							id = 'scale_y',
							left = 150,
							top = 50,
							width = 55,
							autoupdate_interval = 200.0,

							events = {
								[GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 								
									local scale = VolumeWorld.coreWorld.transform:GetScale_L (WorkingPlane.planeEnt)
									scale.y = self:GetNum()
									VolumeWorld.coreWorld.transform:SetScale_L (WorkingPlane.planeEnt, scale)
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetNum(VolumeWorld.coreWorld.transform:GetScale_L(WorkingPlane.planeEnt).y) 
									return true
								end,
							},
						}),

						GuiString({
							styles = {GuiStyles.string_props_04,},
							str = "Z",
							left = 210,
							top = 53,
							color = Vector4(0.3, 0.3, 1.0, 1.0),
						}),

						GuiTextfield({
							styles = {
								GuiStyles.props_textfield,
								GuiStyles.float_textfield,
							},  
							id = 'scale_z',
							left = 220,
							top = 50,
							width = 55,
							autoupdate_interval = 200.0,

							events = {
								[GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) 								
									local scale = VolumeWorld.coreWorld.transform:GetScale_L (WorkingPlane.planeEnt)
									scale.z = self:GetNum()
									VolumeWorld.coreWorld.transform:SetScale_L (WorkingPlane.planeEnt, scale)
								end,
								[GUI_EVENTS.UPDATE] = function(self, ev)
									self:SetNum(VolumeWorld.coreWorld.transform:GetScale_L(WorkingPlane.planeEnt).z) 
									return true
								end,
							},
						}),

					}),
				}),
			}),
    }),
})
end