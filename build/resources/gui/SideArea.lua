function Gui.SideArea()
local sidearea = GuiEntity({
    ignore_events = false,
    collide_through = false,
    align = GUI_ALIGN.RIGHT,
    right = 1,
    width = 300,
    valign = GUI_VALIGN.BOTH,
    top = 0,
    bottom = 1,

    events = {
        [GUI_EVENTS.UNFOCUS] = function(self, ev)
            if ev.entity:is_eq(self.entity) then self.entity:SetFocus(HEntity()) end
            return false
        end,
    },
    
    GuiButton({
        styles = {GuiStyles.empty_button,},
        background = {
            color = 'null',
            color_hover = 'act_01',
            color_press = 'act_00',
        },
        cursor = SYSTEM_CURSORS.VERTARROW,
        align = GUI_ALIGN.BOTH,
        left = 4,
        right = 4,
        height = 4,
        
        top = Tools.side_area_separator,
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(true)
                local rect = self.entity:GetRectAbsolute()
                self.mouse_offset = ev.coords.y - rect.t
                return true
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                self.mouse_offset = nil
                return true
            end,
            [GUI_EVENTS.BUTTON_MOVE] = function(self, ev) 
                if self.mouse_offset then
                    local parent = self.entity:GetParent()
                    local parentRect = parent:GetRectAbsolute()
                    Tools.side_area_separator = ev.coords.y - self.mouse_offset - parentRect.t
                    Tools.side_area_separator = math.min(math.max(100, Tools.side_area_separator), parentRect.h - 100)
                    if self.entity.top ~= Tools.side_area_separator then 
                        self.entity.top = Tools.side_area_separator
                        parent:UpdateSize()
                    end
                end
                return true
            end,
        },
    })
})

sidearea.onMoveResize = function(self, is_move, is_resize)
        local rect = self.entity:GetRectAbsolute()
        if self.props_win then self.props_win.bottom = rect.h - Tools.side_area_separator end
        if self.mats_win then self.mats_win.top = Tools.side_area_separator + self.mats_win.bottom end
        return true
    end

return sidearea
end