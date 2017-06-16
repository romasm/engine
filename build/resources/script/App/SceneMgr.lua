if not SceneMgr then SceneMgr = {} end

function SceneMgr:Init()
    print("SceneMgr:Init")  

    self.worldmgr = GetWorldMgr()

    self.worlds = {}
    self.current_world = 0
end

function SceneMgr:IsWorld()
    return #self.worlds > 0
end

function SceneMgr:IsUnsave()
    if #self.worlds == 0 then return false end
    return self.worlds[self.current_world].unsave
end

-- one world at a time for now

function SceneMgr:LoadWorld(path)
    local newworld = {}
    newworld.world = self.worldmgr:OpenWorld(path)
    if newworld.world == nil then
        Gui.DialogOk( MainWindow.mainWinRoot.entity, "Scene file version is outdated!" )
        return 0
    end
    
    newworld.path = path
    
    if not newworld.world then return 0 end
    newworld.unsave = false 
    
    if #self.worlds > 0 then SceneMgr:CloseWorld(1) end
    return self:InitWorld(newworld)
end

function SceneMgr:CreateWorld()
    if #self.worlds > 0 then SceneMgr:CloseWorld(1) end

    local newworld = {}
    newworld.world = self.worldmgr:CreateWorld()
    newworld.path = ""

    if not newworld.world then return 0 end
	newworld.unsave = true

    return self:InitWorld(newworld)
end

function SceneMgr:InitWorld(WLD)      
    --Resource.ForceResourceReload()

    Viewport:SetWorld(WLD)

    self.worlds[#self.worlds + 1] = WLD
    self.current_world = #self.worlds
    return self.current_world
end

function SceneMgr:CloseWorld(id)
    if id <= 0 or id > #self.worlds then return end
        
    local WLD = self.worlds[id]
    self.worldmgr:CloseWorld(WLD.world)
    WLD.world = nil

    WLD.scenepl = nil
    
    Viewport:ClearWorld()

    table.remove(self.worlds, id)

    self.current_world = #self.worlds
end

function SceneMgr:SaveWorld(id)
    if id <= 0 or id > #self.worlds then return end
    if self.worlds[id].path:len() == 0 then return end
    
	if self.worlds[id].unsave == false then return end
    self.worlds[id].world:Save(self.worlds[id].path)

    self.worlds[id].unsave = false
end

function SceneMgr:SaveAsWorld(id, path)
    if id <= 0 or id > #self.worlds then return end
    self.worlds[id].world:Save(path)
    self.worlds[id].path = path

    MainWindow:SetCaption(path)

    self.worlds[id].unsave = false
end