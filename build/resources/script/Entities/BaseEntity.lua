if not EntityTypes then 
    EntityTypes = {
        constructor = function (world, ent, classTable)
            return classTable(world, ent)
        end,

        wrap = function (world, ent)
            local res = world.script:GetLuaEntity(ent)
            if res ~= nil then
                return res
            else
                local ent_type = world:GetEntityType(ent)
                return EntityTypes[ent_type](world, ent)
            end
        end,
    }
end

if not EntityTypes.BaseEntity then EntityTypes.BaseEntity = class() end

function EntityTypes.BaseEntity:init(world, ent)
    self.world = world
    self.ent = ent
    
    self.transformSys = self.world.transform

    if self.ent ~= nil then
        if self.ent:IsNull() then self.ent = nil
        else return false end
    end  

    self.ent = self.world:CreateEntity()
    if self.ent:IsNull() then return false end

    return true
end

function EntityTypes.BaseEntity:Kill()
    if self.ent ~= nil then
        self.world:DestroyEntity(self.ent)
    end
    self.ent = nil
    self.world = nil
end

function EntityTypes.BaseEntity:IsAlive()
    return self.ent ~= nil
end

function EntityTypes.BaseEntity:SetPosition(x, y, z)
    return self.transformSys:SetPosition(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:SetRotation(p, y, r)
    return self.transformSys:SetRotation(self.ent, p, y, r)
end

function EntityTypes.BaseEntity:SetScale(x, y, z)
    return self.transformSys:SetScale(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:GetPositionW()
    return self.transformSys:GetPositionW(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:GetRotationW()
    return self.transformSys:GetRotationW(self.ent, p, y, r)
end

function EntityTypes.BaseEntity:GetScaleW()
    return self.transformSys:GetScaleW(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:GetPositionL()
    return self.transformSys:GetPositionL(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:GetRotationL()
    return self.transformSys:GetRotationL(self.ent, p, y, r)
end

function EntityTypes.BaseEntity:GetScaleL()
    return self.transformSys:GetScaleL(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:ForceUpdateTransform()
    return self.transformSys:ForceUpdate(self.ent)
end