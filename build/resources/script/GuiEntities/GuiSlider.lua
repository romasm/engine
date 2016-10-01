if not GuiSlider then GuiSlider = class(GuiEntity) end

-- private
local function UpdateSlider(self, delta)
    local rect = self.entity:GetRectAbsolute()
    if self.orient == GUI_SLIDER_ORIENT.HORZ then
        local max_left = rect.w - (self.btn_slider.entity.width_percent and self.btn_slider.entity.width * 0.01 * rect.w or self.btn_slider.entity.width)
        self.btn_slider.entity.left = math.max(0, math.min(self.btn_slider.entity.left + delta, max_left))
        self.value = self.btn_slider.entity.left / max_left
    else
        local max_top = rect.h - (self.btn_slider.entity.height_percent and self.btn_slider.entity.height * 0.01 * rect.h or self.btn_slider.entity.height)
        self.btn_slider.entity.top = math.max(0, math.min(self.btn_slider.entity.top + delta, max_top))
        self.value = self.btn_slider.entity.top / max_top     
    end
    self.btn_slider.entity:UpdatePos()
end

local function SetSliderPos(self, coords)
    local rect = self.entity:GetRectAbsolute()
    if self.orient == GUI_SLIDER_ORIENT.HORZ then
        local slider_width = self.btn_slider.entity.width_percent and self.btn_slider.entity.width * 0.01 * rect.w or self.btn_slider.entity.width
        local max_left = rect.w - slider_width
        self.btn_slider.entity.left = math.max(0, math.min(coords - slider_width / 2 - rect.l, max_left)) 
        self.value = self.btn_slider.entity.left / max_left       
    else
        local slider_height = self.btn_slider.entity.height_percent and self.btn_slider.entity.height * 0.01 * rect.h or self.btn_slider.entity.height
        local max_top = rect.h - slider_height
        self.btn_slider.entity.top = math.max(0, math.min(coords - slider_height / 2 - rect.t, max_top))  
        self.value = self.btn_slider.entity.top / max_top          
    end
    self.btn_slider.entity:UpdatePos()
end

-- public
function GuiSlider:init(props)
    self.slider_bound = false
    self.orient = 0

    self.slider_size = 10
    self.slider_size_percent = false
    
    self.axis_size = 10
    self.axis_size_percent = false

    self.slider = {}
    self.axis = {}
    
    self._base.init(self, props)
    
    self.btn_axis = GuiButton(self.axis)
    self.entity:AttachChild(self.btn_axis.entity)
    self.btn_slider = GuiButton(self.slider)
    self.entity:AttachChild(self.btn_slider.entity)
    
    self.slider = nil
    self.axis = nil

    self.state_move = false
    self.hold_offset = 0

    self.value = 0
end

function GuiSlider:ApplyProps(props)
    self._base.ApplyProps(self, props)

    if props.alt ~= nil then
        self.slider.alt = props.alt
        self.axis.alt = props.alt
    end
    if props.slider_bound ~= nil then self.slider_bound = props.slider_bound end
    if props.orient ~= nil then self.orient = props.orient end

    if props.slider_size ~= nil then self.slider_size = props.slider_size end
    if props.slider_size_percent ~= nil then self.slider_size_percent = props.slider_size_percent end

    if props.axis_size ~= nil then self.axis_size = props.axis_size end
    if props.axis_size_percent ~= nil then self.axis_size_percent = props.axis_size_percent end

    if props.slider ~= nil then
        for i,v in pairs(props.slider) do
            self.slider[i] = v
        end
    end
    if props.axis ~= nil then
        for i,v in pairs(props.axis) do
            self.axis[i] = v
        end
    end

    self.slider.ignore_events = false
    self.slider.collide_through = false

    self.axis.ignore_events = false
    self.axis.collide_through = false
    
    if self.orient == GUI_SLIDER_ORIENT.HORZ then
        self.slider.height = 100
        self.slider.height_percent = true
        self.slider.top = 0
        self.slider.left_percent = false
        self.slider.width = self.slider_size
        self.slider.width_percent = self.slider_size_percent
        
        if self.slider_bound then
            self.axis.width = 100
            self.axis.width_percent = true
            self.axis.left = 0
        else
            self.axis.right = self.slider_size / 2
            self.axis.right_percent = self.slider_size_percent
            self.axis.left = self.axis.right
            self.axis.left_percent = self.slider_size_percent
            self.axis.align = GUI_ALIGN.BOTH
        end

        self.axis.top = 0
        self.axis.valign = GUI_VALIGN.MIDDLE
        self.axis.height = self.axis_size
        self.axis.height_percent = self.axis_size_percent
    else
        self.slider.height = self.slider_size
        self.slider.height_percent = self.slider_size_percent
        self.slider.left = 0
        self.slider.top_percent = false
        self.slider.width = 100
        self.slider.width_percent = true

        if self.slider_bound then
            self.axis.height = 100
            self.axis.height_percent = true
            self.axis.top = 0
        else
            self.axis.bottom = self.slider_size / 2
            self.axis.bottom_percent = self.slider_size_percent
            self.axis.top = self.axis.bottom
            self.axis.top_percent = self.slider_size_percent
            self.axis.valign = GUI_VALIGN.BOTH
        end

        self.axis.width = self.axis_size
        self.axis.width_percent = self.axis_size_percent
        self.axis.left = 0
        self.axis.align = GUI_ALIGN.CENTER
    end
end

function GuiSlider:onMoveResize(is_move, is_resize)
    if not self._base.onMoveResize(self, is_move, is_resize) then return false end

    self:SetSlider(self.value)

    return true
end

function GuiSlider:callback(eventData)
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
        self.entity:SetHierarchyFocusOnMe(false)

    elseif eventData.event == GUI_EVENTS.BUTTON_PRESSED then 
        local slider_rect = self.btn_slider.entity:GetRectAbsolute()
        if eventData.entity:is_eq(self.btn_slider.entity) then
            self.state_move = true
            self.hold_offset = self.orient == GUI_SLIDER_ORIENT.HORZ and (eventData.coords.x - slider_rect.l) or (eventData.coords.y - slider_rect.t)
            self.entity:SetHierarchyFocus(self.btn_slider.entity, true)
            res.event = GUI_EVENTS.SLIDER_START_DRAG

        elseif eventData.entity:is_eq(self.btn_axis.entity) then
            self.hold_offset = self.orient == GUI_SLIDER_ORIENT.HORZ and slider_rect.w / 2 or slider_rect.h / 2
            SetSliderPos(self, self.orient == GUI_SLIDER_ORIENT.HORZ and eventData.coords.x or eventData.coords.y)
            self.btn_axis:SetPressed(false)
            self.btn_slider:SetPressed(true)
            self.state_move = true
            self.entity:SetHierarchyFocus(self.btn_slider.entity, true)
            res.event = GUI_EVENTS.SLIDER_START_DRAG
        end

    elseif eventData.event == GUI_EVENTS.BUTTON_UNPRESSED then 
        if eventData.entity:is_eq(self.btn_slider.entity) then
            self.state_move = false
            self.entity:SetHierarchyFocus(self.btn_slider.entity, false)
            res.event = GUI_EVENTS.SLIDER_END_DRAG

            local fake_event = HEvent()
            fake_event.collide = self.btn_slider.entity:IsCollide(eventData.coords.x, eventData.coords.y)
            if fake_event.collide then fake_event.event = GUI_EVENTS.MOUSE_HOVER
            else fake_event.event = GUI_EVENTS.MOUSE_OUT end
            self.btn_slider:callback(fake_event)
        end

    elseif eventData.event == GUI_EVENTS.BUTTON_MOVE then 
        if self.state_move and eventData.entity:is_eq(self.btn_slider.entity) then
            local slider_rect = self.btn_slider.entity:GetRectAbsolute()
            local delta = self.orient == GUI_SLIDER_ORIENT.HORZ and eventData.coords.x - slider_rect.l - self.hold_offset or eventData.coords.y - slider_rect.t - self.hold_offset
            if delta ~= 0 and not (self.value == 1 and delta > 0) and not (self.value == 0 and delta < 0) then 
                UpdateSlider(self, delta)
                res.event = GUI_EVENTS.SLIDER_DRAG
            end
        end

    elseif eventData.event == GUI_EVENTS.KEY_DOWN then
        if eventData.key == KEYBOARD_CODES.KEY_RIGHT then
            if self.orient == GUI_SLIDER_ORIENT.HORZ then 
                self:SetSlider(self.value + GUI_SLIDER_MINSTEP) 
                res.event = GUI_EVENTS.SLIDER_END_DRAG
            end
        elseif eventData.key == KEYBOARD_CODES.KEY_LEFT then
            if self.orient == GUI_SLIDER_ORIENT.HORZ then 
                self:SetSlider(self.value - GUI_SLIDER_MINSTEP) 
                res.event = GUI_EVENTS.SLIDER_END_DRAG
            end
        elseif eventData.key == KEYBOARD_CODES.KEY_DOWN then
            if self.orient == GUI_SLIDER_ORIENT.VERT then 
                self:SetSlider(self.value - GUI_SLIDER_MINSTEP) 
                res.event = GUI_EVENTS.SLIDER_END_DRAG
            end
        elseif eventData.key == KEYBOARD_CODES.KEY_UP then
            if self.orient == GUI_SLIDER_ORIENT.VERT then 
                self:SetSlider(self.value + GUI_SLIDER_MINSTEP) 
                res.event = GUI_EVENTS.SLIDER_END_DRAG
            end
        end
    end
    
    res.entity = self.entity

    return self._base.callback(self, res)
end

function GuiSlider:SetSlider(percent)
    self.value = math.max(0, math.min(1, percent))
    local rect = self.entity:GetRectAbsolute()
    if self.orient == GUI_SLIDER_ORIENT.HORZ then
        local max_left = rect.w - (self.btn_slider.entity.width_percent and self.btn_slider.entity.width * 0.01 * rect.w or self.btn_slider.entity.width)
        self.btn_slider.entity.left = max_left * self.value
    else
        local max_top = rect.h - (self.btn_slider.entity.height_percent and self.btn_slider.entity.height * 0.01 * rect.h or self.btn_slider.entity.height)
        self.btn_slider.entity.top = max_top * self.value  
    end
    self.btn_slider.entity:UpdatePos()
end