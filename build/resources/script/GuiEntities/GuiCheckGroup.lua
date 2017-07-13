if not GuiCheckGroup then GuiCheckGroup = class(GuiEntity) end

-- private
local UncheckAllBut = function (self, id)
    for i = 1, #self.checks do
        if i ~= id then self.checks[i]:SetCheck(false) end
    end
end

-- public
function GuiCheckGroup:SetCheck(id)
    if id > #self.checks then return end

    UncheckAllBut(self, id)
    self.checks[id]:SetCheck(true)
end

function GuiCheckGroup:GetCheck()
    return self.current_check
end

function GuiCheckGroup:init(props)
    --replace self:base(GuiCheckGroup).init(self, props)
    self.entity = HEntity()
    self.entity:Create()
    
    self:ApplyProps(props)
    self:ApplyCallbacks(props)

    local name_id = ""
    if props.id then name_id = props.id end

    self.entity:Init(name_id, self)
    
    -- child process
    self.checks = {}

    for ch = 1, #props do
        if props[ch]:is_a(GuiCheck) then
            table.insert(self.checks, props[ch])
            props[ch].group_id = #self.checks
        end
        if not props[ch].overlay then
            self.entity:AttachChild(props[ch].entity)
        else
            self:AttachOverlay(props[ch])
        end
    end

    if #self.checks < 2 then
        error("Pointless (checks < 2) GuiCheckGroup named "..name_id)
        return
    end

    self.current_check = 1
    self:SetCheck(self.current_check)
end

function GuiCheckGroup:callback(eventData)
    local res = eventData
    
    if not self.entity:IsActivated() or not self.entity:IsActivatedBranch() then
        if eventData.event == GUI_EVENTS.MOUSE_DOWN then
            self.entity:SetHierarchyFocusOnMe(false)
            res.event = GUI_EVENTS.DO_DENIED
        elseif not (eventData.event == GUI_EVENTS.UPDATE or eventData.event == GUI_EVENTS.SYS_MOVE or 
            eventData.event == GUI_EVENTS.SYS_RESIZE or eventData.event == GUI_EVENTS.MOUSE_WHEEL) then
            res.event = GUI_EVENTS.NULL
        end
        return self._base.callback(self, res)
    end

    if eventData.event == GUI_EVENTS.MOUSE_DOWN then 
        self.entity:SetHierarchyFocusOnMe(false)
        res.entity = self.entity

    elseif eventData.event == GUI_EVENTS.CB_CHECKED then 
        local check = eventData.entity:GetInherited()
        if check:is_a(GuiCheck) then
            self.current_check = check.group_id
            UncheckAllBut(self, self.current_check)

            res.event = GUI_EVENTS.CBGROUP_CHECK
            res.entity = self.entity
        end
        
    elseif eventData.event == GUI_EVENTS.CB_UNCHECKED then 
        local check = eventData.entity:GetInherited()
        if check:is_a(GuiCheck) then
            check:SetCheck(true)
            check:ApplyHover()
        end   
        
    elseif eventData.event == GUI_EVENTS.KEY_DOWN then 
        if eventData.key == KEYBOARD_CODES.KEY_DOWN then
            self.current_check = self.current_check + 1
            self.current_check = self.current_check > #self.checks and 1 or self.current_check
            self:SetCheck(self.current_check)
            res.entity = self.entity
            res.event = GUI_EVENTS.CBGROUP_CHECK
        elseif eventData.key == KEYBOARD_CODES.KEY_UP then
            self.current_check = self.current_check - 1
            self.current_check = self.current_check < 1 and #self.checks or self.current_check
            self:SetCheck(self.current_check)
            res.entity = self.entity
            res.event = GUI_EVENTS.CBGROUP_CHECK
        end         
    end

    return self._base.callback(self, res)
end