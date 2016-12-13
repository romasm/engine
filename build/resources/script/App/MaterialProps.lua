if not MaterialProps then MaterialProps = {} end

function MaterialProps.reloadMatWin()
    MaterialProps:Update()
end

loader.require("ShadersProps.opaque_main", MaterialProps.reloadMatWin)

function MaterialProps.reload()
    if MaterialProps.window then
        Tools.left_side_area.entity:DetachChild(MaterialProps.window.entity)
        MaterialProps.window.entity:Destroy()
        MaterialProps.window = nil
    end

    MaterialProps.window = Gui.MaterialWindow()
    Tools.left_side_area.entity:AttachChild(MaterialProps.window.entity)
    
    Tools.left_side_area.first_win = MaterialProps.window.entity

    MaterialProps.window.entity:UpdatePosSize()

    local body = MaterialProps.window.entity:GetChildById('body')
    MaterialProps.body = body:GetInherited()
    MaterialProps.none_msg = MaterialProps.window.entity:GetChildById('none_msg')
    MaterialProps:Update()
end

function MaterialProps:Init()
    print("MaterialProps:Init") 

    self.reload()

    self.material = nil

    self.update_time = 0
    self.update_need = false
end

function MaterialProps:Tick(dt)
    self.update_time = self.update_time + dt

    if self.update_time < COMMON_UPDATE_WAIT then return end
    self.update_time = 0
    if self.update_need then
        local ev = HEvent()
        ev.event = GUI_EVENTS.UPDATE
        self.body.entity:SendEvent(ev)                      

        self.update_need = false
    end
end

function MaterialProps:Update()
    self.body:ClearGroups()
    
    if not self.material then
        self.none_msg.enable = true
        return 
    end
    
    local shader_name = self.material:GetShaderName()
    while shader_name:len() > 0 do
        local name_start = shader_name:find("\\")
        if name_start == nil then name_start = shader_name:find("/") end
        if name_start == nil then
            break
        end            
        shader_name = shader_name:sub(name_start+1)
    end
    
    if Gui[shader_name.."Props"] == nil then return end
    
    local groups = Gui[shader_name.."Props"]()
    for i, gr in ipairs(groups) do
        self.body:AddGroup(gr)
    end

    self:UpdateData(true)
    self.none_msg.enable = false
end

function MaterialProps:UpdateData(force)
    if not force then
        self.update_need = true
        return
    end

    local ev = HEvent()
    ev.event = GUI_EVENTS.UPDATE
    self.body.entity:SendEvent(ev)

    self.update_time = 0
    self.update_need = false
end

function MaterialProps:SetSelected(name)
    local newMaterial = nil
    if name == nil then
        if self.material == nil then return end
        newMaterial = nil
    else
        if self.material then
            if name == self.material:GetName() then return end
        end
        newMaterial = Resource.GetMaterial( name )
    end

    if self.material then
        self.material:Save()
        Resource.DropMaterial( self.material:GetName() )
    end

    --[[local history = { -- MOVE TO STATIC AND LIBRARY
        s_oldval = self.material,
        s_newval = newMaterial,
        
        undo = function(self) 
                MaterialProps.material = self.s_oldval
                MaterialProps:Update()
                Properties:UpdateData(false, COMPONENTS.STATIC)
            end,
        redo = function(self) 
                MaterialProps.material = self.s_newval
                MaterialProps:Update()
                Properties:UpdateData(false, COMPONENTS.STATIC)
            end,
        msg = name and ("Select material " .. name) or "Unselect material"
    }
    --]]
    self.material = newMaterial
    --History:Push(history)

    self:Update()
end

function MaterialProps:GetSelected()
    return self.material
end