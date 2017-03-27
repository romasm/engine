loader.require("MaterialGui.TransmittanceCallback")

if not ScatteringCallback then ScatteringCallback = {} end

function ScatteringCallback.StartAttenuation(self)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        undo = function(self) 
                MaterialProps.material:SetDefferedParam(self.s_oldval, "attenuation")
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetDefferedParam(self.s_newval, "attenuation")
                MaterialProps:UpdateData(false)
            end,
        msg = "Attenuation scattering value"
    }

    self.history.s_oldval = MaterialProps.material:GetDefferedParam("attenuation")

    local ext = TransmittanceCallback.DistanceAttenuation(self:GetValue())
    MaterialProps.material:SetDefferedParam(ext, "attenuation")
    return true
end

function ScatteringCallback.DragAttenuation(self)
    local ext = TransmittanceCallback.DistanceAttenuation(self:GetValue())
    MaterialProps.material:SetDefferedParam(ext, "attenuation")
    return true
end

function ScatteringCallback.EndAttenuation(self)
    self.history.s_newval = TransmittanceCallback.DistanceAttenuation(self:GetValue())
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    MaterialProps.material:SetDefferedParam(self.history.s_newval, "attenuation")
    History:Push(self.history)
    return true
end

function ScatteringCallback.UpdAttenuation(self)
    local ext = MaterialProps.material:GetDefferedParam("attenuation")
    self:SetValue( TransmittanceCallback.DistanceAttenuation(ext) )
    return true
end

function ScatteringCallback.StartIOR(self)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        undo = function(self) 
                MaterialProps.material:SetDefferedParam(self.s_oldval, "ior")
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetDefferedParam(self.s_newval, "ior")
                MaterialProps:UpdateData(false)
            end,
        msg = "IOR scattering value"
    }

    self.history.s_oldval = MaterialProps.material:GetDefferedParam("ior")

    local ior = 1.0 / self:GetValue()
    MaterialProps.material:SetDefferedParam(ior, "ior")
    return true
end

function ScatteringCallback.DragIOR(self)
    local ior = 1.0 / self:GetValue()
    MaterialProps.material:SetDefferedParam(ior, "ior")
    return true
end

function ScatteringCallback.EndIOR(self)
    self.history.s_newval = 1.0 / self:GetValue()
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    MaterialProps.material:SetDefferedParam(self.history.s_newval, "ior")
    History:Push(self.history)
    return true
end

function ScatteringCallback.UpdIOR(self)
    local ior = MaterialProps.material:GetDefferedParam("ior")
    self:SetValue( 1.0 / ior )
    return true
end