local ICON_ANGEL = math.pi * -0.5

if not GuiGroup then GuiGroup = class(GuiEntity) end

-- public
function GuiGroup:init(props, no_background)
    self.header = {
        height = 0,
        top = 0,
    }

    self.stackable = true
    self.closeable = true
    self.anim_time = 0

    self.background = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_closed = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0)
    }

    self.border = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_closed = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        width = 0
    }

    --replace self._base.init(self, props)
    self.entity = HEntity()
    self.entity:Create()
    
    self:ApplyProps(props)
    self:ApplyCallbacks(props)

    local name_id = ""
    if props.id then name_id = props.id end

    self.entity:Init(name_id, self) 
    ----

    self.entity.height_percent = false

    self.padding = 0
    if self.stackable == true then 
        self.entity.top_percent = false
        self.padding = self.entity.top
    end

    if not no_background then
        local rect_props = {
            background = self.background,
            border = self.border, 
            left = 0,
            top = 0,
            width = 100,
            height = 100,
            width_percent = true,
            height_percent = true,
            ignore_events = true,
            collide_through = true,
            visible = props.visible,
        }
        self.rect = GuiRect(rect_props)
        self.entity:AttachChild(self.rect.entity)
    end

    if self.closeable or self.header.text == nil then
        self.btn_header = GuiButton(self.header)
        self.entity:AttachChild(self.btn_header.entity)
    end

    -- child process
    self.groups = {}
    local prev_end = 0
    local last_padding = 0
    for ch = 1, #props do
        if not props[ch].overlay then
            self.entity:AttachChild(props[ch].entity)
        else
            self:AttachOverlay(props[ch])
        end

        if props[ch]:is_a(GuiGroup) and props[ch].stackable == true then 
            table.insert(self.groups, props[ch])

            last_padding = self.groups[#self.groups].padding
            prev_end = prev_end + last_padding
            self.groups[#self.groups].entity.top = prev_end
            prev_end = prev_end + self.groups[#self.groups].entity.height
        end
    end

    if #self.groups > 0 then 
        self.opened_h = prev_end + last_padding
        self.entity.height = self.opened_h
    else
        self.opened_h = self.entity.height
    end
    ----

    self.closed_h = self.header.height + self.header.top * 2

    self.state_open = true

    self.need_groups_update = false

    self.anim_progress = 1
    self.anim_go = 0

    self:UpdateProps()
end

function GuiGroup:AddGroup(gr)
    if not gr.stackable then return end
    if not gr:is_a(GuiGroup) then return end

    self.entity:AttachChild(gr.entity)

    table.insert(self.groups, gr)
    
    local last_padding = self.groups[#self.groups].padding
    local prev_end = last_padding
    if #self.groups > 1 then prev_end = prev_end + self.groups[#self.groups-1].entity.top + self.groups[#self.groups-1].entity.height end
    self.groups[#self.groups].entity.top = prev_end

    self.opened_h = prev_end + self.groups[#self.groups].entity.height + last_padding
    self.entity.height = self.opened_h
    
    self.entity:UpdateSize()
    if self.window then self.window.entity:UpdateSize() end
end

function GuiGroup:UpdateH(height)
    if #self.groups > 0 then 
        local prev_end = 0
        local last_padding = 0
        for i = 1, #self.groups do
            prev_end = prev_end + self.groups[i].padding
            self.groups[i].entity.top = prev_end
            
            last_padding = self.groups[i].padding
            prev_end = prev_end + self.groups[i].entity.height
        end
        self.opened_h = prev_end + last_padding
    elseif height ~= nil then
        self.opened_h = height
    end

    if self.state_open == true then self.entity.height = self.opened_h end
    if self.stackable then
        local prnt = self.entity:GetParent():GetInherited()
        if prnt:is_a(GuiGroup) then prnt:UpdateH() end
    end
end

function GuiGroup:ClearGroups()
    for i = #self.groups, 1, -1 do
        local gr = self.groups[i]
        self.entity:DetachChild(gr.entity)
        gr.entity:Destroy()
        table.remove(self.groups, i)
    end

    self.opened_h = self.closed_h
    self.entity.height = self.opened_h

    self.entity:UpdateSize()
    if self.window then self.window.entity:UpdateSize() end
end

function GuiGroup:ApplyProps(props)
    self._base.ApplyProps(self, props)

    if props.closeable ~= nil then self.closeable = props.closeable end
    if props.anim_time ~= nil then self.anim_time = props.anim_time end

    if props.header ~= nil then 
        self.header = props.header 
        if self.header.height == nil then self.header.height = 100 end
    end

    if props.background ~= nil then
        if props.background.color ~= nil then 
            self.background.color = type(props.background.color) == 'string' and CoreGui.GetColor(props.background.color) or props.background.color end
        if props.background.color_closed ~= nil then 
            self.background.color_closed = type(props.background.color_closed) == 'string' and CoreGui.GetColor(props.background.color_closed) or props.background.color_closed end
        if props.background.color_nonactive ~= nil then 
            self.background.color_nonactive = type(props.background.color_nonactive) == 'string' and CoreGui.GetColor(props.background.color_nonactive) or props.background.color_nonactive end
    end

    if props.border ~= nil then
        if props.border.color ~= nil then 
            self.border.color = type(props.border.color) == 'string' and CoreGui.GetColor(props.border.color) or props.border.color end
        if props.border.color_closed ~= nil then 
            self.border.color_closed = type(props.border.color_closed) == 'string' and CoreGui.GetColor(props.border.color_closed) or props.border.color_closed end
        if props.border.color_nonactive ~= nil then 
            self.border.color_nonactive = type(props.border.color_nonactive) == 'string' and CoreGui.GetColor(props.border.color_nonactive) or props.border.color_nonactive end
        
        if props.border.width ~= nil then self.border.width = props.border.width end
    end
end

function GuiGroup:UpdateProps()
    if self.btn_header ~= nil and self.btn_header.icon_mat ~= nil then
        self.btn_header.icon_mat:SetVector(Vector4(self.anim_progress * ICON_ANGEL, 0,0,0), 2)
    end
end

function GuiGroup:callback(eventData)
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
        if eventData.entity:is_eq(self.entity) then
            self.entity:SetHierarchyFocusOnMe(false)
            self.entity:SetFocus(HEntity())
        end
        res.entity = self.entity

    elseif eventData.event == GUI_EVENTS.BUTTON_PRESSED and self.closeable == true then 
        if self.btn_header ~= nil and eventData.entity:is_eq(self.btn_header.entity) then
            if self.state_open == true then
                self.state_open = false
                self.anim_go = -1

                res.event = GUI_EVENTS.GROUP_CLOSE
            else
                self.state_open = true
                self.anim_go = 1

                res.event = GUI_EVENTS.GROUP_OPEN
            end
            res.entity = self.entity
        end 
        
    elseif (eventData.event == GUI_EVENTS.GROUP_OPEN or eventData.event == GUI_EVENTS.GROUP_CLOSE) and #self.groups > 0 then
       self.need_groups_update = true

    elseif eventData.event == GUI_EVENTS.UNFOCUS then
        if eventData.entity:is_eq(self.entity) then self.entity:SetFocus(HEntity()) end

    end

    return self._base.callback(self, res)
end

function GuiGroup:onTick(dt)
    local update_size = false

    if self.need_groups_update == true then
        local anim = false

        local prev_end = 0
        local last_padding = 0
        for i = 1, #self.groups do
            prev_end = prev_end + self.groups[i].padding
            self.groups[i].entity.top = prev_end
            
            last_padding = self.groups[i].padding
            prev_end = prev_end + self.groups[i].entity.height 

            self.groups[i].entity:UpdatePos()
            anim = anim or (self.groups[i].anim_go ~= 0) or self.groups[i].need_groups_update
        end
        self.opened_h = prev_end + last_padding
        if self.state_open == true then self.entity.height = self.opened_h end
        update_size = true

        self.need_groups_update = anim
    end

    if self.closeable and self.anim_go ~= 0 then
        self.anim_progress = self.anim_progress + self.anim_go * dt / self.anim_time
        self.anim_progress = math.max(0, math.min(self.anim_progress, 1))

        if self.rect then
            self.rect.background.color = Vector4Lerp(self.background.color_closed, self.background.color, self.anim_progress)
            self.rect.border.color = Vector4Lerp(self.border.color_closed, self.border.color, self.anim_progress)
            self.rect:onActivate()
        end

        if self.btn_header ~= nil and self.btn_header.icon_mat ~= nil then
            self.btn_header.icon_mat:SetVector(Vector4(self.anim_progress * ICON_ANGEL, 0,0,0), 2)
        end

        self.entity.height = self.closed_h + (self.opened_h - self.closed_h) * self.anim_progress
        update_size = true

        if self.anim_progress == 0 or self.anim_progress == 1 then self.anim_go = 0 end
    end

    if update_size == true then
        self.entity:UpdateSize() 
        if self.window then self.window.entity:UpdateSize() end
    end
end

function GuiGroup:SetState(open, anim)
    local fake_ev = HEvent()
    if open then
        if self.state_open == true then return end
        self.state_open = true
        self.anim_go = 1
        if not anim then self.anim_progress = 1 end
        fake_ev.event = GUI_EVENTS.GROUP_OPEN
    else
        if self.state_open == false or self.closeable == false then return end
        self.state_open = false
        self.anim_go = -1
        if not anim then self.anim_progress = 0 end
        fake_ev.event = GUI_EVENTS.GROUP_CLOSE
    end
    self.entity:CallbackHierarchy(fake_ev) -- temp
end