if not AssetBrowser then AssetBrowser = {} end

function AssetBrowser.reload()
    if AssetBrowser.window then
        Tools.left_side_area.entity:DetachChild(AssetBrowser.window.entity)
        AssetBrowser.window.entity:Destroy()
        AssetBrowser.window = nil
    end

    AssetBrowser.window = Gui.AssetBrowserWindow()
    Tools.left_side_area.entity:AttachChild(AssetBrowser.window.entity)
    
    Tools.left_side_area.second_win = AssetBrowser.window.entity
    
    AssetBrowser.body = AssetBrowser.window:GetBody().entity
    AssetBrowser.copyBtn = AssetBrowser.window.entity:GetChildById('copy_btn')
    AssetBrowser.deleteBtn = AssetBrowser.window.entity:GetChildById('delete_btn')

    AssetBrowser:FillBody()
    AssetBrowser.window.entity:UpdatePosSize()

    Tools.left_side_area.entity:UpdatePosSize()

    AssetBrowser.copyBtn:Deactivate()
    AssetBrowser.deleteBtn:Deactivate()
end

function AssetBrowser:Init()
    print("AssetBrowser:Init") 
    
    self.libDir = "../content/materials"
    self.fileList = {}
    self.selectedMatBtn = nil
    self.findstr = ""
    self.nullMat = "../resources/materials/template_new.mtb"

    self:ScanDir(self.libDir)
    
    loader.require("AssetBrowser", AssetBrowser.reload)
    self.reload()

    self:InitPreviewWorld()
end

function AssetBrowser:ScanDir(matsDir)
    local dirList = FileIO.GetDirList(matsDir)
    for i = 1, dirList:size() do
        local name = dirList:getnext()

        local skip = false
        if self.findstr:len() > 0 then
            if name:gsub("%.mtb", ""):find(self.findstr) == nil then 
                skip = true
            end
        end

        local path = matsDir.."/"..name
        if FileIO.IsFile(path) then
            if path:find("%.mtb") ~= nil and skip == false then 
                self.fileList[#self.fileList + 1] = path:gsub("%.mtb", "") 
            end
        else
            if not name:find("editor") then  -- TEMP
                self:ScanDir(path)
            end
        end
    end
    dirList:destruct()
end

function AssetBrowser:AddButton(num)
    local matName = self.fileList[num]:gsub(self.libDir.."/", "")

    if self.stringCounter > 2 then
        self.topOffset = self.topOffset + self.lastHeight + self.padding
        self.leftOffset = self.padding
        self.stringCounter = 0
    end

    local btn = Gui.AssetBrowserMaterial( self.fileList[num], matName, self.topOffset, self.leftOffset, num )

    self.body:AttachChild( btn.entity )

    self.leftOffset = self.leftOffset + btn.entity.width + self.padding
    self.stringCounter = self.stringCounter + 1

    self.lastHeight = btn.entity.height

    return btn
end

function AssetBrowser:FillBody()
    self.padding = GUI_PREVIEW_SIZE.PADDING

    self.stringCounter = 0
    self.topOffset = self.padding
    self.leftOffset = self.padding

    self.lastHeight = 0
    for i, file in ipairs(self.fileList) do
        self:AddButton(i)
    end

    Resource.ForceTextureReloadBackground()

    self.body.height = self.topOffset + self.lastHeight + self.padding
end

function AssetBrowser:Clear()
    self:SetSelected(nil, false, false)

    for i = 1, #self.fileList do
        local btn = self.body:GetChildById( tostring(i) )
        self.body:DetachChild(btn)
        btn:Destroy()
    end

    self.fileList = {}

    self.stringCounter = 0
    self.topOffset = self.padding
    self.leftOffset = self.padding
end

function AssetBrowser:GetSelectedAssetName()
    if not self.selectedMatBtn then
        return nil
    end
    
    return self.selectedMatBtn.assetID:gsub(self.libDir .. "/", "")
end

function AssetBrowser:GetPathFromAssetName(name)
    return self.libDir .. "/" .. name .. ".mtb"
end

function AssetBrowser:GetAssetNameFromPath(path)
    local name = path:gsub(self.libDir .. "/", "")
    name = name:gsub("%.mtb", "")
    return name
end

function AssetBrowser:SetSelectedByName(name, noHistory)
    if not name or name:len() == 0 then
        self:SetSelected(nil, false, noHistory)
        return
    end

    local founded = 0
    for i, assetID in ipairs(self.fileList) do
        if assetID == name then
            founded = i
            break
        end
    end

    if founded == 0 then return end

    local btn = self.body:GetChildById( tostring(founded) )
    if btn:is_null() then return end

    self:SetSelected(btn:GetInherited(), false, noHistory)
end

function AssetBrowser:SetSelected(btn, noScroll, noHistory)
    local history = {        
        undo = function(self) 
                AssetBrowser:SetSelectedByName(self.s_oldval, true)
            end,
        redo = function(self)
                AssetBrowser:SetSelectedByName(self.s_newval, true)
            end,
    }
    
    if self.selectedMatBtn then
        history.s_oldval = self.selectedMatBtn.assetID

        MaterialProps:SetSelected(nil)
        self.selectedMatBtn:SetPressed(false)
    end

    if not btn then
        self.selectedMatBtn = nil
        self.copyBtn:Deactivate()
        self.deleteBtn:Deactivate()
        
        history.msg = "Unselect material"
        history.s_newval = nil
        if not noHistory then History:Push(history) end

        Properties:UpdateData(false, COMPONENTS.STATIC)
        return 
    end
    
    MaterialProps:SetSelected(btn.assetID .. ".mtb")
    self.selectedMatBtn = btn
    self.selectedMatBtn:SetPressed(true)

    if not noScroll then
        self.window:SetScrollY( self.selectedMatBtn.entity.top )
    end

    self.copyBtn:Activate()
    self.deleteBtn:Activate()
    
    history.msg = "Select material " .. btn.assetID
    history.s_newval = btn.assetID
    print("eee" .. history.msg)
    if not noHistory then History:Push(history) end

    Properties:UpdateData(false, COMPONENTS.STATIC)
end

function AssetBrowser:Rename(textfield)
    local button = textfield.entity:GetParent():GetInherited()
    local newName = textfield:GetText()
    
    local newPath = self.libDir .. "/" .. newName

    if button.assetID == newPath then return end

    textfield.alt = newName
    button.alt = newName

    for i, filePath in ipairs(self.fileList) do
        if filePath == button.assetID then
            self.fileList[i] = newPath
            break
        end
    end

    FileIO.Rename( button.assetID..".tga", newPath..".tga" )
    FileIO.Rename( button.assetID..".mtb", newPath..".mtb" )

    button.icon_mat:SetTextureByName(newPath .. ".tga", 0, 0)

    button.assetID = newPath

    if self.selectedMatBtn == button then
        MaterialProps:SetSelected(nil, true)
        MaterialProps:SetSelected(button.assetID .. ".mtb")
    end
end

function AssetBrowser:AddToList(assetID)
    self.fileList[#self.fileList + 1] = assetID

    local btn = self:AddButton( #self.fileList )
    self.body.height = self.topOffset + self.lastHeight + self.padding

    self.window.entity:UpdatePosSize()
    return btn
end

function AssetBrowser:CreateNew()
    local newAssetID = self.libDir .. "/Material"
    local newCounter = 0
    
    while FileIO.IsExist( newAssetID .. tostring(newCounter) .. ".mtb" ) do newCounter = newCounter + 1 end
    newAssetID = newAssetID .. tostring(newCounter)

    FileIO.Copy(self.nullMat, newAssetID..".mtb")
    
    self:GeneratePreview(newAssetID..".mtb")

    local btn = self:AddToList(newAssetID)
    self:SetSelected(btn, false, false)
    self.selectedMatBtn:SetPressed(true)
end

function AssetBrowser:DeleteSelected()
    if self.selectedMatBtn == nil then return end

    MaterialProps:SetSelected(nil, true)
    self.selectedMatBtn:SetPressed(false)

    local assetID = self.selectedMatBtn.assetID
    self.selectedMatBtn = nil

    AssetBrowser:Delete(assetID)
end

function AssetBrowser:Delete(deleteAssetID)
    self:Clear()
    
    FileIO.Delete( deleteAssetID ..".mtb" )
    FileIO.Delete( deleteAssetID ..".tga" )

    self.copyBtn:Deactivate()
    self.deleteBtn:Deactivate()
    
    self:ScanDir(self.libDir)
    self:FillBody()
    self.window.entity:UpdatePosSize()
end

function AssetBrowser:CopySelected()
    if self.selectedMatBtn == nil then return end
    AssetBrowser:Copy(self.selectedMatBtn.assetID)
end

function AssetBrowser:Copy(copyAssetID)
    local newAssetID = copyAssetID .. "_copy"
    local newCounter = 0
    
    while FileIO.IsExist( newAssetID .. tostring(newCounter) .. ".mtb" ) do newCounter = newCounter + 1 end
    newAssetID = newAssetID .. tostring(newCounter)

    FileIO.Copy( copyAssetID ..".mtb", newAssetID ..".mtb" )
    
    self:GeneratePreview(newAssetID..".mtb")

    local btn = self:AddToList(newAssetID)
    self:SetSelected(btn, false, false)
    self.selectedMatBtn:SetPressed(true)
end

function AssetBrowser:Find(str)
    self.findstr = str

    self.findstr = self.findstr:gsub("%.", "%%%.")
    self.findstr = self.findstr:gsub("%+", "%%%+")
    self.findstr = self.findstr:gsub("%-", "%%%-")
    self.findstr = self.findstr:gsub("%(", "%%%(")
    self.findstr = self.findstr:gsub("%)", "%%%)")

    self.findstr = self.findstr:gsub("%?", "%.%?")
    self.findstr = self.findstr:gsub("%*", "%.%+")

    self:Clear()
    
    self:ScanDir(self.libDir)
    self:FillBody()
    self.window.entity:UpdatePosSize()
end

function AssetBrowser:InitPreviewWorld()
    width = GUI_PREVIEW_SIZE.X * 2
    height = GUI_PREVIEW_SIZE.Y * 2

    local worldmgr = GetWorldMgr()
    
    self.previewWorld = worldmgr:CreateSmallWorld()
    
    if not self.previewWorld then 
        error("Cant generate preview image for material " .. filename)
        return
    end
    
    -- for screenshots    
    self.screenshotCamera = EntityTypes.Camera(self.previewWorld)
    self.screenshotCamera:SetPosition(0.0, 0.0, -4.3)
    self.screenshotCamera:SetFov(0.25)
    self.screenshotCamera:SetFar(100.0)
    
    self.screenshotScene = self.previewWorld:CreateScene(self.screenshotCamera.ent, width * 2, height * 2, true)
    self.screenshotScene:SetExposure(false, 0.2)

    self.screenshotCamera:Deactivate(self.screenshotScene)

    self.screenshotSphere = EntityTypes.StaticModel(self.previewWorld)
    self.screenshotSphere:SetMesh("../resources/meshes/mat_sphere.stm")

    -- for preview    
    self.previewCamera = EntityTypes.Camera(self.previewWorld)
    self.previewCamera:SetPosition(100.0, 0.0, -4.3)
    self.previewCamera:SetFov(0.15)
    
    self.previewScene = self.previewWorld:CreateScene(self.previewCamera.ent, 284, 112, true)
    self.previewScene:SetExposure(false, 0.2)
    
    self.previewSphere = EntityTypes.StaticModel(self.previewWorld)
    self.previewSphere:SetMesh("../resources/meshes/mat_sphere.stm")
    self.previewSphere:SetPosition(100.0, 0.0, 0.0)

    self.previewWorld.active = false
end

-- NOT NEED?
--[[function AssetBrowser:ClosePreviewWorld() 
    local worldmgr = GetWorldMgr()
    worldmgr:CloseWorld(self.previewWorld)
end--]]

function AssetBrowser:GeneratePreview(filename)  
    if not self.previewWorld or self.waitingForPreview then 
        return 
    end

    self.waitingForPreview = true
    
    self.screenshotCamera:Activate(self.screenshotScene)
    self.screenshotSphere:SetMaterial(filename, 0)

    Resource.ForceTextureReloadBackground() -- TODO: use job callback system
    self:PostGeneratePreview(filename:gsub("%.mtb", "%.tga"))
end

function AssetBrowser:PostGeneratePreview(filename)
    self.previewWorld:Snapshot(self.screenshotScene)
    self.screenshotScene:SaveScreenshot(filename, 2.0, 2.0)

    self.screenshotSphere:SetMaterial(self.nullMat, 0)
    self.screenshotCamera:Deactivate(self.screenshotScene)

    self.waitingForPreview = false

    print("Material preview " .. filename .. " generated")
end

function AssetBrowser:PreviewMaterial(live, path)
    if not self.previewWorld then return nil end
    self.previewWorld.active = live
    if live then
        self.previewSphere:SetMaterial(path, 0)
        return self.previewScene:GetSRV()
    else
        self.previewSphere:SetMaterial(self.nullMat, 0)
        return nil
    end   
end