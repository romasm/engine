if not NormalCallback then NormalCallback = {} end

function NormalCallback.SetNormalTex(self, ev)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        undo = function(self) 
                MaterialProps:SetNormalTexture(self.s_oldval)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps:SetNormalTexture(self.s_newval)
                MaterialProps:UpdateData(false)
            end,
        msg = "Albedo texture"
    }

    history.s_oldval = MaterialProps:GetNormalTexture()
    history.s_newval = texture
    
    MaterialProps:SetNormalTexture(texture)

    local group = self.entity:GetParent()
    local space_sl = group:GetChildById('normal_space')
    local space_str = group:GetChildById('normal_space_str')
    if texture:len() == 0 then
        space_sl:Deactivate()
        space_str:Deactivate()
    else
        space_sl:Activate()
        space_str:Activate()
    end

    History:Push(history)
    return true
end

function NormalCallback.UpdNormalTex(self, ev)
    local group = self.entity:GetParent()
    local space_sl = group:GetChildById('normal_space')
    local space_str = group:GetChildById('normal_space_str')

    local texture = MaterialProps:GetNormalTexture()
    self:SetTexture( texture )

    if texture:len() == 0 then
        space_sl:Deactivate()
        space_str:Deactivate()
    else
        space_sl:Activate()
        space_str:Activate()
    end
    return true
end

function NormalCallback.SetNormalSpace(self, ev)
    local selected = self:GetSelected()

    local history = {
        s_oldval = false,
        s_newval = false,
        undo = function(self) 
                MaterialProps:SetNormalSpace(self.s_oldval)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps:SetNormalSpace(self.s_newval)
                MaterialProps:UpdateData(false)
            end,
        msg = "Normals space"
    }

    history.s_oldval = MaterialProps:GetNormalSpace()
    history.s_newval = (selected > 1)

    if history.s_oldval == history.s_newval then return true end

    MaterialProps:SetNormalSpace(history.s_newval)

    History:Push(history)
    return true
end

function NormalCallback.UpdNormalSpace(self, ev)
    local space = MaterialProps:GetNormalSpace()
    if space == true then self:SetSelected(2)
    else self:SetSelected(1) end
    return true
end