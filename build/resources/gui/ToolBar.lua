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

    GuiButton({
        styles = {
            GuiStyles.tool_button,
        },

        left = 4,

        icon = {material = GuiMaterials.select_icon},

        id = 'tool_none',
        alt = "Select  ( Q )",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                Tools:UnpressAll(self.entity)
                Viewport:SetTransform(TRANSFORM_MODE.NONE)
                return true 
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
                Tools:SetTransform(TRANSFORM_MODE.NONE, self.entity)
                Viewport:SetTransform(TRANSFORM_MODE.NONE)
                return true 
            end,
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.tool_button,
        },

        left = 48,

        icon = {material = GuiMaterials.move_icon},

        id = 'tool_move',
        alt = "Move  ( W )",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                Tools:UnpressAll(self.entity)
                Viewport:SetTransform(TRANSFORM_MODE.MOVE)
                return true 
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
                Tools:SetTransform(TRANSFORM_MODE.NONE, self.entity)
                Viewport:SetTransform(TRANSFORM_MODE.NONE)
                return true 
            end,
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.tool_button,
        },

        left = 92,

        icon = {material = GuiMaterials.rotate_icon},

        id = 'tool_rot',
        alt = "Rotate  ( E )",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                Tools:UnpressAll(self.entity)
                Viewport:SetTransform(TRANSFORM_MODE.ROT)
                return true 
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
                Tools:SetTransform(TRANSFORM_MODE.NONE, self.entity)
                Viewport:SetTransform(TRANSFORM_MODE.NONE)
                return true 
            end,
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.tool_button,
        },

        left = 136,

        icon = {material = GuiMaterials.scale_icon},

        id = 'tool_scale',
        alt = "Scale  ( R )",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                Tools:UnpressAll(self.entity)
                Viewport:SetTransform(TRANSFORM_MODE.SCALE)
                return true 
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
                Tools:SetTransform(TRANSFORM_MODE.NONE, self.entity)
                Viewport:SetTransform(TRANSFORM_MODE.NONE)
                return true 
            end,
        },
	}),
--[[
	GuiButton({
		styles = {
			GuiStyles.tool_button,
		},

		left = 180,

		icon = {material = GuiMaterials.scale_icon},

		id = 'tool_move_snap',
		alt = "Move snap",

		events = {
			[GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
				TransformControls:SetMoveSnaping(true)
				return true 
			end,
			[GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
				TransformControls:SetMoveSnaping(false)
				return true 
			end,
		},
	}),--]]
	})
end