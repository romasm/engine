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
        height = 23,

        text = {
            str = "Common",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,
    
    height = 195,

     GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Name",
        left = 10,
        top = 35,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.text_textfield,
        },

        top = 33,
        left = 110,
        width = 150,
        height = 20,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev) return TransformCallback.SetName(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdName(self, ev) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Parent name",
        left = 10,
        top = 65,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.text_textfield,
        },

        top = 63,
        left = 110,
        width = 150,
        height = 20,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev) return TransformCallback.SetParent(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdParent(self, ev) end,
        },
    }),

    -- position
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Position",
        left = 10,
        top = 105,
    }),

    GuiString({
        styles = {GuiStyles.string_props_02,},
        str = "X",
        left = 70,
        top = 105,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },
        id = 'pos_x',
        left = 80,
        top = 103,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetPos(self, ev, 1) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdPos(self, ev, 1) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_02,},
        str = "Y",
        left = 140,
        top = 105,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },   
        id = 'pos_y',
        left = 150,
        top = 103,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetPos(self, ev, 2) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdPos(self, ev, 2) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_02,},
        str = "Z",
        left = 210,
        top = 105,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },  
        id = 'pos_z',
        left = 220,
        top = 103,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetPos(self, ev, 3) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdPos(self, ev, 3) end,
        },
    }),

    -- rotation
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Rotation",
        left = 10,
        top = 135,
    }),

    GuiString({
        styles = {GuiStyles.string_props_02,},
        str = "P",
        left = 70,
        top = 135,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.angle_textfield,
        },  
        id = 'rot_x',
        left = 80,
        top = 133,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetRot(self, ev, 1) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdRot(self, ev, 1) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_02,},
        str = "Y",
        left = 140,
        top = 135,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.angle_textfield,
        },   
        id = 'rot_y',
        left = 150,
        top = 133,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetRot(self, ev, 2) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdRot(self, ev, 2) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_02,},
        str = "R",
        left = 210,
        top = 135,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.angle_textfield,
        },  
        id = 'rot_z',
        left = 220,
        top = 133,
        width = 55,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetRot(self, ev, 3) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdRot(self, ev, 3) end,
        },
    }),

    -- scale
     GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Scale",
        left = 10,
        top = 165,
    }),

    GuiString({
        styles = {GuiStyles.string_props_02,},
        str = "X",
        left = 70,
        top = 165,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },  
        id = 'scale_x',
        left = 80,
        top = 163,
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
        styles = {GuiStyles.string_props_02,},
        str = "Y",
        left = 140,
        top = 165,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },   
        id = 'scale_y',
        left = 150,
        top = 163,
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
        styles = {GuiStyles.string_props_02,},
        str = "Z",
        left = 210,
        top = 165,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },  
        id = 'scale_z',
        left = 220,
        top = 163,
        width = 55,
        data = {
            decimal = 4,
        },

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return TransformCallback.SetScale(self, ev, 3) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TransformCallback.UpdScale(self, ev, 3) end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
})
end