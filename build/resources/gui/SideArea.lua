SIDE_WIN_MIN_HEIGHT = 100

function Gui.SideArea(is_left)

local separator_id = is_left and 2 or 1

local sidearea = GuiEntity({
    ignore_events = false,
    collide_through = false,
    align = is_left and GUI_ALIGN.LEFT or GUI_ALIGN.RIGHT,
    right = 0,
    left = 0,
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
        left = is_left and 0 or 4,
        right = is_left and 4 or 0,
        height = 4,

        id = 'resize_btn',
        
        top = Tools.side_area_separator[separator_id],
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
                    local separator_id = parent:GetInherited().separator_id

                    Tools.side_area_separator[separator_id] = ev.coords.y - self.mouse_offset - parentRect.t
                    Tools.side_area_separator[separator_id] = math.min(math.max(SIDE_WIN_MIN_HEIGHT, Tools.side_area_separator[separator_id]), parentRect.h - SIDE_WIN_MIN_HEIGHT)
                    if self.entity.top ~= Tools.side_area_separator[separator_id] then
                        parent:UpdateSize()
                    end
                end
                return true
            end,
        },
    })
})

sidearea.separator_id = separator_id
sidearea.resize_btn = sidearea.entity:GetChildById('resize_btn')

sidearea.onMoveResize = function(self, is_move, is_resize)
        local rect = self.entity:GetRectAbsolute()

        Tools.side_area_separator[self.separator_id] = math.min(math.max(SIDE_WIN_MIN_HEIGHT, Tools.side_area_separator[self.separator_id]), rect.h - SIDE_WIN_MIN_HEIGHT)
        self.resize_btn.top = Tools.side_area_separator[self.separator_id]

        if self.first_win then self.first_win.bottom = rect.h - Tools.side_area_separator[self.separator_id] end
        if self.second_win then self.second_win.top = Tools.side_area_separator[self.separator_id] + self.second_win.bottom end

        return true
    end

return sidearea
end