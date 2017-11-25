if not MaterialProps then MaterialProps = {} end

function MaterialProps.reloadMatWin()
    MaterialProps:Update()
end

loader.require("MaterialGui.Albedo", MaterialProps.reloadMatWin)
loader.require("MaterialGui.Normal", MaterialProps.reloadMatWin)
loader.require("MaterialGui.Roughness", MaterialProps.reloadMatWin)
loader.require("MaterialGui.Reflectivity", MaterialProps.reloadMatWin)
loader.require("MaterialGui.AO", MaterialProps.reloadMatWin)
loader.require("MaterialGui.Emissive", MaterialProps.reloadMatWin)
loader.require("MaterialGui.Scattering", MaterialProps.reloadMatWin)
loader.require("MaterialGui.Alphatest", MaterialProps.reloadMatWin)
loader.require("MaterialGui.Opacity", MaterialProps.reloadMatWin)
loader.require("MaterialGui.Transmittance", MaterialProps.reloadMatWin)
loader.require("MaterialGui.Thickness", MaterialProps.reloadMatWin)
loader.require("MaterialGui.MediumRoughness", MaterialProps.reloadMatWin)

loader.require("Menus.Material")

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
    self.components = {}
    self.hasProps = {
        albedo = false,
        roughness = false,
        reflectivity = false,
        normal = false,
        ao = false,
        emissive = false,
        transmittance = false,
        scattering = false,
        alphatest = false,
        opacity = false,
        thickness = false,
        medium_roughness = false,
    }

    self.scrollTo = nil

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
    if not self.material then
        self.none_msg.enable = true
        self.preview.entity.enable = false
        self.window:SetHeader("Material")

        AssetBrowser:PreviewMaterial(false)
        self.preview.rect_mat:ClearTextures()

        self:RecreateProps()
        return 
    end
        
    local matName = self.material:GetName()

    self.window:SetHeader("Material  " .. AssetBrowser:GetAssetNameFromPath( matName ))
    self.preview.entity.enable = true
    self.none_msg.enable = false

    local srv = AssetBrowser:PreviewMaterial(true, matName)
    self.preview.rect_mat:SetShaderResourceByID(srv, 0, SHADERS.PS)
    
    self:MarkProps()
    self:RecreateProps()
end

function MaterialProps:RecreateProps()
    self.components = {}
    self.body:ClearGroups()

    if not self.material then return end

    if self.hasProps.albedo then 
        self.components.albedo = Gui.MaterialAlbedo() 
        self.body:AddGroup(self.components.albedo)
    end
    if self.hasProps.roughness then 
        self.components.roughness = Gui.MaterialRoughness() 
        self.body:AddGroup(self.components.roughness)
    end
    if self.hasProps.reflectivity then 
        self.components.reflectivity = Gui.MaterialReflectivity() 
        self.body:AddGroup(self.components.reflectivity)
    end
    if self.hasProps.normal then 
        self.components.normal = Gui.MaterialNormal() 
        self.body:AddGroup(self.components.normal)
    end
    if self.hasProps.ao then 
        self.components.ao = Gui.MaterialAO() 
        self.body:AddGroup(self.components.ao)
    end
    if self.hasProps.emissive then 
        self.components.emissive = Gui.MaterialEmissive() 
        self.body:AddGroup(self.components.emissive)
    end
    if self.hasProps.opacity then 
        self.components.opacity = Gui.MaterialOpacity() 
        self.body:AddGroup(self.components.opacity)
    end
    if self.hasProps.transmittance then 
        self.components.transmittance = Gui.MaterialTransmittance() 
        self.body:AddGroup(self.components.transmittance)
    end
    if self.hasProps.scattering then 
        self.components.scattering = Gui.MaterialScattering() 
        self.body:AddGroup(self.components.scattering)
    end
    if self.hasProps.alphatest then 
        self.components.alphatest = Gui.MaterialAlphatest() 
        self.body:AddGroup(self.components.alphatest)
    end
    if self.hasProps.thickness then 
        self.components.thickness = Gui.MaterialThickness() 
        self.body:AddGroup(self.components.thickness)
    end
    if self.hasProps.medium_roughness then 
        self.components.medium_roughness = Gui.MaterialMediumRoughness() 
        self.body:AddGroup(self.components.medium_roughness)
    end

    self:UpdateData(true)

    if self.scrollTo ~= nil then
        local target = self.body.entity:GetChildById(self.scrollTo)
        if target:is_null() then return end

        self.window:SetScrollY( target.top )

        self.scrollTo = nil
    end
end

function MaterialProps:MarkProps()
    local albedoColor = self.material:GetVector("albedoColor", SHADERS.PS)
    if albedoColor.x + albedoColor.y + albedoColor.z == 0.0 then self.hasProps.albedo = false
    else self.hasProps.albedo = true end
    self.hasProps.albedo = self.hasProps.albedo or self.material:GetFloat("hasAlbedoTexture", SHADERS.PS) > 0

    self.hasProps.normal = self.material:GetFloat("hasNormalTexture", SHADERS.PS) > 0

    self.hasProps.roughness = (self.material:GetFloat("roughnessX", SHADERS.PS) + self.material:GetFloat("roughnessY", SHADERS.PS)) > 0
    self.hasProps.roughness = self.hasProps.roughness or self.material:GetFloat("hasRoughnessTexture", SHADERS.PS) > 0

    self.hasProps.reflectivity = self.material:GetFloat("hasReflectivityTexture", SHADERS.PS) > 0
    if not self.hasProps.reflectivity then 
        if self.material:GetFloat("isMetalPipeline", SHADERS.PS) > 0 then
            self.hasProps.reflectivity = self.material:GetFloat("metalnessValue", SHADERS.PS) > 0
        else
            local specColor = self.material:GetVector("reflectivityColor", SHADERS.PS)
            if not (specColor.x == 0.23 and specColor.y == 0.23 and specColor.z == 0.23) then self.hasProps.reflectivity = true end
        end
    end
    
    self.hasProps.ao = self.material:GetFloat("hasAOTexture", SHADERS.PS) > 0

    self.hasProps.emissive = self.material:GetFloat("emissiveIntensity", SHADERS.PS) > 0
    self.hasProps.emissive = self.hasProps.emissive or self.material:GetFloat("hasEmissiveTexture", SHADERS.PS) > 0
        
    self.hasProps.alphatest = self.material:GetShaderName() == "../resources/shaders/objects/alphatest_main"
    
    self.hasProps.opacity = self.material:GetShaderName() == "../resources/shaders/objects/transparent_medium"

    local sssColor = self.material:GetVector("subsurfaceColor", SHADERS.PS)
    if sssColor.x + sssColor.y + sssColor.z == 0.0 then self.hasProps.scattering = false
    else self.hasProps.scattering = true end
    self.hasProps.scattering = self.hasProps.scattering or self.material:GetFloat("hasSubsurfTexture", SHADERS.PS) > 0
    
    if not self.hasProps.opacity then
        self.hasProps.transmittance = false
        self.hasProps.medium_roughness = false
    else
        local ext = self.material:GetFloat("attenuationValue", SHADERS.PS)
        self.hasProps.transmittance = ext ~= 0.0
        self.hasProps.transmittance = self.hasProps.transmittance or self.material:GetFloat("hasAbsorptionTexture", SHADERS.PS) > 0
        self.hasProps.transmittance = self.hasProps.transmittance or self.material:GetFloat("invIorRed", SHADERS.PS) ~= 1.5
        self.hasProps.transmittance = self.hasProps.transmittance or self.material:GetFloat("invIorGreen", SHADERS.PS) ~= 1.5
        self.hasProps.transmittance = self.hasProps.transmittance or self.material:GetFloat("invIorBlue", SHADERS.PS) ~= 1.5
        self.hasProps.transmittance = self.hasProps.transmittance or self.material:GetFloat("tirAmount", SHADERS.PS) > 0.0

        self.hasProps.medium_roughness = self.material:GetFloat("hasInsideRoughnessTexture", SHADERS.PS) > 0
        self.hasProps.medium_roughness = self.hasProps.medium_roughness or self.material:GetFloat("insideRoughnessValue", SHADERS.PS) > 0
        
        self.hasProps.alphatest = self.material:GetFloat("hasAlphaTexture", SHADERS.PS) > 0
    end

    self.hasProps.thickness = self.material:GetFloat("hasThicknessTexture", SHADERS.PS) > 0 
    self.hasProps.thickness = self.hasProps.thickness or self.material:GetFloat("thicknessValue", SHADERS.PS) > 0
end

function MaterialProps:InitProp(propName)
    self:ZeroProp(propName)
    if propName == "albedo" then
        self.material:SetVector(Vector4(1,1,1,0), "albedoColor", SHADERS.PS)

    elseif propName == "emissive" then
        self.material:SetVector(Vector4(1,1,1,0), "emissiveColor", SHADERS.PS)
        self.material:SetFloat(1.0, "emissiveIntensity", SHADERS.PS)

    elseif propName == "scattering" then
        self.material:SetVector(Vector4(1,1,1,0), "subsurfaceColor", SHADERS.PS)

    elseif propName == "transmittance" then
        self.material:SetVector(Vector4(0,0,0,0), "subsurfaceColor", SHADERS.PS)
        self.material:SetFloat(15.0, "attenuationValue", SHADERS.PS)

    elseif propName == "thickness" then
        self.material:SetFloat(0.5, "thicknessValue", SHADERS.PS)     
           
    elseif propName == "medium_roughness" then
        self.material:SetFloat(0, "insideRoughnessValue", SHADERS.PS) 

    elseif propName == "alphatest" then
        if self.material:GetShaderName() ~= "../resources/shaders/objects/transparent_medium" then
            self.material:SetShader("../resources/shaders/objects/alphatest_main")
        end

    elseif propName == "opacity" then
        self.material:SetShader("../resources/shaders/objects/transparent_medium")
        self.material:SetFloat(0.1, "opacityValue", SHADERS.PS)
        self:ZeroProp("transmittance")
        self:ZeroProp("medium_roughness")
    end
end

function MaterialProps:ZeroProp(propName)
    if propName == "albedo" then
        self.material:SetVector(Vector4(0,0,0,0), "albedoColor", SHADERS.PS)
        self.material:SetFloat(0, "hasAlbedoTexture", SHADERS.PS)
        self.material:SetTextureName("", "albedoTexture", SHADERS.PS)

    elseif propName == "roughness" then
        self.material:SetFloat(0, "hasRoughnessTexture", SHADERS.PS)
        self.material:SetFloat(0, "roughnessAnisotropic", SHADERS.PS)
        self.material:SetFloat(0, "roughnessX", SHADERS.PS)
        self.material:SetFloat(0, "roughnessY", SHADERS.PS)
        self.material:SetTextureName("", "roughnessTexture", SHADERS.PS)

    elseif propName == "reflectivity" then
        self.material:SetFloat(1, "isMetalPipeline", SHADERS.PS)
        self.material:SetFloat(0, "metalnessValue", SHADERS.PS)
        self.material:SetFloat(0, "hasReflectivityTexture", SHADERS.PS)
        self.material:SetVector(Vector4(0.23,0.23,0.23,0), "reflectivityColor", SHADERS.PS)
        self.material:SetTextureName("", "reflectivityTexture", SHADERS.PS)

    elseif propName == "normal" then
        self.material:SetFloat(0, "hasNormalTexture", SHADERS.PS)
        self.material:SetFloat(0, "normalMapInvertY", SHADERS.PS)
        self.material:SetFloat(0, "objectSpaceNormalMap", SHADERS.PS)
        self.material:SetTextureName("", "normalTexture", SHADERS.PS)

    elseif propName == "ao" then
        self.material:SetFloat(0, "hasAOTexture", SHADERS.PS)
        self.material:SetFloat(1.0, "aoPower", SHADERS.PS)
        self.material:SetTextureName("", "aoTexture", SHADERS.PS)

    elseif propName == "emissive" then
        self.material:SetVector(Vector4(0,0,0,0), "emissiveColor", SHADERS.PS)
        self.material:SetFloat(0, "hasEmissiveTexture", SHADERS.PS)
        self.material:SetFloat(0, "emissiveIntensity", SHADERS.PS)
        self.material:SetTextureName("", "emissiveTexture", SHADERS.PS)

    elseif propName == "transmittance" then
        self.material:SetVector(Vector4(0,0,0,0), "absorptionColor", SHADERS.PS)
        self.material:SetFloat(0, "hasAbsorptionTexture", SHADERS.PS)
        self.material:SetTextureName("", "absorptionTexture", SHADERS.PS)
        self.material:SetFloat(0, "attenuationValue", SHADERS.PS)
        self.material:SetFloat(1.5, "invIorRed", SHADERS.PS)
        self.material:SetFloat(1.5, "invIorGreen", SHADERS.PS)
        self.material:SetFloat(1.5, "invIorBlue", SHADERS.PS)
        self.material:SetFloat(0.0, "iorAsSpecular", SHADERS.PS)
        self.material:SetFloat(0.0, "tirAmount", SHADERS.PS)

    elseif propName == "scattering" then
        self.material:SetVector(Vector4(0,0,0,0), "subsurfaceColor", SHADERS.PS)
        self.material:SetFloat(0, "hasSubsurfTexture", SHADERS.PS)
        self.material:SetTextureName("", "subsurfTexture", SHADERS.PS)

    elseif propName == "thickness" then
        self.material:SetFloat(0, "thicknessValue", SHADERS.PS) 
        self.material:SetFloat(0, "hasThicknessTexture", SHADERS.PS)
        self.material:SetTextureName("", "thicknessTexture", SHADERS.PS)

    elseif propName == "medium_roughness" then
        self.material:SetFloat(0, "insideRoughnessValue", SHADERS.PS)
        self.material:SetFloat(0, "hasInsideRoughnessTexture", SHADERS.PS)
        self.material:SetTextureName("", "insideRoughnessTexture", SHADERS.PS)

    elseif propName == "alphatest" then
        if self.material:GetShaderName() ~= "../resources/shaders/objects/transparent_medium" then
            self.material:SetShader("../resources/shaders/objects/opaque_main")
        end
        self.material:SetFloat(0.5, "alphaValue", SHADERS.PS)
        self.material:SetFloat(0, "hasAlphaTexture", SHADERS.PS)
        self.material:SetTextureName("", "alphaTexture", SHADERS.PS)

    elseif propName == "opacity" then
        self.material:SetShader("../resources/shaders/objects/opaque_main")
    end
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

    local delta = {
        x = ev.coords.x - self.previewPrevCoords.x,
        y = ev.coords.y - self.previewPrevCoords.y
    }
    
    self.previewPrevCoords.x = ev.coords.x
    self.previewPrevCoords.y = ev.coords.y

    local rotation = AssetBrowser.previewNode:GetRotation_L()
    rotation.x = rotation.x + delta.y * self.previewRotSpeed * self.previewZoom
    rotation.y = rotation.y + delta.x * self.previewRotSpeed * self.previewZoom
    rotation.x = math.max( -math.pi * 0.4, math.min( rotation.x, math.pi * 0.4 ) )

    AssetBrowser.previewNode:SetRotationPYR_L( rotation )
end

function MaterialProps:ProcessPreviewStartMove(viewport, ev)
    MaterialProps.previewMove = true 
    
    self.previewPrevCoords.x = ev.coords.x
    self.previewPrevCoords.y = ev.coords.y
end

function MaterialProps:ProcessPreviewStopMove(viewport, ev)
    MaterialProps.previewMove = false 
end

function MaterialProps:ProcessPreviewZoom(viewport, ev)
    self.previewZoom = self.previewZoom + ev.coords.x * self.previewZoomSpeed
    self.previewZoom = math.max( 0.7, math.min( self.previewZoom, 2.3 ) )

    AssetBrowser.previewCamera:SetPosition_L3F( 0.0, 0.0, -self.previewZoom )
end

-- MENU
function MaterialProps:OpenMenu(ent, coords)
    if self.material == nil then return end

    local menu = Gui.MaterialMenu()
    ent:AttachOverlay(menu)
    menu:Open(coords.x, coords.y)
end

function MaterialProps:MenuClick(id) -- TODO: to history
    if self.hasProps[id] == nil then return end

    self.hasProps[id] = not self.hasProps[id]
    if self.hasProps[id] == true then 
        self.scrollTo = id
        self:InitProp(id)
    else
        self:ZeroProp(id)
    end

    self:RecreateProps()
    return true
end

-- 2 SELECTOR
function MaterialProps.SetSelector(self, shaderSlot, str)
    local selected = self:GetSelected()

    local history = {
        s_oldval = false,
        s_newval = false,
        slot = shaderSlot,
        undo = function(self) 
                MaterialProps.material:SetFloat(self.s_oldval and 1.0 or 0.0, self.slot, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloat(self.s_newval and 1.0 or 0.0, self.slot, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = str
    }

    history.s_oldval = (MaterialProps.material:GetFloat(shaderSlot, SHADERS.PS) > 0.0)
    history.s_newval = (selected > 1)

    if history.s_oldval == history.s_newval then return true end

    history:redo()

    History:Push(history)
    return true
end

function MaterialProps.UpdSelector(self, shaderSlot)
    local space = (MaterialProps.material:GetFloat(shaderSlot, SHADERS.PS) > 0.0)
    if space == true then self:SetSelected(2)
    else self:SetSelected(1) end
    return true
end

-- TEXTURE
function MaterialProps.SetTexture(self, textureSlot, flagSlot, str)
    local texture = self:GetTexture()
    
    local history = {
        s_oldval = "",
        s_newval = "",
        slotT = textureSlot,
        slotF = flagSlot,
        undo = function(self)
                MaterialProps.material:SetTextureName(self.s_oldval, self.slotT, SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_oldval:len() > 0 and 1.0 or 0.0, self.slotF, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetTextureName(self.s_newval, self.slotT, SHADERS.PS)
                MaterialProps.material:SetFloat(self.s_newval:len() > 0 and 1.0 or 0.0, self.slotF, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = str .. " texture"
    }

    if MaterialProps.material:GetFloat(flagSlot, SHADERS.PS) == 0.0 then history.s_oldval = "" 
    else history.s_oldval = MaterialProps.material:GetTextureName(textureSlot, SHADERS.PS) end

    history.s_newval = texture
    
    history:redo()

    History:Push(history)
    return true
end

function MaterialProps.UpdTexture(self, textureSlot, flagSlot)
    if MaterialProps.material:GetFloat(flagSlot, SHADERS.PS) == 0.0 then 
        self:SetTexture( "" ) 
    else
        self:SetTexture( MaterialProps.material:GetTextureName(textureSlot, SHADERS.PS) ) 
    end 
    return true
end

-- COLOR PICKER
function MaterialProps.StartColorPicking(colorPicker, shaderSlot, str, allow_temperature)
    if colorPicker.picker then 
        colorPicker.picker = false
        return true
    else
        ColorPicker:Show(colorPicker, colorPicker.background.color, allow_temperature)
        colorPicker.picker = true
    end

    colorPicker.history = {
        s_oldval = Vector4(0,0,0,0),
        s_newval = Vector4(0,0,0,0),
        slot = shaderSlot,
        undo = function(self) 
                MaterialProps.material:SetVector(self.s_oldval, self.slot, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetVector(self.s_newval, self.slot, SHADERS.PS)
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

-- SLIDER
function MaterialProps.StartValue(self, shaderSlot, str)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        slot = shaderSlot,
        undo = function(self) 
                MaterialProps.material:SetFloat(self.s_oldval, self.slot, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetFloat(self.s_newval, self.slot, SHADERS.PS)
                MaterialProps:UpdateData(false)
            end,
        msg = str.. " value"
    }

    self.history.s_oldval = MaterialProps.material:GetFloat(shaderSlot, SHADERS.PS)
    MaterialProps.material:SetFloat(self:GetValue(), shaderSlot, SHADERS.PS)
    return true
end

function MaterialProps.DragValue(self, shaderSlot)
    MaterialProps.material:SetFloat(self:GetValue(), shaderSlot, SHADERS.PS)
    return true
end

function MaterialProps.EndValue(self, shaderSlot)
    self.history.s_newval = self:GetValue()
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    MaterialProps.material:SetFloat(self.history.s_newval, shaderSlot, SHADERS.PS)
    History:Push(self.history)
    return true
end

function MaterialProps.UpdValue(self, shaderSlot)
    self:SetValue( MaterialProps.material:GetFloat(shaderSlot, SHADERS.PS) )
    return true
end

-- SLIDER DEFFERED PARAM
function MaterialProps.StartDeffered(self, shaderSlot, str)
    self.history = {
        s_oldval = 0,
        s_newval = 0,
        slot = shaderSlot,
        undo = function(self) 
                MaterialProps.material:SetDefferedParam(self.s_oldval, self.slot)
                MaterialProps:UpdateData(false)
            end,
        redo = function(self) 
                MaterialProps.material:SetDefferedParam(self.s_newval, self.slot)
                MaterialProps:UpdateData(false)
            end,
        msg = str.. " value"
    }

    self.history.s_oldval = MaterialProps.material:GetDefferedParam(shaderSlot)
    MaterialProps.material:SetDefferedParam(self:GetValue(), shaderSlot)
    return true
end

function MaterialProps.DragDeffered(self, shaderSlot)
    MaterialProps.material:SetDefferedParam(self:GetValue(), shaderSlot)
    return true
end

function MaterialProps.EndDeffered(self, shaderSlot)
    self.history.s_newval = self:GetValue()
    if CMath.IsNearlyEq(self.history.s_oldval, self.history.s_newval, 0.001) then return true end
    MaterialProps.material:SetDefferedParam(self.history.s_newval, shaderSlot)
    History:Push(self.history)
    return true
end

function MaterialProps.UpdDeffered(self, shaderSlot)
    self:SetValue( MaterialProps.material:GetDefferedParam(shaderSlot) )
    return true
end