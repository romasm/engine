Main = {}

function Main:CopyToDeploy(path)
    return FileIO.Copy("..\\" .. path, self.deployPath .. path)
end

function Main:Start()	
    CoreGui.ConsoleWin.Clear()
    CoreGui.ConsoleWin.AllowClosing(true)
    CoreGui.ConsoleWin.SetTitle("Deploy")

    print("Deploy started")
    self.deployed = false
	
    local nullWin = HWND()
    self.deployPath = dlgOpenFolder(nullWin, "Deploy folder")
    if not FileIO.IsExist(self.deployPath) then 
        error("Path does not exist!")
        return 
    end

    local allOK = true

    local buildDir = "\\build_" .. os.date("%y_%m_%d_%H_%M_%S")
    self.deployPath = self.deployPath .. buildDir
    allOK = allOK and FileIO.CreateDir(self.deployPath)

    self.deployPath = self.deployPath .. "\\"
    
    allOK = allOK and self:CopyToDeploy("bin\\core.exe")
    allOK = allOK and self:CopyToDeploy("bin\\assimp-vc110-mt.dll")
    allOK = allOK and self:CopyToDeploy("bin\\d3dcompiler_46.dll")
    allOK = allOK and self:CopyToDeploy("bin\\lua51.dll")
    
    allOK = allOK and self:CopyToDeploy("config")

    allOK = allOK and FileIO.Copy("..\\launch_release.bat", self.deployPath .. "launch.bat")

    allOK = allOK and FileIO.CreateDir(self.deployPath .. "stats")

    allOK = allOK and FileIO.CopyByExt("..\\resources", self.deployPath .. "resources", ".dds")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.deployPath .. "resources", ".fnt")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.deployPath .. "resources", ".luac")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.deployPath .. "resources", ".mtb")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.deployPath .. "resources", ".msh")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.deployPath .. "resources", ".rsc")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.deployPath .. "resources", ".ico")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.deployPath .. "resources", ".tq")
    allOK = allOK and FileIO.CopyByExt("..\\resources", self.deployPath .. "resources", ".bc")
    
    allOK = allOK and FileIO.CreateDir(self.deployPath .. "content")
    allOK = allOK and FileIO.CopyByExt("..\\content\\environments", self.deployPath .. "content\\environments", ".mtb")
    allOK = allOK and FileIO.CopyByExt("..\\content\\environments", self.deployPath .. "content\\environments", ".dds")

    if not allOK then
        error("Errors during deployment occurred!")
        return
    end

    self.deployed = true
    print("Deploy finished")
end

function Main:onTick(dt) 
    if self.deployed then
        return true --false
    else
        return true
    end
end