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

function EntityTypes.BaseEntity:KillHi()
    if self.ent ~= nil then
        self.world:DestroyEntityHierarchically(self.ent)
    end
    self.ent = nil
    self.world = nil
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

function EntityTypes.BaseEntity:Rename(name)
    return self.world:RenameEntity(self.ent, name)
end

function EntityTypes.BaseEntity:SetPosition(x, y, z)
    return self.transformSys:SetPosition(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:SetRotation(p, y, r)
    return self.transformSys:SetRotation(self.ent, p, y, r)
end

function EntityTypes.BaseEntity:SetRotationAxis(axis, angle)
    return self.transformSys:SetRotationAxis(self.ent, axis, angle)
end

function EntityTypes.BaseEntity:SetScale(x, y, z)
    return self.transformSys:SetScale(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:AddPositionLocal(x, y, z)
    return self.transformSys:AddPositionLocal(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:AddPosition(x, y, z)
    return self.transformSys:AddPosition(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:AddRotation(p, y, r)
    return self.transformSys:AddRotation(self.ent, p, y, r)
end

function EntityTypes.BaseEntity:AddRotationAxis(axis, angle)
    return self.transformSys:AddRotationAxis(self.ent, axis, angle)
end

function EntityTypes.BaseEntity:AddScale(x, y, z)
    return self.transformSys:AddScale(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:GetPositionW()
    return self.transformSys:GetPositionW(self.ent)
end

function EntityTypes.BaseEntity:GetRotationW()
    return self.transformSys:GetRotationW(self.ent)
end

function EntityTypes.BaseEntity:GetDirectionW()
    return self.transformSys:GetDirectionW(self.ent)
end

function EntityTypes.BaseEntity:GetScaleW()
    return self.transformSys:GetScaleW(self.ent)
end

function EntityTypes.BaseEntity:GetPositionL()
    return self.transformSys:GetPositionL(self.ent)
end

function EntityTypes.BaseEntity:GetRotationL()
    return self.transformSys:GetRotationL(self.ent)
end

function EntityTypes.BaseEntity:GetDirectionL()
    return self.transformSys:GetDirectionL(self.ent)
end

function EntityTypes.BaseEntity:GetScaleL()
    return self.transformSys:GetScaleL(self.ent)
end

function EntityTypes.BaseEntity:ForceUpdateTransform()
    return self.transformSys:ForceUpdate(self.ent)
end

function EntityTypes.BaseEntity:Attach(parent)
    return self.transformSys:Attach(self.ent, parent.ent)
end

function EntityTypes.BaseEntity:Detach()
    return self.transformSys:Detach(self.ent)
end

function EntityTypes.BaseEntity:DetachChildren()
    return self.transformSys:DetachChildren(self.ent)
end

function EntityTypes.BaseEntity:GetParent()
    return self.transformSys:GetParent(self.ent)
end

function EntityTypes.BaseEntity:GetChildFirst()
    return self.transformSys:GetChildFirst(self.ent)
end

function EntityTypes.BaseEntity:GetNext()
    return self.transformSys:GetChildNext(self.ent)
end

function EntityTypes.BaseEntity:Enable()
    return self.world:SetEntityEnable(self.ent, true)
end

function EntityTypes.BaseEntity:Disable()
    return self.world:SetEntityEnable(self.ent, false)
end

function EntityTypes.BaseEntity:IsEnabled()
    return self.world:IsEntityEnable(self.ent)
end

-- EDITOR ONLY
function EntityTypes.BaseEntity:Visible(visible)
    return self.world:SetEntityEditorVisible(self.ent, visible)
end

function EntityTypes.BaseEntity:IsVisible()
    return self.world:IsEntityEditorVisible(self.ent)
end


-- Node
if not EntityTypes.Node then EntityTypes.Node = class(EntityTypes.BaseEntity) end

function EntityTypes.Node:init(world, ent)
    if not self:base(EntityTypes.Node).init(self, world, ent) then 
        return 
    end

    -- init
    self.world:SetEntityType(self.ent, "Node")
    self.transformSys:AddComponent(self.ent)
end
