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

-- Set transform

function EntityTypes.BaseEntity:SetPosition_L3F(x, y, z)
    return self.transformSys:SetPosition_L3F(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:SetPosition_L(vect)
    return self.transformSys:SetPosition_L(self.ent, vect)
end

function EntityTypes.BaseEntity:SetRotationPYR_L3F(p, y, r)
    return self.transformSys:SetRotationPYR_L3F(self.ent, p, y, r)
end

function EntityTypes.BaseEntity:SetRotationPYR_L(pyr_vect)
    return self.transformSys:SetRotationPYR_L(self.ent, pyr_vect)
end

function EntityTypes.BaseEntity:SetRotation_L(quat)
    return self.transformSys:SetRotation_L(self.ent, quat)
end

function EntityTypes.BaseEntity:SetRotationAxis_L(axis, angle)
    return self.transformSys:SetRotationAxis_L(self.ent, axis, angle)
end

function EntityTypes.BaseEntity:SetScale_L3F(x, y, z)
    return self.transformSys:SetScale_L3F(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:SetScale_L(vect)
    return self.transformSys:SetScale_L(self.ent, vect)
end

function EntityTypes.BaseEntity:SetPosition_W3F(x, y, z)
    return self.transformSys:SetPosition_W3F(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:SetPosition_W(vect)
    return self.transformSys:SetPosition_W(self.ent, vect)
end

function EntityTypes.BaseEntity:SetRotationPYR_W3F(p, y, r)
    return self.transformSys:SetRotationPYR_W3F(self.ent, p, y, r)
end

function EntityTypes.BaseEntity:SetRotation_W(quat)
    return self.transformSys:SetRotation_W(self.ent, quat)
end

function EntityTypes.BaseEntity:SetRotationPYR_W(pyr)
    return self.transformSys:SetRotationPYR_W(self.ent, pyr)
end

function EntityTypes.BaseEntity:SetRotationAxis_W(axis, angle)
    return self.transformSys:SetRotationAxis_W(self.ent, axis, angle)
end

function EntityTypes.BaseEntity:SetScale_W(x, y, z)
    return self.transformSys:SetScale_W(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:SetScale_W(vect)
    return self.transformSys:SetScale_W(self.ent, vect)
end

function EntityTypes.BaseEntity:SetTransform_L(mat)
    return self.transformSys:SetTransform_L(self.ent, mat)
end

function EntityTypes.BaseEntity:SetTransform_W(mat)
    return self.transformSys:SetTransform_W(self.ent, mat)
end

-- Add transform

function EntityTypes.BaseEntity:AddPosition_L3F(x, y, z)
    return self.transformSys:AddPosition_L3F(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:AddPosition_L(vect)
    return self.transformSys:AddPosition_L(self.ent, vect)
end

function EntityTypes.BaseEntity:AddRotationPYR_L3F(p, y, r)
    return self.transformSys:AddRotationPYR_L3F(self.ent, p, y, r)
end

function EntityTypes.BaseEntity:AddRotationPYR_L(pyr_vect)
    return self.transformSys:AddRotationPYR_L(self.ent, pyr_vect)
end

function EntityTypes.BaseEntity:AddRotation_L(quat)
    return self.transformSys:AddRotation_L(self.ent, quat)
end

function EntityTypes.BaseEntity:AddRotationAxis_L(axis, angle)
    return self.transformSys:AddRotationAxis_L(self.ent, axis, angle)
end

function EntityTypes.BaseEntity:AddScale_L3F(x, y, z)
    return self.transformSys:AddScale_L3F(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:AddScale_L(vect)
    return self.transformSys:AddScale_L(self.ent, vect)
end

function EntityTypes.BaseEntity:AddPosition_W3F(x, y, z)
    return self.transformSys:AddPosition_W3F(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:AddPosition_W(vect)
    return self.transformSys:AddPosition_W(self.ent, vect)
end

function EntityTypes.BaseEntity:AddRotationPYR_W3F(p, y, r)
    return self.transformSys:AddRotationPYR_W3F(self.ent, p, y, r)
end

function EntityTypes.BaseEntity:AddRotationPYR_W(pyr_vect)
    return self.transformSys:AddRotationPYR_W(self.ent, pyr_vect)
end

function EntityTypes.BaseEntity:AddRotation_W(quat)
    return self.transformSys:AddRotation_W(self.ent, quat)
end

function EntityTypes.BaseEntity:AddRotationAxis_W(axis, angle)
    return self.transformSys:AddRotationAxis_W(self.ent, axis, angle)
end

function EntityTypes.BaseEntity:AddScale_W3F(x, y, z)
    return self.transformSys:AddScale_W3F(self.ent, x, y, z)
end

function EntityTypes.BaseEntity:AddScale_W(vect)
    return self.transformSys:AddScale_W(self.ent, vect)
end

-- Get transform

function EntityTypes.BaseEntity:GetPosition_W()
    return self.transformSys:GetPosition_W(self.ent)
end

function EntityTypes.BaseEntity:GetRotation_W()
    return self.transformSys:GetRotation_W(self.ent)
end

function EntityTypes.BaseEntity:GetRotationPYR_W()
    return self.transformSys:GetRotationPYR_W(self.ent)
end

function EntityTypes.BaseEntity:GetForward_W()
	return self.transformSys:GetForward_W(self.ent)
end

function EntityTypes.BaseEntity:GetScale_W()
    return self.transformSys:GetScale_W(self.ent)
end

function EntityTypes.BaseEntity:GetPosition_L()
    return self.transformSys:GetPosition_L(self.ent)
end

function EntityTypes.BaseEntity:GetRotation_L()
    return self.transformSys:GetRotation_L(self.ent)
end

function EntityTypes.BaseEntity:GetRotationPYR_L()
    return self.transformSys:GetRotationPYR_L(self.ent)
end

function EntityTypes.BaseEntity:GetForward_L()
	return self.transformSys:GetForward_L(self.ent)
end

function EntityTypes.BaseEntity:GetScale_L()
    return self.transformSys:GetScale_L(self.ent)
end

function EntityTypes.BaseEntity:GetTransform_L()
    return self.transformSys:GetTransform_L(self.ent)
end

function EntityTypes.BaseEntity:GetTransform_W()
    return self.transformSys:GetTransform_W(self.ent)
end



function EntityTypes.BaseEntity:ForceUpdateTransformHierarchy()
    return self.transformSys:ForceUpdateHierarchy(self.ent)
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
