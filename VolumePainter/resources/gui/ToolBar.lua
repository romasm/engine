function Gui.ToolBar()
return GuiRect({
    styles = {
        GuiStyles.live,
    },
    align = GUI_ALIGN.BOTH,
    valign = GUI_VALIGN.TOP,

    height = 40,
    top = 4,
    bottom = 4,

    background = {
        color = 'bg_01',
    },

    id = "toolbar_window",

	-- TOOLS
	GuiButton({
        styles = {
            GuiStyles.tool_button,
        },

        left = 4,

        icon = {material = GuiMaterials.select_icon},

		id = 'tool_brush',
        alt = "Brush  ( 1 )",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
				Tools:SetToolMode(TOOL_MODE.BRUSH)
				return true 
            end,
        },
    }),
		
	GuiButton({
		styles = {
			GuiStyles.tool_button,
		},

		left = 48,

		icon = {material = GuiMaterials.select_icon},

		id = 'tool_plane',
		alt = "Working plane  ( 2 )",

		events = {
			[GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
				Tools:SetToolMode(TOOL_MODE.PLANE)
				return true 
			end,
		},
	}),
	
	-- TRANSFORM
	GuiButton({
        styles = {
            GuiStyles.tool_button,
        },

        left = 148,

        icon = {material = GuiMaterials.move_icon},

        id = 'tool_move',
        alt = "Move  ( W )",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
				Tools:SetTransform(TRANSFORM_MODE.MOVE, self.entity)
				return true 
            end,
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.tool_button,
        },

        left = 192,

        icon = {material = GuiMaterials.rotate_icon},

        id = 'tool_rot',
        alt = "Rotate  ( E )",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
				Tools:SetTransform(TRANSFORM_MODE.ROT, self.entity)
				return true 
            end,
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.tool_button,
        },

        left = 236,

        icon = {material = GuiMaterials.scale_icon},

        id = 'tool_scale',
        alt = "Scale  ( R )",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
				Tools:SetTransform(TRANSFORM_MODE.SCALE, self.entity)
                return true 
            end,
        },
	}),

	--[[GuiButton({
		styles = {
			GuiStyles.tool_button,
		},

		left = 180,

		icon = {material = GuiMaterials.scale_icon},

		id = 'tool_move_snap',
		alt = "Move snap",

		events = {
			[GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
				TransformControls:SetMoveSnapping(true)
				return true 
			end,
			[GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
				TransformControls:SetMoveSnapping(false)
				return true 
			end,
		},
	}),--]]
	})
end