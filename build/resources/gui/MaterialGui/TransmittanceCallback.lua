if not TransmittanceCallback then TransmittanceCallback = {} end

local DistanceAttenuation = function(value)
    -- LightMul = exp( -AttenuationCoeff * Dist)
    -- LightMul = 0.5; AttenuationCoeff = 0.69314718 / Dist
    return 0.69314718 / value
end

function TransmittanceCallback.StartAttenuation(self)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        undo = function(self) 
                MaterialProps.material:SetFloat(self.s_oldval, "attenuationValue", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloat(self.s_newval, "attenuationValue", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Transmittance value"
    }

    self.history.s_oldval = MaterialProps.material:GetFloat("attenuationValue", SHADERS.PS)

    local ext = DistanceAttenuation(self:GetValue())
    MaterialProps.material:SetFloat(ext, "attenuationValue", SHADERS.PS)
    return true
end

function TransmittanceCallback.DragAttenuation(self)
    local ext = DistanceAttenuation(self:GetValue())
    MaterialProps.material:SetFloat(ext, "attenuationValue", SHADERS.PS)
    return true
end

function TransmittanceCallback.EndAttenuation(self)
    self.history.s_newval = DistanceAttenuation(self:GetValue())
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    MaterialProps.material:SetFloat(self.history.s_newval, "attenuationValue", SHADERS.PS)
    History:Push(self.history)
    return true
end

function TransmittanceCallback.UpdAttenuation(self)
    local ext = MaterialProps.material:GetFloat("attenuationValue", SHADERS.PS)
    self:SetValue( DistanceAttenuation(ext) )
    return true
end

-- IOR & Abbe

-- http://arachnoid.com/OpticalRayTracer_technical/resources/opticalraytracer_technical.pdf
-- IORg = IOR
-- IORr = IOR - 0.13197861 / Abbe
-- IORb = IOR + 0.37056297 / Abbe
-- IOR air = 1.000277

local InvIOR = function (inv_ior)
    --return 1.000277 / inv_ior
    return 1.0 / inv_ior
end

local CalcAbbe = function (inv_ior_r, inv_ior_g, inv_ior_b)
    if inv_ior_r == inv_ior_g or inv_ior_g == inv_ior_b then return 70.0 end

    local abbe = 0.37056297 / (InvIOR(inv_ior_b) - InvIOR(inv_ior_g))
    return abbe
end

local IorToDispersionIor = function (old_inv_ior_r, old_inv_ior_g, old_inv_ior_b, new_inv_ior)
    local abbe = CalcAbbe(old_inv_ior_r, old_inv_ior_g, old_inv_ior_b)
    if abbe <= 0.0 or abbe >= 70.0 then return new_inv_ior, new_inv_ior, new_inv_ior end

    local ior = InvIOR(new_inv_ior)

    local ior_r = math.max(1.0, ior - 0.13197861 / abbe)
    local ior_b = math.max(1.0, ior + 0.37056297 / abbe)
    return InvIOR(ior_r), new_inv_ior, InvIOR(ior_b)
end

local AbbeToDispersionIor = function (inv_ior, abbe)
    if abbe >= 70.0 then return inv_ior, inv_ior, inv_ior end

    local ior = InvIOR(inv_ior)

    local ior_r = math.max(1.0, ior - 0.13197861 / abbe)
    local ior_b = math.max(1.0, ior + 0.37056297 / abbe)
    return InvIOR(ior_r), inv_ior, InvIOR(ior_b)
end

function TransmittanceCallback.StartIOR(self)
    self.history = {
        s_oldvalR = 0,
        s_oldvalG = 0,
        s_oldvalB = 0,
        s_newvalR = 0,
        s_newvalG = 0,
        s_newvalB = 0,
        undo = function(self) 
                MaterialProps.material:SetFloat(self.s_oldvalR, "invIorRed", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_oldvalG, "invIorGreen", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_oldvalB, "invIorBlue", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloat(self.s_newvalR, "invIorRed", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_newvalG, "invIorGreen", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_newvalB, "invIorBlue", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "IOR"
    }

    self.history.s_oldvalR = MaterialProps.material:GetFloat("invIorRed", SHADERS.PS)
    self.history.s_oldvalG = MaterialProps.material:GetFloat("invIorGreen", SHADERS.PS)
    self.history.s_oldvalB = MaterialProps.material:GetFloat("invIorBlue", SHADERS.PS)

    local ior_r, ior_g, ior_b = IorToDispersionIor(self.history.s_oldvalR, self.history.s_oldvalG, self.history.s_oldvalB, InvIOR(self:GetValue()))

    MaterialProps.material:SetFloat(ior_r, "invIorRed", SHADERS.PS)
    MaterialProps.material:SetFloat(ior_g, "invIorGreen", SHADERS.PS)
    MaterialProps.material:SetFloat(ior_b, "invIorBlue", SHADERS.PS)
    return true
end

function TransmittanceCallback.DragIOR(self)
    local oldIorR = MaterialProps.material:GetFloat("invIorRed", SHADERS.PS)
    local oldIorG = MaterialProps.material:GetFloat("invIorGreen", SHADERS.PS)
    local oldIorB = MaterialProps.material:GetFloat("invIorBlue", SHADERS.PS)

    local ior_r, ior_g, ior_b = IorToDispersionIor(oldIorR, oldIorG, oldIorB, InvIOR(self:GetValue()))
    
    MaterialProps.material:SetFloat(ior_r, "invIorRed", SHADERS.PS)
    MaterialProps.material:SetFloat(ior_g, "invIorGreen", SHADERS.PS)
    MaterialProps.material:SetFloat(ior_b, "invIorBlue", SHADERS.PS)
    return true
end

function TransmittanceCallback.EndIOR(self)
    local oldIorR = MaterialProps.material:GetFloat("invIorRed", SHADERS.PS)
    local oldIorG = MaterialProps.material:GetFloat("invIorGreen", SHADERS.PS)
    local oldIorB = MaterialProps.material:GetFloat("invIorBlue", SHADERS.PS)

    self.history.s_newvalR, self.history.s_newvalG, self.history.s_newvalB = IorToDispersionIor(oldIorR, oldIorG, oldIorB, InvIOR(self:GetValue()))
    
    if CMath.IsNearlyEq(self.history.s_oldvalR, self.history.s_newvalR, 0.0001) and 
        CMath.IsNearlyEq(self.history.s_oldvalG, self.history.s_newvalG, 0.0001) and
        CMath.IsNearlyEq(self.history.s_oldvalB, self.history.s_newvalB, 0.0001) then return true end

    MaterialProps.material:SetFloat(self.history.s_newvalR, "invIorRed", SHADERS.PS)
    MaterialProps.material:SetFloat(self.history.s_newvalG, "invIorGreen", SHADERS.PS)
    MaterialProps.material:SetFloat(self.history.s_newvalB, "invIorBlue", SHADERS.PS)
    
    if self.history.s_newvalG == 1.0 then
        local abbe_slider = self.entity:GetParent():GetChildById("abbe_slider"):GetInherited()
        abbe_slider:SetValue(70.0)
    end


    History:Push(self.history)
    return true
end

function TransmittanceCallback.UpdIOR(self)
    local ior = MaterialProps.material:GetFloat("invIorGreen", SHADERS.PS)
    self:SetValue( InvIOR(ior) )
    return true
end

function TransmittanceCallback.StartAbbe(self)
    self.history = {
        s_oldvalR = 0,
        s_oldvalG = 0,
        s_oldvalB = 0,
        s_newvalR = 0,
        s_newvalG = 0,
        s_newvalB = 0,
        undo = function(self) 
                MaterialProps.material:SetFloat(self.s_oldvalR, "invIorRed", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_oldvalG, "invIorGreen", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_oldvalB, "invIorBlue", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloat(self.s_newvalR, "invIorRed", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_newvalG, "invIorGreen", SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_newvalB, "invIorBlue", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "Abbe number"
    }

    self.history.s_oldvalR = MaterialProps.material:GetFloat("invIorRed", SHADERS.PS)
    self.history.s_oldvalG = MaterialProps.material:GetFloat("invIorGreen", SHADERS.PS)
    self.history.s_oldvalB = MaterialProps.material:GetFloat("invIorBlue", SHADERS.PS)

    local ior_r, ior_g, ior_b = AbbeToDispersionIor(self.history.s_oldvalG, self:GetValue())

    MaterialProps.material:SetFloat(ior_r, "invIorRed", SHADERS.PS)
    MaterialProps.material:SetFloat(ior_g, "invIorGreen", SHADERS.PS)
    MaterialProps.material:SetFloat(ior_b, "invIorBlue", SHADERS.PS)
    return true
end

function TransmittanceCallback.DragAbbe(self)
    local oldIorG = MaterialProps.material:GetFloat("invIorGreen", SHADERS.PS)

    local ior_r, ior_g, ior_b = AbbeToDispersionIor(oldIorG, self:GetValue())
    
    MaterialProps.material:SetFloat(ior_r, "invIorRed", SHADERS.PS)
    MaterialProps.material:SetFloat(ior_g, "invIorGreen", SHADERS.PS)
    MaterialProps.material:SetFloat(ior_b, "invIorBlue", SHADERS.PS)
    return true
end

function TransmittanceCallback.EndAbbe(self)
    local oldIorG = MaterialProps.material:GetFloat("invIorGreen", SHADERS.PS)

    self.history.s_newvalR, self.history.s_newvalG, self.history.s_newvalB = AbbeToDispersionIor(oldIorG, self:GetValue())
    
    if CMath.IsNearlyEq(self.history.s_oldvalR, self.history.s_newvalR, 0.0001) and 
        CMath.IsNearlyEq(self.history.s_oldvalG, self.history.s_newvalG, 0.0001) and
        CMath.IsNearlyEq(self.history.s_oldvalB, self.history.s_newvalB, 0.0001) then return true end

    MaterialProps.material:SetFloat(self.history.s_newvalR, "invIorRed", SHADERS.PS)
    MaterialProps.material:SetFloat(self.history.s_newvalG, "invIorGreen", SHADERS.PS)
    MaterialProps.material:SetFloat(self.history.s_newvalB, "invIorBlue", SHADERS.PS)

    History:Push(self.history)
    return true
end

function TransmittanceCallback.UpdAbbe(self)
    local oldIorR = MaterialProps.material:GetFloat("invIorRed", SHADERS.PS)
    local oldIorG = MaterialProps.material:GetFloat("invIorGreen", SHADERS.PS)
    local oldIorB = MaterialProps.material:GetFloat("invIorBlue", SHADERS.PS)

    local abbe = CalcAbbe(oldIorR, oldIorG, oldIorB)
    self:SetValue( abbe )
    return true
end

function TransmittanceCallback.SetIORAsSpec(self, checked)
    local history = {
        s_oldval = false,
        s_newval = false,
        undo = function(self) 
                MaterialProps.material:SetFloat(self.s_oldval and 1.0 or 0.0, "iorAsSpecular", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloat(self.s_newval and 1.0 or 0.0, "iorAsSpecular", SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = "IOR reflection"
    }

    history.s_oldval = (MaterialProps.material:GetFloat("iorAsSpecular", SHADERS.PS) > 0.0)
    history.s_newval = checked

    if history.s_oldval == history.s_newval then return true end

    MaterialProps.material:SetFloat(checked and 1.0 or 0.0, "iorAsSpecular", SHADERS.PS)

    History:Push(history)
    return true
end

function TransmittanceCallback.UpdIORAsSpec(self, ev)
    local checked = (MaterialProps.material:GetFloat("iorAsSpecular", SHADERS.PS) > 0.0)
    self:SetCheck( checked )
    return true
end