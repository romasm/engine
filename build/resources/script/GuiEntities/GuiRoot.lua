if not GuiRoot then GuiRoot = class() end
--[[
function GuiRoot.reload()          
    
    if root.alt then
        root.entity:DetachChild(root.alt.entity)
        root.alt.entity:Destroy()
        root.alt = nil
    end
    root.alt = Gui.AltString()
    root.entity:AttachChild(root.alt.entity)
    root.alt.entity.enable = false
    
    root.altstring = root.alt.entity:GetChildById('altstring'):GetInherited()
end
--]]
loader.require("Pieces.Alt")--, GuiRoot.reload)

-- public
function GuiRoot:init(ent)
    self.entity = ent
    CoreGui.SetRootClass(ent, self)

    self.sys_window = CoreGui.SysWindows.GetByRoot(ent)

    self.alt = Gui.AltString()
    self.entity:AttachChild(self.alt.entity)
    self.alt.entity.enable = false

    self.altstring = self.alt.entity:GetChildById('altstring'):GetInherited()

    self.alt_vis = 0
    self.alt_time = -1
    self.alt_hide_time = -1

    self.alt_anim_progress = 0
    self.alt_anim_go = false

    self.just_resize = false
    
    self.captionH = self.sys_window:GetCaptionH()
end

function GuiRoot:callback(eventData)
    if eventData.event == GUI_EVENTS.KEY_DOWN then 
        eventData = Hotkeys:Process(eventData, self) 
    end
    return eventData
end

function GuiRoot:onMoveResize(is_move, is_resize)
    if self.just_resize then
        self.just_resize = false
        CoreGui.UpdateRootRect(self.entity)
        return true 
    end
    
    if self.sys_window:IsMaximized() then
        self.entity.left = SYSTEM_BORDER_SIZE
        self.entity.top = SYSTEM_BORDER_SIZE
        self.entity.bottom = SYSTEM_BORDER_SIZE
        self.entity.right = SYSTEM_BORDER_SIZE

        self.sys_window:SetCaptionH(self.captionH + SYSTEM_BORDER_SIZE)
        
        self.just_resize = true
        self.entity:UpdatePosSize()
    else
        self.entity.left = 0
        self.entity.top = 0
        self.entity.bottom = 0
        self.entity.right = 0

        self.sys_window:SetCaptionH(self.captionH)
        
        self.just_resize = true
        self.entity:UpdatePosSize()
    end
    return false
end

function GuiRoot:onTick(dt)
    if self.alt_hide_time >= 0 then 
        self.alt_hide_time = self.alt_hide_time + dt
        if self.alt_hide_time >= ALT_SETS.TIME_STOP then self.alt_hide_time = -1 end
    end

    if self.alt_vis == 0 then return end
    
    if self.alt_time >= 0 then
        self.alt_time = self.alt_time + dt
        if self.alt_time >= ALT_SETS.TIME_SHOW then
            self.alt.entity.enable = true
            --self.entity:SetOnTop(self.alt.entity) -- TODO

            self.alt_anim_go = true
            self.alt_time = -1
        else
            return
        end
    end

    local winrect = self.entity:GetRectAbsolute()
    local pos = CoreGui.GetCursorPos()

    local alt_left = pos.x + ALT_SETS.X
    local alt_top = pos.y + ALT_SETS.Y
    local alt_width = self.alt.entity.width
    local alt_height = self.alt.entity.height

    alt_left = alt_left - math.max(0, alt_left + alt_width - (winrect.w-1))
    if alt_top + alt_height > winrect.h-1 then
        alt_top = alt_top - alt_height - ALT_SETS.Y
    end

    self.alt.entity.left = alt_left
    self.alt.entity.top = alt_top
    self.alt.entity:UpdatePos()

    if self.alt_anim_go then
        self.alt_anim_progress = self.alt_anim_progress + dt / ALT_SETS.TIME_FADEIN
        self.alt_anim_progress = math.min(self.alt_anim_progress, 1)

        local color_rect = Vector4(self.alt.background.color.x, self.alt.background.color.y, 
            self.alt.background.color.z, self.alt.background.color.w)
        local color_rect_border = Vector4(self.alt.border.color.x, self.alt.border.color.y, 
            self.alt.border.color.z, self.alt.border.color.w)
        local color_str = Vector4(self.altstring.color.x, self.altstring.color.y, 
            self.altstring.color.z, self.altstring.color.w)

        color_rect.w = color_rect.w * self.alt_anim_progress
        color_rect_border.w = color_rect_border.w * self.alt_anim_progress
        color_str.w = color_str.w * self.alt_anim_progress

        self.alt.rect_mat:SetVectorByID(color_rect, 2)
        self.alt.rect_mat:SetVectorByID(color_rect_border, 3)
        self.altstring.entity:SetTextColor(self.altstring.text, color_str)

        if self.alt_anim_progress == 1 then 
            self.alt_anim_go = false 
        end
    end
end

function GuiRoot:ShowAlt(str)
    if str:len() == 0 then return end

    self.altstring:SetString(str)
    local rect = self.altstring.entity:GetRectAbsolute()

    self.alt.entity.width = rect.w
    self.alt.entity.height = rect.h
    self.alt.entity:UpdateSize()

    self.alt_vis = self.alt_vis + 1

    if self.alt_hide_time >= 0 then
        self.alt.entity.enable = true
        --self.entity:SetOnTop(self.alt.entity) -- TODO

        self.alt_anim_go = true
        self.alt_time = -1
        self.alt_hide_time = -1
    else
        if self.alt_vis == 1 then self.alt_time = 0 end
    end
end

function GuiRoot:HideAlt()
    if self.alt_vis == 0 then return end

    if self.alt_time == -1 then self.alt_hide_time = 0 end

    self.alt_vis = self.alt_vis - 1
    if self.alt_vis == 0 then 
        self.alt.entity.enable = false
        self.alt_anim_go = false
        self.alt_anim_progress = 0
    end
end