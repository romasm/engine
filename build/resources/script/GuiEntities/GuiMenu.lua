local BG_MAT = "../resources/shaders/gui/rect"
local SHADOW_MAT = "../resources/shaders/gui/shadow"

-- bu
function GuiSubmenuButton(props)
    local button = GuiButton(props)
    button.is_submenu_btn = true
    button.hover_only = true
    return button
end

if not GuiMenu then GuiMenu = class(GuiEntity) end

-- public
function GuiMenu:init(props)
    self.overlay = true
    self.parent = nil

    self.props = props

    self.adapt_horz = true
    self.adapt_vert = true

    self.offset = {
        x = 0, y = 0
    }

    self.background = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
    }
    self.border = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        width = 0
    }
    self.shadow = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        width = 0
    }

    self.submenus = {}
    self.submenu_opened = false

    self.is_hide = true
    self.is_submenu = false
    self.unfocus_stop = false

    self.entity = HEntity()

    self.closed = true
end

function GuiMenu:IsOpened()
    if (not self.entity:is_null() and self.entity:IsExist()) then return true end
    return false
end

function GuiMenu:IsHide()
    return self.is_hide
end

function GuiMenu:Open(x, y)
    local root = self.parent.entity:GetRoot()
    
    if self.is_hide and (not self.entity:is_null() and self.entity:IsExist()) then
        self.is_hide = false
        self.entity.enable = true
    else
        if (not self.entity:is_null() and self.entity:IsExist()) then return end

        self.closed = false
        self.is_hide = false

        --self._base.init(self, self.props)
        --self.entity = HEntity()
        self.entity:Create()
    
        self:ApplyProps(self.props)
        self.entity.focus_mode = GUI_FOCUS_MODE.ONTOP

        self:ApplyCallbacks(self.props)

        local id = ""
        if self.props.id then id = self.props.id end

        self.entity:Init(id, self)
    
        -- child process
        for ch = 1, #self.props do
            if not self.props[ch].overlay then
                self.entity:AttachChild(self.props[ch].entity)
                if self.props[ch].is_submenu_btn then
                    for menu_id, submenu in pairs(self.props[ch].overlays) do
                        submenu.is_submenu = true
                        self.submenus[self.props[ch].entity:GetID()] = submenu
                        break
                    end
                end
            else
                self:AttachOverlay(self.props[ch])
            end
        end
        -----

        if self.shadow.width > 0 then
            self.shadow_rect = self.entity:AddRect(SHADOW_MAT)
            self.shadow_mat = self:SetRectMaterial(self.shadow_rect)
            self.shadow_mat:SetVectorByID(self.shadow.color, 2, 0)
        end
    
        self.rect = self.entity:AddRect(BG_MAT)
        self.rect_mat = self:SetRectMaterial(self.rect)
    
        self.rect_mat:SetVectorByID(self.background.color, 2)
        self.rect_mat:SetVectorByID(self.border.color, 3)

        -- positioning
        root:AttachChild(self.entity)
    
        self.entity:UpdateSize()
    end

    local Grect = root:GetRectAbsolute()
    local rect = self.entity:GetRectAbsolute()

    local shader_data = Vector4(0,0,0,0)
    shader_data.x = self.border.width / rect.w
    shader_data.y = self.border.width / rect.h
    shader_data.z = 1 - shader_data.x
    shader_data.w = 1 - shader_data.y
    self.rect_mat:SetVectorByID(shader_data, 1)

    self.entity.left = x - Grect.l
    if self.adapt_horz and x + rect.w > Grect.w then
        self.entity.left = self.entity.left - rect.w - self.offset.x
    end

    self.entity.top = y - Grect.t
    if self.adapt_vert and y + rect.h > Grect.h then
        self.entity.top = self.entity.top - rect.h - self.offset.y
    end

    self.entity:UpdatePos()

    rect.l = 0
    rect.t = 0
    self.entity:SetRect(self.rect, rect)

    if self.shadow_mat ~= nil then
        rect.l = -self.shadow.width
        rect.t = -self.shadow.width
        local double_shadow_w = self.shadow.width * 2
        rect.w = rect.w + double_shadow_w
        rect.h = rect.h + double_shadow_w
        self.entity:SetRect(self.shadow_rect, rect)

        shader_data.x = self.shadow.width / rect.w
        shader_data.y = self.shadow.width / rect.h
        self.shadow_mat:SetVectorByID(shader_data, 1)
    end

    self.entity:SetHierarchyFocusOnMe(false, true)

    --[[local ev = HEvent()
    ev.entity = self.entity
    ev.event = GUI_EVENTS.MENU_OPEN
    self.parent.entity:CallbackHierarchy()
    --]]
end

function GuiMenu:Close()
    if not self.closed and (not self.entity:is_null() and self.entity:IsExist()) then
        for btn_id, submenu in pairs(self.submenus) do
            submenu:Close()
        end
        self.entity:GetParent():DetachChild(self.entity)
        self.entity:Destroy()
        self.closed = true
        --self.entity = HEntity()
    end
end

function GuiMenu:Hide()
    if (not self.entity:is_null() and self.entity:IsExist()) then
        self.is_hide = true
        self.entity.enable = false
    end
end

function GuiMenu:ApplyProps(props)
    self._base.ApplyProps(self, props)

    if props.adapt_horz ~= nil then self.adapt_horz = props.adapt_horz end
    if props.adapt_vert ~= nil then self.adapt_vert = props.adapt_vert end

    if props.offset ~= nil then 
        if props.offset.x ~= nil then self.offset.x = props.offset.x end 
        if props.offset.y ~= nil then self.offset.y = props.offset.y end 
    end

    if props.background ~= nil then
        if props.background.color ~= nil then 
            self.background.color = type(props.background.color) == 'string' and CoreGui.GetColor(props.background.color) or props.background.color end
    end

    if props.border ~= nil then
        if props.border.color ~= nil then 
            self.border.color = type(props.border.color) == 'string' and CoreGui.GetColor(props.border.color) or props.border.color end
        
        if props.border.width ~= nil then self.border.width = props.border.width end
    end

    if props.shadow ~= nil then
        if props.shadow.color ~= nil then 
            self.shadow.color = type(props.shadow.color) == 'string' and CoreGui.GetColor(props.shadow.color) or props.shadow.color end
        
        if props.shadow.width ~= nil then self.shadow.width = props.shadow.width end
    end
end

function GuiMenu:callback(eventData)
    local res = eventData
    local parent_event = false

    if eventData.event == GUI_EVENTS.UNFOCUS then 
        if self.entity:is_eq(eventData.entity) and not self.submenu_opened then
            res.event = GUI_EVENTS.MENU_CLOSE
            parent_event = true
            if not self.is_submenu then
                res.entity = self.entity
                self:Close()
            end
        end
    elseif eventData.event == GUI_EVENTS.BUTTON_PRESSED then 
        if not eventData.entity:GetInherited().is_submenu_btn then
            res.event = GUI_EVENTS.MENU_CLICK
            parent_event = true

            if not self.is_submenu then
                self.parent.entity:SetHierarchyFocusOnMe(false)
            end
            self:Close()
        end
        
    elseif eventData.event == GUI_EVENTS.CB_CHECKED then 
        res.event = GUI_EVENTS.MENU_CHECK
        parent_event = true

    elseif eventData.event == GUI_EVENTS.CB_UNCHECKED then 
        res.event = GUI_EVENTS.MENU_UNCHECK
        parent_event = true

    elseif eventData.event == GUI_EVENTS.BUTTON_HOVER then 
        if eventData.entity:GetInherited().is_submenu_btn then
            self.submenu_opened = true
            local rect = eventData.entity:GetRectAbsolute()
            self.submenus[eventData.id]:Open(rect.l + rect.w, rect.t)

            res.event = GUI_EVENTS.MENU_SUB_OPEN
            res.entity = self.submenus[eventData.id].entity
            parent_event = true
        end

    elseif eventData.event == GUI_EVENTS.BUTTON_MOVE then 
        for k, submenu in pairs(self.submenus) do
            if k ~= eventData.id then
                if not submenu:IsHide() then 
                    submenu:Hide() 
                    self.submenu_opened = false
                    self.unfocus_stop = true
                    self.entity:SetHierarchyFocusOnMe(false)
                end
            end
        end
        if eventData.entity:GetInherited().is_submenu_btn then
            self.submenu_opened = true
        end
        
    elseif eventData.event == GUI_EVENTS.MENU_CLICK then 
        self:Close()
        parent_event = true

        if not self.is_submenu then
            self.parent.entity:SetHierarchyFocusOnMe(false)
        end

    elseif eventData.event == GUI_EVENTS.MENU_CLOSE then 
        if self.unfocus_stop then
            self.unfocus_stop = false
        else
            res.entity = self.entity
            self:Close()
            res.event = GUI_EVENTS.MENU_CLOSE
            parent_event = true
        end

    elseif eventData.event == GUI_EVENTS.KEY_DOWN and eventData.key == KEYBOARD_CODES.KEY_ESCAPE then 
        self:Close()
        parent_event = true
        res.event = GUI_EVENTS.MENU_CLOSE

        if not self.is_submenu then
            self.parent.entity:SetHierarchyFocusOnMe(false)
        end

    end
    
    res = self._base.callback(self, res)
    if parent_event then 
        self.parent.entity:CallbackHierarchy(res)
        res.event = GUI_EVENTS.NULL
        res.entity = self.entity
    end
    return res
end

function GuiMenu:SetItemState(item_id, active)
    if (self.entity:is_null() or not self.entity:IsExist()) then return false end
    
    local ent = self.entity:GetChildById(item_id)
    if ent:is_null() then return false end
    
    if active then ent:Activate()
    else ent:Deactivate() end
    
    return true
end

function GuiMenu:SendCloseEvent()
    self:Close()

    if not self.is_submenu then
        self.parent.entity:SetHierarchyFocusOnMe(false)
    end
    
    local fake_event = HEvent()
    fake_event.event = GUI_EVENTS.MENU_CLOSE
    fake_event.entity = self.entity
    self.parent.entity:CallbackHierarchy(fake_event)
end