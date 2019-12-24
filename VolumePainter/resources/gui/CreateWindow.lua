GuiStyles.voxel_textfield = {
	styles = {
		GuiStyles.props_textfield,
		GuiStyles.int_textfield,
	},
	data = {
		min = 0,
		max = 1024,
	},
	text = {
		length = 4,
		str = "512"
	},
	top = 35,
	width = 55,
}

function Gui.CreateWindow(x_coord, y_coord)
return GuiWindow({
    styles = {
        GuiStyles.integrated_window,
    },
    background = {
        color = 'bg_01',
        color_live = 'bg_01',
        color_nonactive = 'bg_01',
    },
	independent = true,
	independent_mini = true,
	closeable = false,
    left = x_coord - 150,
    top = y_coord - 100,
    width = 250,
    height = 160,
    id = "create_window",
    header_size = 0,
    header = {str = lcl.create_header},
	resizeable = {x = false, y = false},
	scrollable = {x = false, y = false},
    dragable = true,
    
    events = {
        [GUI_EVENTS.KILL] = function(self, ev) 
            MainWindow.createWindow = nil
            return true
        end,
    },

	GuiClientarea({
		GuiString({
			styles = {GuiStyles.string_props_03,},
			str = lcl.cancel_resolution,
			left = 10,
			top = 10,
		}),

		GuiString({
			styles = {GuiStyles.string_props_04,},
			str = "X",
			left = 10,
			top = 38,
			color = Vector4(1.0, 0.3, 0.3, 1.0),
		}),

		GuiTextfield({
			styles = {
				GuiStyles.voxel_textfield,
			},
			id = 'res_x',
			left = 20,
		}),

		GuiString({
			styles = {GuiStyles.string_props_04,},
			str = "Y",
			left = 80,
			top = 38,
			color = Vector4(0.3, 1.0, 0.3, 1.0),
		}),

		GuiTextfield({
			styles = {
				GuiStyles.voxel_textfield,
			},   
			id = 'res_y',
			left = 90,
		}),

		GuiString({
			styles = {GuiStyles.string_props_04,},
			str = "Z",
			left = 150,
			top = 38,
			color = Vector4(0.3, 0.3, 1.0, 1.0),
		}),

		GuiTextfield({
			styles = {
				GuiStyles.voxel_textfield,
			},  
			id = 'res_z',
			left = 160,
		}),

		GuiButton({
			styles = {GuiStyles.action_button,},
			bottom = 10,
			left = 10,
			width = 100,
			height = 30,
			align = GUI_ALIGN.LEFT,
			valign = GUI_VALIGN.BOTTOM,
			id = 'create_btn',
			text = {str = lcl.create_button,},
			events = {
					[GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
						local clientarea = self.entity:GetParent()
						local resX = clientarea:GetChildById('res_x'):GetInherited():GetNum()
						local resY = clientarea:GetChildById('res_y'):GetInherited():GetNum()
						local resZ = clientarea:GetChildById('res_z'):GetInherited():GetNum()
						if VolumeWorld:CreateWorld(resX, resY, resZ) == 0 then error("Unable to create scene!") end

						local window = clientarea:GetParent():GetInherited() 
						window:Close()
						return true
					end,
			},
		}),

		GuiButton({
			styles = {GuiStyles.action_button,},
			bottom = 10,
			right = 10,
			width = 100,
			height = 30,
			align = GUI_ALIGN.RIGHT,
			valign = GUI_VALIGN.BOTTOM,
			id = 'cancel_btn',
			text = {str = lcl.cancel_button,},
			events = {
				[GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
					local window = self.entity:GetParent():GetParent():GetInherited() 
					window:Close()
					return true
				end,
			},
		}),

		GuiBody({
            width = 0,
            height = 0,		    
		}),
    }),
})
end