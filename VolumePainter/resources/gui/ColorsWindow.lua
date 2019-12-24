function Gui.ColorsWindow(x_coord, y_coord)
    local colorsList = {
        {'bg_01',       "BG primary main"},
        {'bg_01_v1',    "BG primary #1"},
        {'bg_01_v3',    "BG primary #2"},
        {'bg_03',       "BG primary #3"},
        {'bg_08',       "BG primary #4"},
        {'bg_05',       "BG secondary main"},
        {'bg_05_a4',    "BG secondary #1 alpha"},
        {'bg_05_a6',    "BG secondary #2 alpha"},
        {'bg_05_a7',    "BG secondary #3 alpha"},
        {'bg_10',       "BG secondary #4"},
        {'act_00',      "Important"},
        {'act_00_a0',   "Important alpha"},
        {'act_04',      "Warning"},
        {'act_01',      "GUI #1"},
        {'act_02',      "GUI #2"},
        {'act_03',      "GUI #3"},
        {'act_05',      "GUI #4"},
        {'text_01',     "GUI #5"},
        {'text_02',     "GUI #6"},
        {'text_06',     "GUI #7"},
    }

local window = GuiWindow({
    styles = {
        GuiStyles.integrated_window,
    },
    background = {
        color = 'bg_01',
        color_live = 'bg_01',
        color_nonactive = 'bg_01',
    },
    cleintarea_padding = { r = 120, },
    independent = true,
    closeable = false,
    left = x_coord - 350,
    top = y_coord - 250,
    width = 500,
    height = 550,
    id = "colors_window",
    header_size = 0,
    header = {str = lcl.colorscheme},
    resizeable = {x = false, y = false},
    dragable = true,
    
    events = {
        [GUI_EVENTS.KILL] = function(self, ev) 
            MainWindow.colorsWindow = nil
            return true
        end,
    },
    
    GuiButton({
        styles = {GuiStyles.action_button,},
        top = 10,
        id = 'save_btn',
		text = {str = lcl.save,},
		right = 10,
		width = 100,
		height = 30,
		events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                local preset = Config.GetColorsPresetName()
                Config.SaveColors(preset)
                return true
            end,
        },
    }),

    GuiButton({
        styles = {GuiStyles.action_button,},
        top = 50,
        id = 'reset_btn',
        text = {str = lcl.reset,},
		right = 10,
		width = 100,
		height = 30,
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                local preset = Config.GetColorsPresetName()
                Config.LoadColors(preset)
                CoreGui.UpdateShaderData()

                local ev = HEvent()
                ev.event = GUI_EVENTS.UPDATE
                local body = self.entity:GetParent():GetInherited():GetBody()
                body.entity:SendEvent(ev)   
                return true
            end,
        },
    }),

    GuiClientarea({
        GuiBody({
            width = 345,
            height = 0,

        }),
    }),
})

    local body = window:GetBody()
    local offset_h = 10
    for i = 1, #colorsList do 
        local str = GuiString({
            styles = {GuiStyles.string_props_01,},
            str = colorsList[i][2],
            left = 10,
            top = offset_h,
        })
        body.entity:AttachChild( str.entity )

        local btn = GuiButton({
            styles = {GuiStyles.color_button,},
            left = 180,
            top = offset_h,
            width = 155,
            alt = lcl.pickcolor .." ".. colorsList[i][2],
            background = {color_nonactive = 'bg_03',},
            id = colorsList[i][1],

            events = {
                [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev)
                    if self.picker then 
                        self.picker = false
                        return true
                    else
                        ColorPicker:Show(self, self.background.color, false)
                        self.picker = true
                    end
                    return true
                end,
                [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) 
                    local color = ColorPicker:GetColor()
                    local oldColor = Config.GetColor(self.entity:GetID())
                    color.w = oldColor.w
                    Config.SetColor(self.entity:GetID(), color)
                    CoreGui.UpdateShaderData()
                end,
                [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return true end,
                [GUI_EVENTS.UPDATE] = function(self, ev)                    
                    self.background.color = Config.GetColor(self.entity:GetID())
                    self.background.color.w = 1.0
                    self.background.color_hover = self.background.color
                    self.background.color_press = self.background.color
                    self:UpdateProps()
                    return true
                end,
            },
        })
        body.entity:AttachChild( btn.entity )

        offset_h = offset_h + 30
    end

    body.entity.height = offset_h

    local ev = HEvent()
    ev.event = GUI_EVENTS.UPDATE
    body.entity:SendEvent(ev)   

    return window
end