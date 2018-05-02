loader.require("ComponentsGui.TransformCallback")

GuiStyles.angle_textfield = {
    styles = {
        GuiStyles.props_textfield,
        GuiStyles.float_textfield,
    },

    data = {
        decimal = 2,
        min = -360,
        max = 360,
    },
}

function Gui.TransformComp()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    
    id = "transform",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        height = 21,

        text = {
            str = "Common",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,
    
    height = 193,

     GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Name",
        left = 10,
        top = 33,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.text_textfield,
        },

        top = 30,
        left = 120,
        width = 155,
        height = 20,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev) return TransformCallback.SetName(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdName(self, ev) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Parent name",
        left = 10,
        top = 58,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.text_textfield,
        },

        top = 55,
        left = 120,
        width = 155,
        height = 20,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev) return TransformCallback.SetParent(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdParent(self, ev) end,
        },
    }),

    -- position
    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Position",
        left = 10,
        top = 88,
    }),

    GuiString({
        styles = {GuiStyles.string_props_04,},
        str = "X",
        left = 70,
        top = 88,
        color = Vector4(1.0, 0.3, 0.3, 1.0),
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },
        id = 'pos_x',
        left = 80,
        top = 85,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetPos(self, ev, 1) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdPos(self, ev, 1) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_04,},
        str = "Y",
        left = 140,
        top = 88,
        color = Vector4(0.3, 1.0, 0.3, 1.0),
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },   
        id = 'pos_y',
        left = 150,
        top = 85,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetPos(self, ev, 2) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdPos(self, ev, 2) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_04,},
        str = "Z",
        left = 210,
        top = 88,
        color = Vector4(0.3, 0.3, 1.0, 1.0),
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },  
        id = 'pos_z',
        left = 220,
        top = 85,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetPos(self, ev, 3) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdPos(self, ev, 3) end,
        },
    }),

    -- rotation
    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Rotation",
        left = 10,
        top = 113,
    }),

    GuiString({
        styles = {GuiStyles.string_props_04,},
        str = "P",
        left = 70,
        top = 113,
        color = Vector4(1.0, 0.3, 0.3, 1.0),
    }),

    GuiTextfield({
        styles = {
            GuiStyles.angle_textfield,
        },  
        id = 'rot_x',
        left = 80,
        top = 110,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetRot(self, ev, 1) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdRot(self, ev, 1) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_04,},
        str = "Y",
        left = 140,
        top = 113,
        color = Vector4(0.3, 1.0, 0.3, 1.0),
    }),

    GuiTextfield({
        styles = {
            GuiStyles.angle_textfield,
        },   
        id = 'rot_y',
        left = 150,
        top = 110,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetRot(self, ev, 2) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdRot(self, ev, 2) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_04,},
        str = "R",
        left = 210,
        top = 113,
        color = Vector4(0.3, 0.3, 1.0, 1.0),
    }),

    GuiTextfield({
        styles = {
            GuiStyles.angle_textfield,
        },  
        id = 'rot_z',
        left = 220,
        top = 110,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetRot(self, ev, 3) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdRot(self, ev, 3) end,
        },
    }),

    -- scale
     GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Scale",
        left = 10,
        top = 138,
    }),

    GuiString({
        styles = {GuiStyles.string_props_04,},
        str = "X",
        left = 70,
        top = 138,
        color = Vector4(1.0, 0.3, 0.3, 1.0),
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },  
        id = 'scale_x',
        left = 80,
        top = 135,
        width = 55,
        data = {
            decimal = 4,
        },

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetScale(self, ev, 1) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdScale(self, ev, 1) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_04,},
        str = "Y",
        left = 140,
        top = 138,
        color = Vector4(0.3, 1.0, 0.3, 1.0),
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },   
        id = 'scale_y',
        left = 150,
        top = 135,
        width = 55,
        data = {
            decimal = 4,
        },

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetScale(self, ev, 2) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdScale(self, ev, 2) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_04,},
        str = "Z",
        left = 210,
        top = 138,
        color = Vector4(0.3, 0.3, 1.0, 1.0),
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },  
        id = 'scale_z',
        left = 220,
        top = 135,
        width = 55,
        data = {
            decimal = 4,
        },

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetScale(self, ev, 3) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdScale(self, ev, 3) end,
        },
	}),

	GuiString({
		styles = {GuiStyles.string_props_03,},
		str = "Mobility",
		left = 10,
		top = 168,
	}),

	GuiCheck({
		styles = {GuiStyles.props_check,},
		left = 120,
		top = 165,
		width = 120,
		height = 18,
		text = { str = "Dynamic" },
		alt = "Dynamic entity is moveable, can be in hierarchy",

		events = {
			[GUI_EVENTS.CB_CHECKED] = function(self, ev) return TransformCallback.SetDynamic(self, ev, true) end,
			[GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return TransformCallback.SetDynamic(self, ev, false) end,
			[GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdDynamic(self, ev) end,
		}
	}),
	})
end