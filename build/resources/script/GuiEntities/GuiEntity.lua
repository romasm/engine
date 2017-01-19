if not GuiEntity then GuiEntity = class() end

function GuiEntity:init(props)
    self.entity = HEntity()
    self.entity:Create()
    
    self:ApplyProps(props)
    self:ApplyCallbacks(props)

    local id = ""
    if props.id then id = props.id end

    self.entity:Init(id, self)
    
    -- child process
    for ch = 1, #props do
        if not props[ch].overlay then
            self.entity:AttachChild(props[ch].entity)
        else
            self:AttachOverlay(props[ch])
        end
    end
end

function GuiEntity:AttachOverlay(obj)
    obj.parent = self
    local overlay_id = obj.props.id
    if overlay_id:len() > 0 then 
        if self.overlays == nil then self.overlays = {} end
        self.overlays[overlay_id] = obj
    end
end

function GuiEntity:ApplyProps(props)
    if props.styles then 
        for st = 1, #props.styles do
            self:ApplyProps(props.styles[st])
        end
    end    

    if props.enable ~= nil then self.entity.enable = props.enable end
    if props.visible ~= nil then self.entity.visible = props.visible end
   
    if props.left ~= nil then self.entity.left = props.left end
    if props.top ~= nil then self.entity.top = props.top end
    if props.right ~= nil then self.entity.right = props.right end
    if props.bottom ~= nil then self.entity.bottom = props.bottom end
    if props.width ~= nil then self.entity.width = props.width end
    if props.height ~= nil then self.entity.height = props.height end

    if props.left_percent ~= nil then self.entity.left_percent = props.left_percent end
    if props.top_percent ~= nil then self.entity.top_percent = props.top_percent end
    if props.right_percent ~= nil then self.entity.right_percent = props.right_percent end
    if props.bottom_percent ~= nil then self.entity.bottom_percent = props.bottom_percent end
    if props.width_percent ~= nil then self.entity.width_percent = props.width_percent end
    if props.height_percent ~= nil then self.entity.height_percent = props.height_percent end

    if props.align ~= nil then self.entity.align = props.align end
    if props.valign ~= nil then self.entity.valign = props.valign end

    if props.focus_mode ~= nil then self.entity.focus_mode = props.focus_mode end

    if props.collide_through ~= nil then self.entity.collide_through = props.collide_through end

    if props.double_click ~= nil then self.entity.double_click = props.double_click end
    if props.ignore_events ~= nil then self.entity.ignore_events = props.ignore_events end
end

function GuiEntity:ApplyCallbacks(props)
    if props.events == nil then return end

    self.events = {}
    for ev, func in pairs(props.events) do
        self.events[ev] = func
    end
end

function GuiEntity:callback(eventData)
    local res = eventData
    
    if self.events ~= nil and self.events[res.event] ~= nil then
        if self.events[res.event](self, res) == true then -- throw event
            res.event = GUI_EVENTS.NULL
        end
    end

    return res
end

function GuiEntity:onMoveResize(is_move, is_resize)
    if self.events == nil then return true end          -- todo: nil the onMoveResize if unused
    if not (is_move or is_resize) then return true end

    local drop_resize = false
    local drop_move = false

    if is_resize and self.events[GUI_EVENTS.SYS_RESIZE] then 
        drop_resize = self.events[GUI_EVENTS.SYS_RESIZE](self) 
    end
    if is_move and self.events[GUI_EVENTS.SYS_MOVE] then 
        drop_move = elf.events[GUI_EVENTS.SYS_MOVE](self) 
    end
    return not( (drop_move or (not is_move)) and (drop_resize or (not is_resize)) )
end

function GuiEntity:SetRectMaterial(rect_id, mat_desc)
    local mat = self.entity:GetRectShaderInst(rect_id)
    if mat == nil then
        error("Cant get shader instance for rect " .. rect_id) end

    if mat_desc == nil then return mat end
    
    if mat_desc.textures then
        for slot, tex in pairs(mat_desc.textures) do
            mat:SetTextureName(tex, slot)
        end
    end
    
    if mat_desc.vectors then
        for slot, vect in pairs(mat_desc.vectors) do
            mat:SetVector(vect, slot)
        end
    end
    
    if mat_desc.floats then
        for slot, f in pairs(mat_desc.floats) do
            mat:SetFloat(f, slot)
        end
    end
    
    return mat
end

-- dumb entity
if not GuiDumb then GuiDumb = class(GuiEntity) end

function GuiDumb:callback(eventData)
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

    elseif eventData.event == GUI_EVENTS.UNFOCUS then
        if eventData.entity:is_eq(self.entity) then self.entity:SetFocus(HEntity()) end

    end
    
    return self._base.callback(self, res)
end