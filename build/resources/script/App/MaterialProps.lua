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

    self.previewMove = false
    self.previewPrevCoords = {x = 0, y = 0}
    self.previewZoom = 1.3

    self.previewRotSpeed = 0.005
    self.previewZoomSpeed = 0.1
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
    self.preview.rect_mat:SetShaderResourceByID(srv, 0, SHADERS.PS)
    
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

local CursorToCenter = function(viewport)
    local vp_rect = viewport.entity:GetRectAbsolute()
    local center = {
        x = vp_rect.l + vp_rect.w / 2,
        y = vp_rect.t + vp_rect.h / 2
    }
    CoreGui.SetCursorPos(viewport.entity, center.x, center.y)
    return center
end

function MaterialProps:ProcessPreviewMove(viewport, ev)
    if not self.previewMove then return end

    --local center = CursorToCenter(viewport)

    local delta = {
        x = ev.coords.x - self.previewPrevCoords.x,
        y = ev.coords.y - self.previewPrevCoords.y
    }
    
    self.previewPrevCoords.x = ev.coords.x
    self.previewPrevCoords.y = ev.coords.y

    local rotation = AssetBrowser.previewNode:GetRotationL()
    rotation.x = rotation.x + delta.y * self.previewRotSpeed * self.previewZoom
    rotation.y = rotation.y + delta.x * self.previewRotSpeed * self.previewZoom
    rotation.x = math.max( -math.pi * 0.4, math.min( rotation.x, math.pi * 0.4 ) )

    AssetBrowser.previewNode:SetRotation( rotation.x, rotation.y, rotation.z )
end

function MaterialProps:ProcessPreviewStartMove(viewport, ev)
    MaterialProps.previewMove = true 
    --CoreGui.ShowCursor(false)
    
    --CursorToCenter(viewport)
    self.previewPrevCoords.x = ev.coords.x
    self.previewPrevCoords.y = ev.coords.y
end

function MaterialProps:ProcessPreviewStopMove(viewport, ev)
    MaterialProps.previewMove = false 
    --CoreGui.ShowCursor(true)
end

function MaterialProps:ProcessPreviewZoom(viewport, ev)
    self.previewZoom = self.previewZoom + ev.coords.x * self.previewZoomSpeed
    self.previewZoom = math.max( 0.7, math.min( self.previewZoom, 2.3 ) )

    AssetBrowser.previewCamera:SetPosition( 0.0, 0.0, -self.previewZoom )
end

-- MATERIAL EDIT

function MaterialProps.StartColorPicking(colorPicker, shaderSlot, str)
    if colorPicker.picker then 
        colorPicker.picker = false
        return true
    else
        ColorPicker:Show(colorPicker, colorPicker.background.color, false)
        colorPicker.picker = true
    end

    colorPicker.history = {
        s_oldval = Vector4(0,0,0,0),
        s_newval = Vector4(0,0,0,0),
        undo = function(self) 
                MaterialProps.material:SetVector(self.s_oldval, shaderSlot, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetVector(self.s_newval, shaderSlot, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = str.. " color"
    }

    colorPicker.history.s_oldval = MaterialProps.material:GetVector(shaderSlot, SHADERS.PS)
    return true
end

function MaterialProps.ColorPicking(colorPicker, shaderSlot)
    local color = ColorPicker:GetColor()
    color.w = colorPicker.history.s_oldval.w
    MaterialProps.material:SetVector(color, shaderSlot, SHADERS.PS)
    return true
end

function MaterialProps.ColorPicked(colorPicker)
    if not ColorPicker:IsChanged() then return end
    colorPicker.history.s_newval.x = colorPicker.background.color.x
    colorPicker.history.s_newval.y = colorPicker.background.color.y
    colorPicker.history.s_newval.z = colorPicker.background.color.z
    colorPicker.history.s_newval.w = colorPicker.history.s_oldval.w
    History:Push(colorPicker.history)
    return true
end

function MaterialProps.UpdColor(colorPicker, shaderSlot)
    local val = MaterialProps.material:GetVector(shaderSlot, SHADERS.PS)
    colorPicker.background.color = Vector4(val.x, val.y, val.z, 1)
    colorPicker.background.color_hover = colorPicker.background.color
    colorPicker.background.color_press = colorPicker.background.color
    colorPicker:UpdateProps()
    return true
end

function MaterialProps:SetAlbedoTexture(name)
    self.material:SetTextureName(name, "albedoTexture", SHADERS.PS)
    self.material:SetFloat(name:len() > 0 and 1.0 or 0.0, "hasAlbedoTexture", SHADERS.PS)
end

function MaterialProps:GetAlbedoTexture()
    if self.material:GetFloat("hasAlbedoTexture", SHADERS.PS) == 0.0 then return "" end 
    return self.material:GetTextureName("albedoTexture", SHADERS.PS)
end

function MaterialProps:SetNormalTexture(name)
    self.material:SetTextureName(name, "normalTexture", SHADERS.PS)
    self.material:SetFloat(name:len() > 0 and 1.0 or 0.0, "hasNormalTexture", SHADERS.PS)
end

function MaterialProps:GetNormalTexture()
    if self.material:GetFloat("hasNormalTexture", SHADERS.PS) == 0.0 then return "" end 
    return self.material:GetTextureName("normalTexture", SHADERS.PS)
end

function MaterialProps:SetNormalSpace(isObject)
    self.material:SetFloat(isObject and 1.0 or 0.0, "objectSpaceNormalMap", SHADERS.PS)
end

-- if object -> true
function MaterialProps:GetNormalSpace()
    if self.material:GetFloat("objectSpaceNormalMap", SHADERS.PS) == 0.0 then return false
    else return true end 
end