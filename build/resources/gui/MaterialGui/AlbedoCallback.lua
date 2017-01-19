if not AlbedoCallback then AlbedoCallback = {} end

function AlbedoCallback.SetAlbedoTex(self)
    local texture = self:GetTexture()

    local history = {
        s_oldval = "",
        s_newval = "",
        undo = function(self) 
                MaterialProps:SetAlbedoTexture(self.s_oldval)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps:SetAlbedoTexture(self.s_newval)
                MaterialProps:UpdateData(false)
            end,
        msg = "Albedo texture"
    }

    history.s_oldval = MaterialProps:GetAlbedoTexture()
    history.s_newval = texture
    
    MaterialProps:SetAlbedoTexture(texture)

    History:Push(history)
    return true
end

function AlbedoCallback.UpdAlbedoTex(self)
    self:SetTexture( MaterialProps:GetAlbedoTexture() )
    return true
end