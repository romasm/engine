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
    self.filtredList = {}
    self.selectedMatBtn = nil
    self.findstr = ""
    self.nullMat = "../resources/materials/template_new.mtb"

    self:ScanDir(self.libDir)
    
    loader.require("AssetBrowser", AssetBrowser.reload)
    self.reload()

    self:InitPreviewWorld()

    self:Deactivate()
end

function AssetBrowser:Deactivate()
    self.window.entity:Deactivate()
end

function AssetBrowser:Activate()
    self.window.entity:Activate()
end

function AssetBrowser:ScanDir(matsDir)
    local dirList = FileIO.GetDirList(matsDir)
    for i = 1, dirList:size() do
        local name = dirList:getnext()

        local path = matsDir.."/"..name
        if FileIO.IsFile(path) then
            if path:find("%.mtb") ~= nil then 
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
    local matName = self.filtredList[num]:gsub(self.libDir.."/", "")

    if self.stringCounter > 2 then
        self.topOffset = self.topOffset + GUI_PREVIEW_SIZE.Y + self.padding
        self.leftOffset = self.padding
        self.stringCounter = 0
    end

    local btn = Gui.AssetBrowserMaterial( self.filtredList[num], matName, self.topOffset, self.leftOffset, num )

    self.body:AttachChild( btn.entity )

    self.leftOffset = self.leftOffset + btn.entity.width + self.padding
    self.stringCounter = self.stringCounter + 1
    
    return btn
end

function AssetBrowser:FillBody()
    self.padding = GUI_PREVIEW_SIZE.PADDING

    self.filtredList = {}
    for i, file in ipairs(self.fileList) do
        if self.findstr:len() > 0 then
            if file:find(self.findstr) ~= nil then 
                self.filtredList[#self.filtredList + 1] = file
            end
        else
            self.filtredList[#self.filtredList + 1] = file
        end
    end

    table.sort(self.filtredList, function (a, b) return a:upper() < b:upper() end)
    
    self.stringCounter = 0
    self.topOffset = self.padding
    self.leftOffset = self.padding

    for i, file in ipairs(self.filtredList) do
        self:AddButton(i)
    end

    Resource.ForceTextureReloadBackground()

    self.body.height = self.topOffset + GUI_PREVIEW_SIZE.Y + self.padding
end

function AssetBrowser:Clear()
    self:SetSelected(nil, false, false)

    for i = 1, #self.filtredList do
        local btn = self.body:GetChildById( tostring(i) )
        self.body:DetachChild(btn)
        btn:Destroy()
    end

    self.filtredList = {}

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

function AssetBrowser:SetSelectedByName(name, noScroll, noHistory)
    if not name or name:len() == 0 then
        self:SetSelected(nil, false, noHistory)
        return
    end

    local founded = 0
    for i, assetID in ipairs(self.filtredList) do
        if assetID == name then
            founded = i
            break
        end
    end

    if founded == 0 then return end

    local btn = self.body:GetChildById( tostring(founded) )
    if btn:is_null() then return end

    self:SetSelected(btn:GetInherited(), noScroll, noHistory)
end

function AssetBrowser:SetSelected(btn, noScroll, noHistory)
    local history = {        
        undo = function(self) 
                AssetBrowser:SetSelectedByName(self.s_oldval, false, true)
            end,
        redo = function(self)
                AssetBrowser:SetSelectedByName(self.s_newval, false, true)
            end,
    }
    
    if self.selectedMatBtn then
        history.s_oldval = self.selectedMatBtn.assetID

        MaterialProps:SetSelected(nil)
        self.selectedMatBtn:SetPressed(false)
    end

    if not btn then
        if not self.selectedMatBtn then return end

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
    
    if not noHistory then History:Push(history) end

    Properties:UpdateData(false, COMPONENTS.STATIC)
end

function AssetBrowser:Rename(textfield)
    local button = textfield.entity:GetParent():GetInherited()
    local newName = textfield:GetText()
        
    local newPath = self.libDir .. "/" .. newName
    local oldPath = button.assetID

    if oldPath == newPath then return end

    local selection = ""
    if self.selectedMatBtn then 
        selection = self.selectedMatBtn.assetID
        self:SetSelected(nil, true, true)
    end

        
    if selection == oldPath then
        selection = newPath
    end

    FileIO.Rename( oldPath..".tga", newPath..".tga" )
    FileIO.Rename( oldPath..".mtb", newPath..".mtb" )

    self:RenameInList(oldPath, newPath)
    
    if selection:len() > 0 then
        self:SetSelectedByName(selection, selection ~= newPath, true)
    end
end

function AssetBrowser:AddToList(assetID)
    self.fileList[#self.fileList + 1] = assetID

    self:Clear()
    self:FillBody()
    self.window.entity:UpdatePosSize()
end

function AssetBrowser:DeleteFromList(assetID)
    local founded = 0
    for i, file in ipairs(self.fileList) do
        if file == assetID then
            founded = i
            break 
        end
    end
    
    if founded == 0 then return end
    table.remove(self.fileList, founded)

    self:Clear()
    self:FillBody()
    self.window.entity:UpdatePosSize()
end

function AssetBrowser:RenameInList(oldAssetID, newAssetID)
    local founded = 0
    for i, file in ipairs(self.fileList) do
        if file == oldAssetID then
            founded = i
            break 
        end
    end
    
    if founded == 0 then return end
    self.fileList[founded] = newAssetID

    self:Clear()
    self:FillBody()
    self.window.entity:UpdatePosSize()
end

function AssetBrowser:CreateNew()
    local newAssetID = self.libDir .. "/Material"
    local newCounter = 0
    
    while FileIO.IsExist( newAssetID .. tostring(newCounter) .. ".mtb" ) do newCounter = newCounter + 1 end
    newAssetID = newAssetID .. tostring(newCounter)

    FileIO.Copy(self.nullMat, newAssetID..".mtb")
    
    self:GeneratePreview(newAssetID..".mtb")

    self:AddToList(newAssetID)
    self:SetSelectedByName(newAssetID, false, false)
    if self.selectedMatBtn then
        self.selectedMatBtn:SetPressed(true)
    end
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
    FileIO.Delete( deleteAssetID ..".mtb" )
    FileIO.Delete( deleteAssetID ..".tga" )

    self.copyBtn:Deactivate()
    self.deleteBtn:Deactivate()
    
    self:DeleteFromList(deleteAssetID)
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

    self:AddToList(newAssetID)
    self:SetSelectedByName(newAssetID, false, false)
    if self.selectedMatBtn then
        self.selectedMatBtn:SetPressed(true)
    end
end

function AssetBrowser:Find(str)
    self.findstr = CStr.MakeFindMask(str)

    self:Clear()
    self:FillBody()
    self.window.entity:UpdatePosSize()
end

function AssetBrowser:InitPreviewWorld()
    width = GUI_PREVIEW_SIZE.X * 2
    height = GUI_PREVIEW_SIZE.Y * 2

    local worldmgr = GetWorldMgr()
    
    self.previewWorld = worldmgr:CreateSmallWorld()
    -- TODO: set sky
    
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
    self.screenshotSphere:Enable(false)

    -- for preview   
    self.previewNode = EntityTypes.Node(self.previewWorld)
    self.previewNode:SetPosition(100.0, 0.0, 0.0)
     
    self.previewCamera = EntityTypes.Camera(self.previewWorld)
    self.previewCamera:SetPosition(0.0, 0.0, -1.3)
    self.previewCamera:Attach(self.previewNode)
    self.previewCamera:SetFov(0.5)
    
    self.previewScene = self.previewWorld:CreateScene(self.previewCamera.ent, GUI_PREVIEW_SIZE.LIVE_X, GUI_PREVIEW_SIZE.LIVE_Y, true)
    self.previewScene:SetExposure(false, 0.2)
    
    self.previewSphere = EntityTypes.StaticModel(self.previewWorld)
    self.previewSphere:SetMesh("../resources/meshes/mat_sphere.stm")
    self.previewSphere:SetPosition(100.0, 0.0, 0.0)

    self.previewWorld.active = false
end

function AssetBrowser:GeneratePreview(filename)  
    if not self.previewWorld or self.waitingForPreview then 
        return 
    end

    self.waitingForPreview = true
    
    self.screenshotCamera:Activate(self.screenshotScene)
    self.screenshotSphere:Enable(true)
    self.screenshotSphere:SetMaterial(filename, 0)

    Resource.ForceTextureReloadBackground() -- TODO: use job callback system
    self:PostGeneratePreview(filename:gsub("%.mtb", "%.tga"))
end

function AssetBrowser:PostGeneratePreview(filename)
    self.previewWorld:Snapshot(self.screenshotScene)
    self.screenshotScene:SaveScreenshot(filename, 2.0, 2.0)

    self.screenshotSphere:SetMaterial(self.nullMat, 0)
    self.screenshotSphere:Enable(false)
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