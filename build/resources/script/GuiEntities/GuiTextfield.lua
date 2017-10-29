local BG_MAT = "../resources/shaders/gui/rect"
local CURSOR_MAT = "../resources/shaders/gui/color"
local SELECT_MAT = "../resources/shaders/gui/color"
local CURSOR_VERT_OFFSET = 1
local CURSOR_WIDTH = 1

loader.require("Menus.Textfield")

if not GuiTextfield then GuiTextfield = class(GuiEntity) end

-- private
local function UpdateCursorSelect(self)
    local rect = self.entity:GetRectAbsolute()
    local text_pos_x = self.text.offset.x + self.border.width - self.scroll
    
    if self.select_pos ~= self.cursor_pos and self.state_live then
        local select_w = math.abs(self.cursor_pos - self.select_pos)
        local select_l = math.min(self.cursor_pos, self.select_pos)
        local r = Rect( text_pos_x + select_l, self.border.width + CURSOR_VERT_OFFSET, 
                        select_w, rect.h - 2 * (self.border.width + CURSOR_VERT_OFFSET) )
        self.entity:SetRect(self.select_r, r)
    else
        self.entity:SetRect(self.select_r, Rect(0,0,0,0))
    end
    if self.state_live then
        local r = Rect( text_pos_x + self.cursor_pos, self.border.width + CURSOR_VERT_OFFSET, 
                  CURSOR_WIDTH, rect.h - 2 * (self.border.width + CURSOR_VERT_OFFSET) )
        self.entity:SetRect(self.cursor, r)
    else
        self.entity:SetRect(self.cursor, Rect(0,0,0,0))
    end
end

local function SelectAll(self)
    local textObj = self.entity:GetText(self.str)
    self.select_letter = 0
    self.select_pos = textObj:GetLetterPos(self.select_letter)
    self.current_letter = textObj.letter_count
    self.cursor_pos = textObj:GetLetterPos(self.current_letter)
    self.state_select = true
end

local function UpdateTextPos(self)
    local rect = self.entity:GetRectAbsolute()
    local text_bounds = self.entity:GetTextBounds(self.str)
    if self.text.center.y then self.text.offset.y = (rect.h - text_bounds.h) / 2 end
    if self.text.center.x then self.text.offset.x = (rect.w - text_bounds.w) / 2 end
    if not self.state_live then
        if self.show_tail and not self.text.center.x then
            self.scroll = math.max(0, text_bounds.w - (rect.w - 2 * (self.border.width + self.text.offset.x)))
        else self.scroll = 0 end
    end
    
    self.entity:SetTextPos(self.str, self.text.offset.x + self.border.width - self.scroll, self.text.offset.y + self.border.width)
end

local function UpdateScroll(self)
    self.prev_letter = self.current_letter
     if self.text.center.x then
        self.scroll = 0
        return
    end

    local rect = self.entity:GetRectAbsolute()
    local textObj = self.entity:GetText(self.str)

    local max_scroll = math.max(0, textObj.line_w - (rect.w - 2 * (self.border.width + self.text.offset.x)))
    local text_area = rect.w - 2 * (self.border.width + self.text.offset.x)

    if self.cursor_pos < self.scroll then
        self.scroll = self.cursor_pos
    elseif self.cursor_pos > self.scroll + text_area then
        self.scroll = self.cursor_pos - text_area
    end

    self.scroll = math.max(0, math.min(max_scroll, self.scroll))
end

local function DeleteSelection(self, update, check_point)
    if (self.select_letter == 0 or self.current_letter == 0) and self.data.d_type ~= GUI_TEXTFIELD.TEXT then
        self.has_minus = false
    end

    -- +1 for lua
    local begin = math.min(self.select_letter, self.current_letter) + 1
    local finish = math.max(self.select_letter, self.current_letter) + 1

    if self.has_point then
        local point_pos = self.text.str:find('%.')
        
        if point_pos == nil or point_pos < begin then
            if check_point then return false end
        else
            if point_pos <= finish then self.has_point = false
            else
                if check_point then return false end
            end
        end
    end

    self.text.str = self.text.str:sub(1, begin - 1) .. self.text.str:sub(finish)
    self.entity:SetText(self.str, self.text.str)

    self.current_letter = begin - 1
    self.select_letter = self.current_letter

    local textObj = self.entity:GetText(self.str)
    self.cursor_pos = textObj:GetLetterPos(self.current_letter)
    self.select_pos = self.cursor_pos

    if update then
        UpdateScroll(self)
        UpdateTextPos(self)
        UpdateCursorSelect(self)
    end

    self.state_select = false

    return true
end

local function InsertSymbol(self, symbol, s_len)
    if self.text.str:len() + s_len > self.text.length then return end
                
    self.text.str = self.text.str:sub(1, self.current_letter) .. symbol .. self.text.str:sub(self.current_letter + 1)
    self.entity:SetText(self.str, self.text.str)

    self.current_letter = self.current_letter + s_len

    local textObj = self.entity:GetText(self.str)
    self.cursor_pos = textObj:GetLetterPos(self.current_letter)
    self.select_letter = self.current_letter
    self.select_pos = self.cursor_pos

    UpdateScroll(self)
    UpdateTextPos(self)
    UpdateCursorSelect(self)
end

local function EditString(self, key, symbol)
    local char = string.byte(symbol)

    if key == KEYBOARD_CODES.KEY_BACK then
        if self.state_select then 
            DeleteSelection(self, true)
            return true 
        end
        if self.current_letter <= 0 then return true end

        local textObj = self.entity:GetText(self.str)
        if self.current_letter == 1 then self.has_minus = false end
        -- (self.current_letter-1) + 1(for lua) = self.current_letter
        if string.byte(self.text.str, self.current_letter) == 46 then self.has_point = false end -- str == '.'
        
        self.text.str = self.text.str:sub(1, self.current_letter - 1) .. self.text.str:sub(self.current_letter + 1)

        self.current_letter = self.current_letter - 1

        self.entity:SetText(self.str, self.text.str)

        self.cursor_pos = textObj:GetLetterPos(self.current_letter)
        self.select_letter = self.current_letter
        self.select_pos = self.cursor_pos

        UpdateScroll(self)
        UpdateTextPos(self)
        UpdateCursorSelect(self)
        return true

    elseif key == KEYBOARD_CODES.KEY_DELETE then
        if self.state_select then 
            DeleteSelection(self, true)
            return true 
        end

        local textObj = self.entity:GetText(self.str)
        if self.current_letter >= textObj.letter_count then return true end
        
        if self.current_letter == 0 then self.has_minus = false end
        -- self.current_letter + 1(for lua)
        if string.byte(self.text.str, self.current_letter + 1) == 46 then self.has_point = false end -- str == '.'
        
        self.text.str = self.text.str:sub(1, self.current_letter) .. self.text.str:sub(self.current_letter + 2)

        self.entity:SetText(self.str, self.text.str)

        UpdateScroll(self)
        UpdateTextPos(self)
        UpdateCursorSelect(self)
        return true

    elseif self.data.d_type == GUI_TEXTFIELD.TEXT then
        if char >= 32 and char <= 255 and char ~= 127 then -- char == all non-special symbols
            if self.state_select then
                DeleteSelection(self)
            end

            InsertSymbol(self, symbol, 1)
            return true
        end
    elseif self.data.d_type == GUI_TEXTFIELD.INT then
        if char >= 48 and char <= 57 or char == 45 then -- char == numbers and '-'
            if self.state_select then
                DeleteSelection(self)
            end
            
            if char == 45 then
                if self.current_letter ~= 0 or self.has_minus then return true end
                self.has_minus = true
            elseif self.has_minus and self.current_letter == 0 then
                return true 
            end

            InsertSymbol(self, symbol, 1)
            return true
        end

    elseif self.data.d_type == GUI_TEXTFIELD.FLOAT then
        if char >= 48 and char <= 57 or char == 45 or char == 46 then -- char == numbers and '-'
            if self.state_select then
                if not DeleteSelection(self, false, char == 46) then return true end -- char == '.'
            end
            
            if char == 45 then
                if self.current_letter ~= 0 or self.has_minus then return true end
                self.has_minus = true
            elseif self.has_minus and self.current_letter == 0 then
                return true
            end

            if char == 46 then
                if self.has_point then return true end
                self.has_point = true
            end
            
            InsertSymbol(self, symbol, 1)
            return true
        end

    end
    return false
end

local function Copy(self)
    local begin = math.min(self.select_letter, self.current_letter) + 1
    local finish = math.max(self.select_letter, self.current_letter) + 1

    CoreGui.Clipboard.PutString(self.text.str:sub(begin, finish - 1))
end

local function Paste(self)
    local str = CoreGui.Clipboard.TakeString()
    if str:len() == 0 then return end
    InsertSymbol(self, str, str:len())
end

-- public
function GuiTextfield:init(props)
    self.selectable = true
    self.color_selection = Vector4(0.0, 0.0, 0.0, 1.0)
    self.color_cursor = Vector4(0.0, 0.0, 0.0, 1.0)
    self.show_tail = false
        
    self.allow_none = false
    self.dbclick_activation = false

    self.hold_focus_onenter = false

    self.background = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_live = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0)
    }

    self.border = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_live = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        width = 0
    }

    self.text = {
        color = Vector4(0.0, 0.0, 0.0, 1.0),
        color_live = Vector4(0.0, 0.0, 0.0, 1.0),
        color_nonactive = Vector4(0.0, 0.0, 0.0, 1.0),
        offset = { x = 0, y = 0 },
        center = { x = false, y = true },
        str = "",
        length = 32,
        font = ""
    }
    
    self.data = {
        d_type = GUI_TEXTFIELD.TEXT,
        min = 0,
        max = 0,
        decimal = 0
    }

    self:base(GuiTextfield).init(self, props)
    
    self.bgrect = self.entity:AddRect(BG_MAT)
    self.bgrect_mat = self:SetRectMaterial(self.bgrect)
    local shader_data = Vector4(0, 0, 1, 1)
    self.bgrect_mat:SetVectorByID(shader_data, 1)

    self.border_rect = GuiRect({
        ignore_events = true,
        collide_through = true,
        width = 100,
        height = 100,
        width_percent = true,
        height_percent = true,
        background = {
            color = Vector4(0,0,0,0),
            color_nonactive = Vector4(0,0,0,0),
        },
        border = {
            color = self.border.color,
            color_nonactive = self.border.color_nonactive,
            width = self.border.width,
        },
    })
    self.entity:AttachChild(self.border_rect.entity)
    
    self.select_r = self.entity:AddRect(SELECT_MAT)
    self.select_mat = self:SetRectMaterial(self.select_r)
    self.select_mat:SetVectorByID(self.color_selection, 1)

    self.cursor = self.entity:AddRect(CURSOR_MAT)
    self.cursor_mat = self:SetRectMaterial(self.cursor)
    self.cursor_mat:SetVectorByID(self.color_cursor, 1)
    
    if self.text.font:len() == 0 then
        error("Font is undefined for GuiTextfield named "..self.entity:GetID())
        return
    end
    
    self.str = self.entity:AddText(self.text.font, self.text.str, "", false, self.text.length)
    
    self.oldstr = self.text.str
    self.no_change = true

    self.state_live = false
    self.state_select = false
    self.state_mouseover = false

    self.selecting = false
        
    self.cursor_pos = 0
    self.select_pos = 0
    self.scroll = 0

    self.select_letter = 0 -- C++ indexing
    self.current_letter = 0 -- C++ indexing
    self.prev_letter = 0 -- C++ indexing

    self.has_minus = false
    self.has_point = false

    self:UpdateProps()
end

function GuiTextfield:ApplyDisable()
    self.bgrect_mat:SetVectorByID(self.background.color_nonactive, 2)
    
    self.entity:SetRect(self.cursor, Rect(0,0,0,0))
    self.entity:SetRect(self.select_r, Rect(0,0,0,0))

    self.entity:SetTextColor(self.str, self.text.color_nonactive)
end

function GuiTextfield:ApplyNone()
    self.bgrect_mat:SetVectorByID(self.background.color, 2)
    self.border_rect.border.color = self.border.color
    self.border_rect:UpdateProps()
    
    self.entity:SetRect(self.cursor, Rect(0,0,0,0))
    self.entity:SetRect(self.select_r, Rect(0,0,0,0))
    
    self.entity:SetTextColor(self.str, self.text.color)
end

function GuiTextfield:ApplyLive()
    self.bgrect_mat:SetVectorByID(self.background.color_live, 2)
    self.border_rect.border.color = self.border.color_live
    self.border_rect:UpdateProps()
    
    self.entity:SetTextColor(self.str, self.text.color_live)
end

function GuiTextfield:ApplyProps(props)
    self._base.ApplyProps(self, props)

    self.entity.double_click = true

    if props.selectable ~= nil then self.selectable = props.selectable end
    if props.color_selection ~= nil then self.color_selection = type(props.color_selection) == 'string' and CoreGui.GetColor(props.color_selection) or props.color_selection end
    if props.color_cursor ~= nil then self.color_cursor = type(props.color_cursor) == 'string' and CoreGui.GetColor(props.color_cursor) or props.color_cursor end
    if props.show_tail ~= nil then self.show_tail = props.show_tail end
    if props.allow_none ~= nil then self.allow_none = props.allow_none end
    if props.dbclick_activation ~= nil then self.dbclick_activation = props.dbclick_activation end
    if props.hold_focus_onenter ~= nil then self.hold_focus_onenter = props.hold_focus_onenter end

    if props.background ~= nil then
        if props.background.color ~= nil then 
            self.background.color = type(props.background.color) == 'string' and CoreGui.GetColor(props.background.color) or props.background.color end
        if props.background.color_live ~= nil then 
            self.background.color_live = type(props.background.color_live) == 'string' and CoreGui.GetColor(props.background.color_live) or props.background.color_live end
        if props.background.color_nonactive ~= nil then 
            self.background.color_nonactive = type(props.background.color_nonactive) == 'string' and CoreGui.GetColor(props.background.color_nonactive) or props.background.color_nonactive end
    end

    if props.border ~= nil then
        if props.border.color ~= nil then 
            self.border.color = type(props.border.color) == 'string' and CoreGui.GetColor(props.border.color) or props.border.color end
        if props.border.color_live ~= nil then 
            self.border.color_live = type(props.border.color_live) == 'string' and CoreGui.GetColor(props.border.color_live) or props.border.color_live end
        if props.border.color_nonactive ~= nil then 
            self.border.color_nonactive = type(props.border.color_nonactive) == 'string' and CoreGui.GetColor(props.border.color_nonactive) or props.border.color_nonactive end
        
        if props.border.width ~= nil then self.border.width = props.border.width end
    end

    if props.text ~= nil then
        if props.text.str ~= nil then self.text.str = props.text.str end
        if props.text.font ~= nil then self.text.font = props.text.font end
        if props.text.length ~= nil then self.text.length = props.text.length end
        if props.text.center ~= nil then 
            if props.text.center.x ~= nil then self.text.center.x = props.text.center.x end
            if props.text.center.y ~= nil then self.text.center.y = props.text.center.y end
        end
        if props.text.offset ~= nil then
            if props.text.offset.x ~= nil then self.text.offset.x = props.text.offset.x end
            if props.text.offset.y ~= nil then self.text.offset.y = props.text.offset.y end
        end

        if props.text.color ~= nil then 
            self.text.color = type(props.text.color) == 'string' and CoreGui.GetColor(props.text.color) or props.text.color end
        if props.text.color_live ~= nil then 
            self.text.color_live = type(props.text.color_live) == 'string' and CoreGui.GetColor(props.text.color_live) or props.text.color_live end
        if props.text.color_nonactive ~= nil then 
            self.text.color_nonactive = type(props.text.color_nonactive) == 'string' and CoreGui.GetColor(props.text.color_nonactive) or props.text.color_nonactive end
    end

    if props.data ~= nil then
        if props.data.d_type ~= nil then self.data.d_type = props.data.d_type end
        if props.data.decimal ~= nil then self.data.decimal = props.data.decimal end
        if props.data.max ~= nil then self.data.max = props.data.max end
        if props.data.min ~= nil then self.data.min = props.data.min end
    end
end

function GuiTextfield:UpdateProps()
    if self.entity:IsActivated() and self.entity:IsActivatedBranch() then self:onActivate()
    else self:onDeactivate() end
end

function GuiTextfield:onMoveResize(is_move, is_resize)
    if not self._base.onMoveResize(self, is_move, is_resize) then return false end

    local rect = self.entity:GetRectAbsolute()
    
    if self.bgrect_mat ~= nil then
        local r = Rect(0, 0, rect.w, rect.h)
        self.entity:SetRect(self.bgrect, r)
    end
    
    if self.str ~= nil then
        UpdateTextPos(self)        
        UpdateCursorSelect(self)
    end

    return true
end

function GuiTextfield:callback(eventData)
    local res = eventData
    
    if not self.entity:IsActivated() or not self.entity:IsActivatedBranch() then
        if eventData.event == GUI_EVENTS.MOUSE_DOWN or eventData.event == GUI_EVENTS.MOUSE_DBLCLICK then
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

        if not self.state_live and self.dbclick_activation then 
            if eventData.key == KEYBOARD_CODES.KEY_LBUTTON then
                res.event = GUI_EVENTS.TF_ACTIVATION_WAIT
                res.entity = self.entity
            end
            return self._base.callback(self, res)
        end

        if eventData.key == KEYBOARD_CODES.KEY_LBUTTON then
            local shift_selected = false
            local rect = self.entity:GetRectAbsolute()
            local textObj = self.entity:GetText(self.str)

            if not self.state_live then
                self:ApplyLive()
                self.state_live = true
                self.oldstr = self.text.str
                res.event = GUI_EVENTS.TF_ACTIVATE
            else
                if CoreGui.Keys.Shift() and self.selectable then
                    local relativeX = eventData.coords.x - rect.l - self.text.offset.x - self.border.width + self.scroll
                    relativeX = math.max(0, relativeX)
                    self.current_letter = textObj:GetClosestLetter(relativeX)

                    if self.select_letter ~= self.current_letter then
                        self.cursor_pos = textObj:GetLetterPos(self.current_letter)
                        shift_selected = true
                        self.state_select = true
                        res.event = GUI_EVENTS.TF_SELECTED
                    end
                end
            end

            if not shift_selected then
                local relativeX = eventData.coords.x - rect.l - self.text.offset.x - self.border.width + self.scroll
                relativeX = math.max(0, relativeX)
                self.current_letter = textObj:GetClosestLetter(relativeX)
                self.cursor_pos = textObj:GetLetterPos(self.current_letter)
                self.select_letter = self.current_letter
                self.select_pos = self.cursor_pos

                if self.selectable then
                    self.entity:SetHierarchyFocusOnMe(true)
                    self.state_select = false
                    self.selecting = true
                end
                if res.event == GUI_EVENTS.MOUSE_DOWN then
                    res.event = GUI_EVENTS.TF_CURSORMOVE
                end
            end

            res.entity = self.entity
            UpdateCursorSelect(self)
            UpdateTextPos(self)

        elseif eventData.key == KEYBOARD_CODES.KEY_RBUTTON then
            if not self.state_live then
                self:ApplyLive()
                self.state_live = true
                self.oldstr = self.text.str
                UpdateTextPos(self)
            end

            if not self.state_select then
                local textObj = self.entity:GetText(self.str)
                local rect = self.entity:GetRectAbsolute()
                local relativeX = eventData.coords.x - rect.l - self.text.offset.x - self.border.width + self.scroll
                relativeX = math.max(0, relativeX)
                self.current_letter = textObj:GetClosestLetter(relativeX)
                self.cursor_pos = textObj:GetLetterPos(self.current_letter)
                self.select_letter = self.current_letter
                self.select_pos = self.cursor_pos
                UpdateCursorSelect(self)
            end

            if self.selectable then
                self.selecting = false
            end

            self.menu = Gui.TextfieldMenu()
            self:AttachOverlay(self.menu)
            self.menu:Open(eventData.coords.x, eventData.coords.y)
            res.event = GUI_EVENTS.NULL
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_DBLCLICK then 
        if not self.state_live and self.dbclick_activation then
            self:ApplyLive()
            self.state_live = true
            self.oldstr = self.text.str
            res.event = GUI_EVENTS.TF_ACTIVATE
        end

        SelectAll(self)
        UpdateScroll(self)
        UpdateCursorSelect(self)
        UpdateTextPos(self)

        if res.event == GUI_EVENTS.MOUSE_DBLCLICK then
            res.event = GUI_EVENTS.TF_SELECTED
        end
        res.entity = self.entity

    elseif eventData.event == GUI_EVENTS.UNFOCUS then 
        self:ApplyNone()
        self.state_live = false
        self.state_select = false
        self.selecting = false
        
        if self.data.d_type == GUI_TEXTFIELD.INT or self.data.d_type == GUI_TEXTFIELD.FLOAT then
            local res = tonumber(self.text.str)
            if res ~= nil then self:SetNum(res)
            else 
                if self.allow_none then self:SetText("")
                else self:SetNum(0) end
            end 
        end

        if self.oldstr == self.text.str then self.no_change = true
        else self.no_change = false end

        UpdateTextPos(self)
        res.event = GUI_EVENTS.TF_DEACTIVATE
        res.entity = self.entity

    elseif eventData.event == GUI_EVENTS.KEY_DOWN then 
        local rect = self.entity:GetRectAbsolute()
        local textObj = self.entity:GetText(self.str)
       
        if eventData.key == KEYBOARD_CODES.KEY_LEFT then
            if self.current_letter > 0 then
                if CoreGui.Keys.Shift() then
                    if not self.state_select then
                        self.state_select = true
                        self.select_letter = self.current_letter
                        self.select_pos = self.cursor_pos
                    end
                    self.current_letter = self.current_letter - 1
                    self.cursor_pos = textObj:GetLetterPos(self.current_letter)
                    res.event = GUI_EVENTS.TF_SELECTED
                else
                    self.state_select = false
                    self.current_letter = self.current_letter - 1
                    self.cursor_pos = textObj:GetLetterPos(self.current_letter)
                    self.select_letter = self.current_letter
                    self.select_pos = self.cursor_pos
                    res.event = GUI_EVENTS.TF_CURSORMOVE
                end
            else
                self.state_select = false
                self.select_letter = self.current_letter
                self.select_pos = self.cursor_pos
                res.event = GUI_EVENTS.TF_CURSORMOVE
            end
            
            UpdateScroll(self)
            UpdateCursorSelect(self)
            UpdateTextPos(self)

        elseif eventData.key == KEYBOARD_CODES.KEY_RIGHT then
            if self.current_letter < textObj.letter_count then
                if CoreGui.Keys.Shift() then
                    if not self.state_select then
                        self.state_select = true
                        self.select_letter = self.current_letter
                        self.select_pos = self.cursor_pos
                    end
                    self.current_letter = self.current_letter + 1
                    self.cursor_pos = textObj:GetLetterPos(self.current_letter)
                    res.event = GUI_EVENTS.TF_SELECTED
                else
                    self.state_select = false
                    self.current_letter = self.current_letter + 1
                    self.cursor_pos = textObj:GetLetterPos(self.current_letter)
                    self.select_letter = self.current_letter
                    self.select_pos = self.cursor_pos
                    res.event = GUI_EVENTS.TF_CURSORMOVE
                end
            else
                self.state_select = false
                self.select_letter = self.current_letter
                self.select_pos = self.cursor_pos
                res.event = GUI_EVENTS.TF_CURSORMOVE
            end
            
            UpdateScroll(self)
            UpdateCursorSelect(self)
            UpdateTextPos(self)

        elseif eventData.key == KEYBOARD_CODES.KEY_ESCAPE then
            self.entity:SetHierarchyFocusOnMe(false)
            local prt = self.entity:GetParent()
            prt:SetFocus(HEntity(), false)
            res.event = GUI_EVENTS.NULL
            
        elseif eventData.key == KEYBOARD_CODES.KEY_RETURN then
            if self.hold_focus_onenter then
                res.event = GUI_EVENTS.TF_DEACTIVATE
            else
                self.entity:SetHierarchyFocusOnMe(false)
                local prt = self.entity:GetParent()
                prt:SetFocus(HEntity(), false)
                res.event = GUI_EVENTS.NULL
            end

        else
            if CoreGui.Keys.Ctrl() then
                if eventData.key == KEYBOARD_CODES.KEY_C then
                    Copy(self)
                elseif eventData.key == KEYBOARD_CODES.KEY_X then
                    Copy(self)
                    DeleteSelection(self, true)
                    res.event = GUI_EVENTS.TF_EDITING
                elseif eventData.key == KEYBOARD_CODES.KEY_V then
                    if self.state_select then
                        DeleteSelection(self, true)
                    end
                    Paste(self)
                    res.event = GUI_EVENTS.TF_EDITING
                elseif eventData.key == KEYBOARD_CODES.KEY_A then
                    SelectAll(self)
                    UpdateScroll(self)
                    UpdateCursorSelect(self)
                    UpdateTextPos(self)
                    res.event = GUI_EVENTS.TF_SELECTED
                end
            else      
                if EditString(self, eventData.key, eventData.symbol) == true then
                    res.event = GUI_EVENTS.TF_EDITING
                end
            end
        end
        res.entity = self.entity
        
    elseif eventData.event == GUI_EVENTS.MOUSE_UP then 
        if self.menu and self.menu:IsOpened() then
            res.event = GUI_EVENTS.NULL
            return res
        end

        if self.state_live then
            self.entity:SetHierarchyFocusOnMe(false)
            if self.selectable then
                self.selecting = false
            end
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.MOUSE_HOVER then 
        res.entity = self.entity
        res.event = GUI_EVENTS.NULL
        CoreGui.SetHCursor(SYSTEM_CURSORS.STICK)

    elseif eventData.event == GUI_EVENTS.MOUSE_OUT then 
        res.entity = self.entity
        res.event = GUI_EVENTS.NULL
        CoreGui.SetHCursor(SYSTEM_CURSORS.ARROW)

    elseif eventData.event == GUI_EVENTS.MOUSE_MOVE then 
        res.entity = self.entity
        res.event = GUI_EVENTS.NULL
        CoreGui.SetHCursor(SYSTEM_CURSORS.STICK)

        if self.selecting then
            local textObj = self.entity:GetText(self.str)
            local rect = self.entity:GetRectAbsolute()
            local relativeX = eventData.coords.x - rect.l - self.text.offset.x - self.border.width + self.scroll
            relativeX = math.max(0, relativeX)
            self.current_letter = textObj:GetClosestLetter(relativeX)
            self.cursor_pos = textObj:GetLetterPos(self.current_letter)
            
            if self.select_letter ~= self.current_letter then
                self.state_select = true
                res.event = GUI_EVENTS.TF_SELECTED
            end
            
            UpdateScroll(self)
            UpdateCursorSelect(self)
            UpdateTextPos(self)
        end
    
    elseif eventData.event == GUI_EVENTS.MENU_CLOSE then 
        self.entity:SetHierarchyFocusOnMe(false)

    elseif eventData.event == GUI_EVENTS.MENU_CLICK then 
        if eventData.entity:GetID() == "tf_copy" then
            Copy(self)
        elseif eventData.entity:GetID() == "tf_cut" then
            Copy(self)
            DeleteSelection(self, true)
            res.event = GUI_EVENTS.TF_EDITING
        elseif eventData.entity:GetID() == "tf_paste" then
            if self.state_select then
                DeleteSelection(self, true)
            end
            Paste(self)
            res.event = GUI_EVENTS.TF_EDITING
        elseif eventData.entity:GetID() == "tf_copyall" then
            SelectAll(self)
            Copy(self)
            UpdateScroll(self)
            UpdateCursorSelect(self)
            UpdateTextPos(self)
            res.event = GUI_EVENTS.TF_SELECTED
        elseif eventData.entity:GetID() == "tf_selectall" then
            SelectAll(self)
            UpdateScroll(self)
            UpdateCursorSelect(self)
            UpdateTextPos(self)
            res.event = GUI_EVENTS.TF_SELECTED
        end
        res.entity = self.entity
    end

    return self._base.callback(self, res)
end

function GuiTextfield:onActivate()
    self:ApplyNone()
end

function GuiTextfield:onDeactivate()
    self:ApplyDisable()
end

function GuiTextfield:SetText(text)
    if text == nil then
        if self.allow_none then self:SetText("") end
        return
    end

    if self.data.d_type == GUI_TEXTFIELD.FLOAT then
        local p = text:find('%.')
        if p ~= nil then
            p = p + self.data.decimal
            text = text:sub(1, p)
        end
    end
    if text:len() > self.text.length then return false end
    self.text.str = text
    self.entity:SetText(self.str, self.text.str)
    UpdateTextPos(self)
    if self.state_live then
        local textObj = self.entity:GetText(self.str)
        self.current_letter = textObj.letter_count
        self.cursor_pos = textObj:GetLetterPos(self.current_letter)
        self.select_letter = self.current_letter
        self.select_pos = self.cursor_pos
        UpdateCursorSelect(self)
    end
    return true
end

function GuiTextfield:SetNum(num)
    if num == nil then
        if self.allow_none then self:SetText("") end
        return
    end

    local val = math.max(self.data.min, math.min(self.data.max, num))
    if val < 0 then self.has_minus = true end
    
    if self.data.d_type == GUI_TEXTFIELD.INT then
        val = math.floor(val)
        return self:SetText(string.format("%i", val))
    elseif self.data.d_type == GUI_TEXTFIELD.FLOAT then
        self.has_point = true
        local str = string.format("%f", val)
        return self:SetText(str)
    end
    return false
end

function GuiTextfield:GetText()
    return self.text.str
end

function GuiTextfield:GetNum()
    if self.text.str:len() == 0 then return nil end

    if self.data.d_type == GUI_TEXTFIELD.TEXT then return 0 end
    local res = tonumber(self.text.str)
    if res == nil then return 0 end
    return res
end

function GuiTextfield:IsChanged()
    return not self.no_change
end

function GuiTextfield:SetActive(active)
    if active then 
        self:ApplyLive()
        self.state_live = true
        self.oldstr = self.text.str
        self.entity:SetHierarchyFocusOnMe(false)

        UpdateCursorSelect(self)
        UpdateScroll(self)
        UpdateTextPos(self)
    else
        local prt = self.entity:GetParent()
        prt:SetFocus(HEntity(), false)
    end
end