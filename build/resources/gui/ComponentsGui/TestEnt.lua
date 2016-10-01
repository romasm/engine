loader.require("ComponentsGui.TestEntCallback")

function Gui.TestEntComp()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    
    id = "testent",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        height = 23,

        text = {
            str = "Test Ent",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,
    
    height = 65,

    -- rot speed
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Speed",
        left = 10,
        top = 35,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.common_dataslider,
        },

        data = {
            min = 0.05,
            max = 4,
            decimal = 2,
            overflow_max = true,
        },

        top = 33,
        left = 110,
        width = 150,
        height = 20,

        data = {
            min = 0,
            max = 10,
            decimal = 2,
            overflow_max = true,
        },
        alt = "Rotation speed",

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return TestEntCallback.StartSpeed(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return TestEntCallback.DragSpeed(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return TestEntCallback.EndSpeed(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return TestEntCallback.UpdSpeed(self, ev) end,
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