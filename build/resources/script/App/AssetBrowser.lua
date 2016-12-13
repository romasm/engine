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

    AssetBrowser:FillBody()
    AssetBrowser.window.entity:UpdatePosSize()
end

function AssetBrowser:ScanDir(matsDir)
    local dirList = FileIO.GetDirList(matsDir)
    for i = 1, dirList:size() do
        local name = dirList:getnext()

        local path = matsDir.."/"..name
        if FileIO.IsFile(path) then
            if path:find(".mtb") ~= nil then self.fileList[#self.fileList + 1] = path:gsub(".mtb", "") end
        else
            self:ScanDir(path)
        end
    end
    dirList:destruct()
end

function AssetBrowser:FillBody()
    local body = self.window.entity:GetChildById('body')

    local padding = 4
    local stringCounter = 0
    local topOffset = padding
    local leftOffset = padding

    local btn = 0

    for i, file in ipairs(self.fileList) do
        local matName = self.fileList[i]:gsub(self.libDir.."/", "")

        if stringCounter > 2 then
            topOffset = topOffset + btn.entity.height + padding
            leftOffset = padding
            stringCounter = 0
        end

        btn = Gui.AssetBrowserMaterial( self.fileList[i], matName, topOffset, leftOffset )

        body:AttachChild( btn.entity )

        leftOffset = leftOffset + btn.entity.width + padding
        stringCounter = stringCounter + 1
    end

    body.height = topOffset + btn.entity.height + padding
end

function AssetBrowser:Init()
    print("AssetBrowser:Init") 
    
    self.libDir = "../content/materials"
    self.fileList = {}
    self.selectedMatBtn = nil

    self:ScanDir(self.libDir)
    
    loader.require("AssetBrowser", AssetBrowser.reload)
    self.reload()
end

function AssetBrowser:SetSelected(btn)
    if self.selectedMatBtn then
        MaterialProps:SetSelected(nil)
        self.selectedMatBtn:SetPressed(false)
    end
    
    MaterialProps:SetSelected(btn.assetID .. ".mtb")
    self.selectedMatBtn = btn
end

function AssetBrowser:UnSelected()
    if self.selectedMatBtn then
        MaterialProps:SetSelected(nil)
        self.selectedMatBtn:SetPressed(false)
        self.selectedMatBtn = nil
    end
end

function AssetBrowser:Rename(textfield)
    local button = textfield.entity:GetParent():GetInherited()
end
