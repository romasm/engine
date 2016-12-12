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

function AssetBrowser:Init()
    print("AssetBrowser:Init") 
    
    self.fileList = {}
    self.libDir = "../content/materials"

    self:ScanDir(self.libDir)

    -- temp
    print("---------")
    for i, file in ipairs(self.fileList) do
        print(file)
    end

    loader.require("AssetBrowser", Tools.reload)
    self.reload()
end