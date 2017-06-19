function Gui.ColorsWindow(x_coord, y_coord)
    local colorsList = {
        'bg_01',
        'bg_01_v1',
        'bg_01_v3',
        'bg_03',
        'bg_05',
        'bg_05_a4',
        'bg_05_a6',
        'bg_05_a7',
        'bg_08',
        'bg_10',

        'act_00',
        'act_00_a0',
        'act_01',
        'act_02',
        'act_03',
        'act_04',
        'act_05',

        'text_01',
        'text_02',
        'text_06',
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

    border = {
        color = 'bg_01',
        color_live = 'bg_01',
        color_nonactive = 'bg_01',
    },

    independent = true,

    closeable = false,
    close = {
        styles = {
            GuiStyles.close_button_alpha,
        },
    },
    
    left = x_coord - 350,
    top = y_coord - 250,
    width = 570,
    height = 501,

    id = "colors_window",

    header_size = 0,
    header = {
        styles = {
            GuiStyles.window_header,
        },

        str = "Color scheme"
    },
    
    events = {
        [GUI_EVENTS.KILL] = function(self, ev) 
            MainWindow.colorsWindow = nil
            return true
        end,
    },
    
    GuiClientarea({
        GuiBody({
            width = 540,
            height = 460,

        }),
    }),
})

    local body = window:GetBody()
    local offset_h = 10
    local offset_l = 10
    for i = 1, #colorsList do 
        local str = GuiString({
            styles = {GuiStyles.string_props_01,},
            str = colorsList[i],
            left = offset_l,
            top = offset_h,
        })
        body.entity:AttachChild( str.entity )

        local btn = GuiButton({
            styles = {GuiStyles.color_button,},
            left = offset_l + 90,
            top = offset_h,
            width = 155,
            alt = "Pick ".. colorsList[i] .." color",
            background = {color_nonactive = 'bg_03',},
            id = colorsList[i],

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
        if offset_h > 430 then
            offset_l = offset_l + 260
            offset_h = 10

            local rect = GuiRect({
                styles = {GuiStyles.ghost,},
                width = 1,
                valign = GUI_VALIGN.BOTH,
                top = 10,
                bottom = 10,
                left = offset_l,
                background = {color = 'text_02',},        
            })
            body.entity:AttachChild( rect.entity )
            offset_l = offset_l + 10
        end
    end

    local ev = HEvent()
    ev.event = GUI_EVENTS.UPDATE
    body.entity:SendEvent(ev)   

    return window
end