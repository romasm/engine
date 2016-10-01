if not GuiDataSlider then GuiDataSlider = class(GuiEntity) end

-- private
local function UpdateSlider(self, coord)
    local rect = self.entity:GetRectAbsolute()
    local border_double = self.btn_slider.border.width * 2

    local slider_size = coord - rect.l - self.btn_slider.border.width
    local slider_max = rect.w - border_double
    slider_size = math.max(0, math.min(slider_size, slider_max))

    if self.step > 0 then
        local estimate_value = (self.max - self.min) * slider_size / slider_max + self.min

        local step_count = math.floor(estimate_value / self.step + 0.5)
        self.value = step_count * self.step
        self.value = math.max(self.min, math.min(self.value, self.max))

        slider_size = slider_max * (self.value - self.min) / (self.max - self.min)
        slider_size = math.floor(slider_size)
    else
        self.value = (self.max - self.min) * slider_size / slider_max + self.min
    end

    self.btn_slider.icon.rect.w = slider_size
    self.btn_slider:onMoveResize(false, false)

    self.textfield:SetNum(self.value)
end

-- public
function GuiDataSlider:init(props)
    self.min = 0
    self.max = 0
    self.step = 0
    self.overflow_min = false
    self.overflow_max = false

    self.slider = {
        icon = {
            material = GuiMaterials.data_slider
        },
        background = {},
        border = {},
    }
    self.text = {
        background = {
            color = Vector4(0.0, 0.0, 0.0, 0.0),
            color_nonactive = Vector4(0.0, 0.0, 0.0, 0.0)
        },
        border = {
            color = Vector4(0.0, 0.0, 0.0, 0.0),
            color_nonactive = Vector4(0.0, 0.0, 0.0, 0.0),
        },
        text = {
            center = {x = false, y = false},
            offset = {},
        },
        data = {
            d_type = GUI_TEXTFIELD.FLOAT,
        },
    }
    
    self._base.init(self, props)
    
    self.btn_slider = GuiButton(self.slider)
    self.entity:AttachChild(self.btn_slider.entity)
    self.textfield = GuiTextfield(self.text)
    self.textfield.allow_none = true
    self.entity:AttachChild(self.textfield.entity)
    
    self.slider = nil
    self.text = nil

    self.state_move = false

    self.undef = false
    
    self.value = self.min
    self.textfield:SetNum(self.value)
end

function GuiDataSlider:ApplyProps(props)
    self._base.ApplyProps(self, props)

    self.slider.ignore_events = false
    self.slider.collide_through = false

    self.text.ignore_events = true
    self.text.collide_through = true

    if props.alt ~= nil then self.slider.alt = props.alt end
    if props.cursor ~= nil then self.slider.cursor = props.cursor end

    if props.fadein_time ~= nil then self.slider.fadein_time = props.fadein_time end
    if props.fadeout_time ~= nil then self.slider.fadeout_time = props.fadeout_time end
    
    self.slider.height = 100
    self.slider.height_percent = true
    self.slider.width = 100
    self.slider.width_percent = true
    
    self.text.height = 100
    self.text.height_percent = true
    self.text.width = 100
    self.text.width_percent = true

    if props.bar ~= nil then
        if props.bar.color ~= nil then 
            self.slider.icon.color = type(props.bar.color) == 'string' and CoreGui.GetColor(props.bar.color) or props.bar.color end
        if props.bar.color_hover ~= nil then 
            self.slider.icon.color_hover = type(props.bar.color_hover) == 'string' and CoreGui.GetColor(props.bar.color_hover) or props.bar.color_hover end
        if props.bar.color_press ~= nil then 
            self.slider.icon.color_press = type(props.bar.color_press) == 'string' and CoreGui.GetColor(props.bar.color_press) or props.bar.color_press end
        if props.bar.color_nonactive ~= nil then 
            self.slider.icon.color_nonactive = type(props.bar.color_nonactive) == 'string' and CoreGui.GetColor(props.bar.color_nonactive) or props.bar.color_nonactive end
    end

    if props.background ~= nil then
        if props.background.color ~= nil then 
            self.slider.background.color = type(props.background.color) == 'string' and CoreGui.GetColor(props.background.color) or props.background.color end
        if props.background.color_hover ~= nil then 
            self.slider.background.color_hover = type(props.background.color_hover) == 'string' and CoreGui.GetColor(props.background.color_hover) or props.background.color_hover end
        if props.background.color_press ~= nil then 
            self.slider.background.color_press = type(props.background.color_press) == 'string' and CoreGui.GetColor(props.background.color_press) or props.background.color_press end
        if props.background.color_nonactive ~= nil then 
            self.slider.background.color_nonactive = type(props.background.color_nonactive) == 'string' and CoreGui.GetColor(props.background.color_nonactive) or props.background.color_nonactive end
        if props.background.color_text ~= nil then 
            self.text.background.color_live = type(props.background.color_text) == 'string' and CoreGui.GetColor(props.background.color_text) or props.background.color_text end
    end

    if props.border ~= nil then
        if props.border.color ~= nil then 
            self.slider.border.color = type(props.border.color) == 'string' and CoreGui.GetColor(props.border.color) or props.border.color end
        if props.border.color_hover ~= nil then 
            self.slider.border.color_hover = type(props.border.color_hover) == 'string' and CoreGui.GetColor(props.border.color_hover) or props.border.color_hover end
        if props.border.color_press ~= nil then
            self.slider.border.color_press = type(props.border.color_press) == 'string' and CoreGui.GetColor(props.border.color_press) or props.border.color_press
            self.text.border.color_live = self.slider.border.color_press
        end
        if props.border.color_nonactive ~= nil then 
            self.slider.border.color_nonactive = type(props.border.color_nonactive) == 'string' and CoreGui.GetColor(props.border.color_nonactive) or props.background.color_nonactive end
        if props.border.width ~= nil then 
            self.slider.border.width = props.border.width 
            self.text.border.width = props.border.width 
        end
    end
    
    if props.text ~= nil then
        if props.text.color ~= nil then 
            self.text.text.color = type(props.text.color) == 'string' and CoreGui.GetColor(props.text.color) or props.text.color end
        if props.text.color_live ~= nil then 
            self.text.text.color_live = type(props.text.color_live) == 'string' and CoreGui.GetColor(props.text.color_live) or props.text.color_live end
        if props.text.color_nonactive ~= nil then 
            self.text.text.color_nonactive = type(props.text.color_nonactive) == 'string' and CoreGui.GetColor(props.text.color_nonactive) or props.background.color_nonactive end
        
        if props.text.color_selection ~= nil then 
            self.text.color_selection = type(props.text.color_selection) == 'string' and CoreGui.GetColor(props.text.color_selection) or props.text.color_selection end
        if props.text.color_cursor ~= nil then 
            self.text.color_cursor = type(props.text.color_cursor) == 'string' and CoreGui.GetColor(props.text.color_cursor) or props.text.color_cursor end
        
        if props.text.font ~= nil then self.text.text.font = props.text.font end
        if props.text.length ~= nil then self.text.text.length = props.text.length end
        if props.text.offset ~= nil then 
            if props.text.offset.x ~= nil then self.text.text.offset.x = props.text.offset.x end
            if props.text.offset.y ~= nil then self.text.text.offset.y = props.text.offset.y end
        end
        if props.text.center ~= nil then 
            if props.text.center.x ~= nil then self.text.text.center.x = props.text.center.x end
            if props.text.center.y ~= nil then self.text.text.center.y = props.text.center.y end
        end
    end

    if props.data ~= nil then
        if props.data.overflow_min ~= nil then self.overflow_min = props.data.overflow_min end
        if props.data.overflow_max ~= nil then self.overflow_max = props.data.overflow_max end
        if props.data.min ~= nil then 
            self.min = props.data.min
            if self.overflow_min then self.text.data.min = GUI_TEXTFIELD_NUMBERLIMITS.MIN
            else self.text.data.min = props.data.min end
        end
        if props.data.max ~= nil then 
            self.max = props.data.max
            if self.overflow_max then self.text.data.max = GUI_TEXTFIELD_NUMBERLIMITS.MAX
            else self.text.data.max = props.data.max end
        end
        if props.data.step ~= nil then self.step = props.data.step end
        if props.data.decimal ~= nil then 
            self.text.data.decimal = props.data.decimal 
            if self.text.data.decimal == 0 then 
                self.text.data.d_type = GUI_TEXTFIELD.INT 
                self.step = math.max( math.floor(self.step), 1 )
            end
        end
    end
end

function GuiDataSlider:onMoveResize(is_move, is_resize)
    if not self._base.onMoveResize(self, is_move, is_resize) then return false end

    local rect = self.entity:GetRectAbsolute()
    local border_double = self.btn_slider.border.width * 2

    local slider_size = (rect.w - border_double) * (math.max(self.min, math.min(self.value, self.max)) - self.min) / (self.max - self.min)
    slider_size = math.floor(slider_size)
    
    self.btn_slider.icon.rect.w = slider_size
    self.btn_slider.icon.rect.h = rect.h - border_double
    self.btn_slider.icon.rect.l = self.btn_slider.border.width
    self.btn_slider.icon.rect.t = self.btn_slider.border.width
    self.btn_slider:onMoveResize(true, true)

    return true
end

function GuiDataSlider:callback(eventData)
    local res = eventData
    
    if not self.entity:IsActivated() or not self.entity:IsActivatedBranch() then
        if eventData.event == GUI_EVENTS.MOUSE_DOWN then
            self.entity:SetHierarchyFocusOnMe(false)
            res.event = GUI_EVENTS.DO_DENIED
        elseif not (eventData.event == GUI_EVENTS.UPDATE or eventData.event == GUI_EVENTS.SYS_MOVE or 
            eventData.event == GUI_EVENTS.SYS_RESIZE or eventData.event == GUI_EVENTS.MOUSE_WHEEL) then
            res.event = GUI_EVENTS.NULL
        end
        res.entity = self.entity
        return self._base.callback(self, res)
    end

    if eventData.event == GUI_EVENTS.MOUSE_DOWN then 
        if eventData.key == KEYBOARD_CODES.KEY_RBUTTON or eventData.key == KEYBOARD_CODES.KEY_MBUTTON then
            self.textfield.entity.ignore_events = false
            self.textfield.entity.collide_through = false

            if self.undef then
                self.textfield:SetText("")
            end

            local fake_event = HEvent()
            fake_event.event = GUI_EVENTS.MOUSE_DOWN
            fake_event.key = KEYBOARD_CODES.KEY_LBUTTON
            fake_event.coords = eventData.coords
            self.textfield:callback(fake_event)
            fake_event.event = GUI_EVENTS.MOUSE_DBLCLICK
            self.textfield:callback(fake_event)

            res.event = GUI_EVENTS.SLIDER_START_DRAG
            res.entity = self.entity
        end
        
    elseif eventData.event == GUI_EVENTS.BUTTON_PRESSED then 
        if eventData.entity:is_eq(self.btn_slider.entity) then
            self.state_move = true
            self.entity:SetHierarchyFocus(self.btn_slider.entity, true)
            UpdateSlider(self, eventData.coords.x)
            self.undef = false
            res.event = GUI_EVENTS.SLIDER_START_DRAG
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.BUTTON_UNPRESSED then 
        if eventData.entity:is_eq(self.btn_slider.entity) then
            self.state_move = false
            self.entity:SetHierarchyFocus(self.btn_slider.entity, false)
            self.undef = false
            res.event = GUI_EVENTS.SLIDER_END_DRAG
            res.entity = self.entity
            
            local fake_event = HEvent()
            fake_event.collide = self.btn_slider.entity:IsCollide(eventData.coords.x, eventData.coords.y)
            if fake_event.collide then fake_event.event = GUI_EVENTS.MOUSE_HOVER
            else fake_event.event = GUI_EVENTS.MOUSE_OUT end
            self.btn_slider:callback(fake_event)
        end

    elseif eventData.event == GUI_EVENTS.BUTTON_MOVE then 
        if self.state_move and eventData.entity:is_eq(self.btn_slider.entity) then
            local slider_rect = self.btn_slider.entity:GetRectAbsolute()
            UpdateSlider(self, eventData.coords.x)
            self.undef = false
            res.event = GUI_EVENTS.SLIDER_DRAG
            res.entity = self.entity
        end
        
    elseif eventData.event == GUI_EVENTS.KEY_DOWN then
        if eventData.key == KEYBOARD_CODES.KEY_RIGHT then
            local step = self.step
            if step == 0 then step = GUI_SLIDER_MINSTEP * self.value end
            self:SetValue(self.value + step) 
            res.event = GUI_EVENTS.SLIDER_END_DRAG
            res.entity = self.entity
        elseif eventData.key == KEYBOARD_CODES.KEY_LEFT then
            local step = self.step
            if step == 0 then step = GUI_SLIDER_MINSTEP * self.value end
            self:SetValue(self.value - step) 
            res.event = GUI_EVENTS.SLIDER_END_DRAG
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.UNFOCUS then 
        if eventData.entity:is_eq(self.entity) then
            self.entity:SetFocus(HEntity(), false)
        end

    elseif eventData.event == GUI_EVENTS.TF_DEACTIVATE then 
        if eventData.entity:is_eq(self.textfield.entity) then
            self.textfield.entity.ignore_events = true
            self.textfield.entity.collide_through = true
            
            if self.textfield.text.str:len() == 0 then
                if self.undef then 
                    self.textfield:SetText("?")
                else
                    self.textfield:SetNum(self.value)
                end
            else
                self.undef = false
                self:SetValue(self.textfield:GetNum())
            end
                        
            res.event = GUI_EVENTS.SLIDER_END_DRAG
            res.entity = self.entity
        end
    end
    
    res.entity = self.entity

    return self._base.callback(self, res)
end

function GuiDataSlider:GetValue()
    if self.undef then return nil end
    return self.value
end

function GuiDataSlider:SetValue(value)
    if value == nil then
        self.value = self.min
        self:onMoveResize(false, false)
        self.textfield:SetText("?")
        self.undef = true
        return
    end
    self.undef = false

    self.value = value
    if self.step > 0 then
        local step_count = math.floor(self.value / self.step + 0.5)
        self.value = step_count * self.step
    end
    if not self.overflow_max then self.value = math.min(self.value, self.max) end
    if not self.overflow_min then self.value = math.max(self.value, self.min) end
    self:onMoveResize(false, false)
    self.textfield:SetNum(self.value)
end