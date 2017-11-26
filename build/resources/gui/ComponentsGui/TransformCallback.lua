if not TransformCallback then TransformCallback = {} end

function TransformCallback.SetName(self, ev)
    if not self:IsChanged() or #Viewport.selection_set ~= 1 then return true end

    local history = {
        s_oldname = "",
        s_newname = "",
        undo = function(self) 
                Viewport.lua_world.world:RenameEntity(Viewport.selection_set[1], self.s_oldname)
                Properties:UpdateData(false, COMPONENTS.TRANSFORM)
            end,
        redo = function(self) 
                Viewport.lua_world.world:RenameEntity(Viewport.selection_set[1], self.s_newname)
                Properties:UpdateData(false, COMPONENTS.TRANSFORM)
            end,
        msg = "Rename"
    }

    history.s_newname = self:GetText()
    history.s_oldname = Viewport.lua_world.world:GetEntityName(Viewport.selection_set[1])
    
    if history.s_oldname == history.s_newname then return true end

    Viewport.lua_world.world:RenameEntity(Viewport.selection_set[1], history.s_newname)

    History:Push(history)
    return true
end

function TransformCallback.UpdName(self, ev)
    if #Viewport.selection_set == 1 then
        local name = Viewport.lua_world.world:GetEntityName(Viewport.selection_set[1])
        self:SetText(name)
        self.entity:Activate()
    else
        self:SetText()
        self.entity:Deactivate()
    end
    return true 
end

function TransformCallback.SetParent(self, ev)
    if not self:IsChanged() then return true end
    local parent = self:GetText()

    local history = {
        s_oldval = {},
        s_newval = "",
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    local newParentEnt = Viewport.lua_world.world:GetEntityByName(self.s_oldval[i])
                    Viewport.lua_world.world.transform:Attach(ent, newParentEnt)
                end
                Properties:UpdateData(false, COMPONENTS.TRANSFORM)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    local newParentEnt = Viewport.lua_world.world:GetEntityByName(self.s_newval)
                    Viewport.lua_world.world.transform:Attach(ent, newParentEnt)
                end
                Properties:UpdateData(false, COMPONENTS.TRANSFORM)
            end,
        msg = "Parent reattach"
    }

    history.s_newval = parent
    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        local parentEnt = Viewport.lua_world.world.transform:GetParent(ent)
        history.s_oldval[i] = Viewport.lua_world.world:GetEntityName(parentEnt)

        is_change = is_change or history.s_oldval[i] ~= history.s_newval

        local newParentEnt = Viewport.lua_world.world:GetEntityByName(history.s_newval)
        Viewport.lua_world.world.transform:Attach(ent, newParentEnt)
    end

    if not is_change then return true end

    History:Push(history)
    return true
end

function TransformCallback.UpdParent(self, ev)
    local parent = ""
    for i, ent in ipairs(Viewport.selection_set) do
        local parentEnt = Viewport.lua_world.world.transform:GetParent(ent)
        local parentName = Viewport.lua_world.world:GetEntityName(parentEnt)
        if i > 1 and parent ~= parentName then
            self:SetText()
            return true
        else parent = parentName end
    end
    self:SetText(parent)
    return true 
end

function TransformCallback.SetPos(self, ev, xyz)
    if not self:IsChanged() then return true end
    local val = self:GetNum()
    if val == nil then
        self.events[GUI_EVENTS.UPDATE](self, ev)
        return true 
    end

    local history = {
        s_oldval = {},
        s_newval = {},
        undo = function(self) Viewport:SetPositionsToSelection(self.s_oldval) end,
        redo = function(self) Viewport:SetPositionsToSelection(self.s_newval) end,
        msg = "Transform move"
    }

    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.transform:GetPosition_L(ent)

        if xyz == 1 then history.s_newval[i] = Vector3(val, history.s_oldval[i].y, history.s_oldval[i].z)
        elseif xyz == 2 then history.s_newval[i] = Vector3(history.s_oldval[i].x, val, history.s_oldval[i].z)
        else history.s_newval[i] = Vector3(history.s_oldval[i].x, history.s_oldval[i].y, val) end

        is_change = is_change or not (history.s_newval[i].x == history.s_oldval[i].x and
            history.s_newval[i].y == history.s_oldval[i].y and
            history.s_newval[i].z == history.s_oldval[i].z)

        Viewport.lua_world.world.transform:SetPosition_L(ent, history.s_newval[i])
        Viewport.lua_world.world.transform:ForceUpdate(ent)
    end

    if not is_change then return true end
    
    TransformControls:UpdateTransform(Viewport.selection_set)
    History:Push(history)
    return true 
end

function TransformCallback.UpdPos(self, ev, xyz)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local pos = Viewport.lua_world.world.transform:GetPosition_L(ent)
        local pos_coord = 0
        if xyz == 1 then pos_coord = pos.x
        elseif xyz == 2 then pos_coord = pos.y
        else pos_coord = pos.z end

        if i > 1 and val ~= pos_coord then
            self:SetNum(nil)
            return true
        else val = pos_coord end
    end
    self:SetNum(val)
    return true 
end

function TransformCallback.SetRot(self, ev, pyr)
    if not self:IsChanged() then return true end
    local val = self:GetNum()
    if val == nil then
        self.events[GUI_EVENTS.UPDATE](self, ev)
        return true 
    end
    val = val * math.pi / 180

    local history = {
        s_oldval = {},
        s_newval = {},
        undo = function(self) Viewport:SetRotationsToSelection(self.s_oldval) end,
        redo = function(self) Viewport:SetRotationsToSelection(self.s_newval) end,
        msg = "Transform rotate"
    }

    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.transform:GetRotationPYR_L(ent)

        if pyr == 1 then history.s_newval[i] = Vector3(val, history.s_oldval[i].y, history.s_oldval[i].z)
        elseif pyr == 2 then history.s_newval[i] = Vector3(history.s_oldval[i].x, val, history.s_oldval[i].z)
        else history.s_newval[i] = Vector3(history.s_oldval[i].x, history.s_oldval[i].y, val) end

        is_change = is_change or not (history.s_newval[i].x == history.s_oldval[i].x and
            history.s_newval[i].y == history.s_oldval[i].y and
            history.s_newval[i].z == history.s_oldval[i].z)

        Viewport.lua_world.world.transform:SetRotationPYR_L(ent, history.s_newval[i])
        Viewport.lua_world.world.transform:ForceUpdate(ent)
    end

    if not is_change then return true end
    
    TransformControls:UpdateTransform(Viewport.selection_set)
    History:Push(history)
    return true 
end

function TransformCallback.UpdRot(self, ev, pyr)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local rot = Viewport.lua_world.world.transform:GetRotationPYR_L(ent)
        local rot_coord = 0
        if pyr == 1 then rot_coord = rot.x * 180 / math.pi
        elseif pyr == 2 then rot_coord = rot.y * 180 / math.pi
        else rot_coord = rot.z * 180 / math.pi end

        if i > 1 and val ~= rot_coord then
            self:SetNum(nil)
            return true
        else val = rot_coord end
    end
    self:SetNum(val)
    return true 
end

function TransformCallback.SetScale(self, ev, xyz)
    if not self:IsChanged() then return true end
    local val = self:GetNum()
    if val == nil then
        self.events[GUI_EVENTS.UPDATE](self, ev)
        return true 
    end

    local history = {
        s_oldval = {},
        s_newval = {},
        undo = function(self) Viewport:SetScalesToSelection(self.s_oldval) end,
        redo = function(self) Viewport:SetScalesToSelection(self.s_newval) end,
        msg = "Transform scale"
    }

    local is_change = false
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.transform:GetScale_L(ent)

        if xyz == 1 then history.s_newval[i] = Vector3(val, history.s_oldval[i].y, history.s_oldval[i].z)
        elseif xyz == 2 then history.s_newval[i] = Vector3(history.s_oldval[i].x, val, history.s_oldval[i].z)
        else history.s_newval[i] = Vector3(history.s_oldval[i].x, history.s_oldval[i].y, val) end

        is_change = is_change or not (history.s_newval[i].x == history.s_oldval[i].x and
            history.s_newval[i].y == history.s_oldval[i].y and
            history.s_newval[i].z == history.s_oldval[i].z)

        Viewport.lua_world.world.transform:SetScale_L(ent, history.s_newval[i])
        Viewport.lua_world.world.transform:ForceUpdate(ent)
    end
    
    if not is_change then return true end
    
    TransformControls:UpdateTransform(Viewport.selection_set)
    History:Push(history)
    return true 
end

function TransformCallback.UpdScale(self, ev, xyz)
    local val = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local scale = Viewport.lua_world.world.transform:GetScale_L(ent)
        local scale_coord = 0
        if xyz == 1 then scale_coord = scale.x
        elseif xyz == 2 then scale_coord = scale.y
        else scale_coord = scale.z end

        if i > 1 and val ~= scale_coord then
            self:SetNum(nil)
            return true
        else val = scale_coord end
    end
    self:SetNum(val)
    return true 
end