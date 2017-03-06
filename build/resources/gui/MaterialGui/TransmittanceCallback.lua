if not TransmittanceCallback then TransmittanceCallback = {} end

function TransmittanceCallback.ExtinctionToAbsorption(extinc)
    return 4 * math.pi * extinc / 0.5876
end

function TransmittanceCallback.AbsorptionToExtinction(absorp)
    return absorp * 0.5876 / (4 * math.pi)
end

function TransmittanceCallback.StartExtinction(self)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        slot = "absorptionValue",
        undo = function(self) 
                MaterialProps.material:SetFloat(self.s_oldval, self.slot, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloat(self.s_newval, self.slot, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Transmittance value"
    }

    self.history.s_oldval = MaterialProps.material:GetFloat("absorptionValue", SHADERS.PS)

    local ext = TransmittanceCallback.ExtinctionToAbsorption( self:GetValue() )
    MaterialProps.material:SetFloat(ext, "absorptionValue", SHADERS.PS)
    return true
end

function TransmittanceCallback.DragExtinction(self)
    local ext = TransmittanceCallback.ExtinctionToAbsorption( self:GetValue() )
    MaterialProps.material:SetFloat(ext, "absorptionValue", SHADERS.PS)
    return true
end

function TransmittanceCallback.EndExtinction(self)
    self.history.s_newval = TransmittanceCallback.ExtinctionToAbsorption( self:GetValue() )
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    MaterialProps.material:SetFloat(self.history.s_newval, "absorptionValue", SHADERS.PS)
    History:Push(self.history)
    return true
end

function TransmittanceCallback.UpdExtinction(self)
    local ext = TransmittanceCallback.AbsorptionToExtinction( MaterialProps.material:GetFloat("absorptionValue", SHADERS.PS) )
    self:SetValue( ext )
    return true
end