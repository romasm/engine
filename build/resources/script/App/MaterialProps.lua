if not MaterialProps then MaterialProps = {} end

function MaterialProps.reloadMatWin()
    MaterialProps:Update()
end

loader.require("ComponentsGui.MaterialProps", MaterialProps.reloadMatWin)

function MaterialProps.reload()
    if MaterialProps.window then
        Tools.left_side_area.entity:DetachChild(MaterialProps.window.entity)
        MaterialProps.window.entity:Destroy()
        MaterialProps.window = nil
    end

    MaterialProps.window = Gui.AssetPropsWindow("Material")
    Tools.left_side_area.entity:AttachChild(MaterialProps.window.entity)
    
    Tools.left_side_area.first_win = MaterialProps.window.entity

    MaterialProps.window.entity:UpdatePosSize()

    local body = MaterialProps.window:GetBody().entity
    MaterialProps.body = body:GetInherited()
    MaterialProps.none_msg = MaterialProps.window.entity:GetChildById('none_msg')
    MaterialProps.preview = MaterialProps.window.entity:GetChildById('asset_viewport'):GetInherited()
    MaterialProps:Update()

    Tools.left_side_area.entity:UpdatePosSize()
end

function MaterialProps:Init()
    print("MaterialProps:Init") 

    loader.require("AssetProps", MaterialProps.reload)
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
        self.preview.entity.enable = false
        self.window:SetHeader("Material")

        AssetBrowser:PreviewMaterial(false)
        self.preview.rect_mat:ClearTextures()
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
    
    local matName = self.material:GetName()

    self.window:SetHeader("Material  " .. AssetBrowser:GetAssetNameFromPath( matName ))
    self.preview.entity.enable = true
    self.none_msg.enable = false

    local srv = AssetBrowser:PreviewMaterial(true, matName)
    self.preview.rect_mat:SetTexture(srv, 0, SHADERS.PS)
    
    local groups = Gui.MaterialProps()
    for i, gr in ipairs(groups) do
        self.body:AddGroup(gr)
    end

    self:UpdateData(true)
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

function MaterialProps:SetSelected(name, unsave)
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
        if unsave == nil then
            self.material:Save()                    -- TODO: preprocess for ensure that no redundant textures are saved
            AssetBrowser:GeneratePreview( self.material:GetName() )
        end
        Resource.DropMaterial( self.material:GetName() )
    end

    self.material = newMaterial
    self:Update()
end

function MaterialProps:GetSelected()
    return self.material
end