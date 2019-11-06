Main = {}

function Main:CopyToBuild(path)
    return FileIO.Copy("..\\" .. path, self.bakePath .. path)
end

function Main:Start()	
    CoreGui.ConsoleWin.Clear()
    CoreGui.ConsoleWin.AllowClosing(true)
    CoreGui.ConsoleWin.SetTitle("Baking")

    print("Baking started")
    self.baked = false
	
    local nullWin = HWND()
    self.bakePath = dlgOpenFolder(nullWin, "Baking folder")
    if not FileIO.IsExist(self.bakePath) then 
        error("Path does not exist!")
        return 
    end

    local allOK = true

    local buildDir = "\\build_" .. os.date("%y_%m_%d_%H_%M_%S")
    self.bakePath = self.bakePath .. buildDir
    allOK = allOK and FileIO.CreateDir(self.bakePath)

    self.bakePath = self.bakePath .. "\\"
    
    allOK = allOK and self:CopyToBuild("bin\\core.exe")
    allOK = allOK and self:CopyToBuild("bin\\assimp-vc110-mt.dll")
    allOK = allOK and self:CopyToBuild("bin\\d3dcompiler_46.dll")
    allOK = allOK and self:CopyToBuild("bin\\lua51.dll")
    
    allOK = allOK and self:CopyToBuild("config")

    allOK = allOK and FileIO.Copy("..\\launch_release.bat", self.bakePath .. "launch.bat")

    allOK = allOK and FileIO.CreateDir(self.bakePath .. "stats")

    allOK = allOK and FileIO.CopyByExt("..\\resources", self.bakePath .. "resources", ".dds")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.bakePath .. "resources", ".fnt")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.bakePath .. "resources", ".luac")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.bakePath .. "resources", ".mtb")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.bakePath .. "resources", ".msh")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.bakePath .. "resources", ".rsc")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.bakePath .. "resources", ".ico")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.bakePath .. "resources", ".tq")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.bakePath .. "resources", ".bc")
    
    allOK = allOK and FileIO.CreateDir(self.bakePath .. "content")
    allOK = allOK and FileIO.CopyByExt("..\\content\\environments", self.bakePath .. "content\\environments", ".mtb")
    allOK = allOK and FileIO.CopyByExt("..\\content\\environments", self.bakePath .. "content\\environments", ".dds")

    if not allOK then
        error("Errors during baking occurred!")
        return
    end

    self.baked = true
    print("Baking finished")
end

function Main:onTick(dt) 
    if self.baked then
        return true --false
    else
        return true
    end
end