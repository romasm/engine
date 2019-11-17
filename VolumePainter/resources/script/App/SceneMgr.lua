loader.require("App.VolumeWorld")

if not SceneMgr then SceneMgr = {} end

function SceneMgr:Init()
    print("SceneMgr:Init")  
	
    self.worlds = {}
    self.current_world = 0
end

function SceneMgr:IsWorld()
    return (#self.worlds > 0)
end

function SceneMgr:IsUnsave()
    if #self.worlds == 0 then return false end
    return self.worlds[self.current_world].unsave
end

-- one world at a time for now
function SceneMgr:GetWorld()
	return self.worlds[1]
end

function SceneMgr:Tick(dt)
	for i = 1, #self.worlds do
		self.worlds[i]:Tick(dt)
	end
end

function SceneMgr:LoadWorld(path)
	local newWorld = VolumeWorld(path)
	if not newWorld.coreWorld then return 0 end
    
    if #self.worlds > 0 then SceneMgr:CloseWorld(1) end
	return self:InitWorld(newWorld)
end

function SceneMgr:CreateWorld()
    if #self.worlds > 0 then SceneMgr:CloseWorld(1) end
	
	local newWorld = VolumeWorld(nil)
	if not newWorld.coreWorld then return 0 end

	return self:InitWorld(newWorld)
end

function SceneMgr:InitWorld(newWorld)      
	--Resource.ForceResourceReload()

	self.worlds[#self.worlds + 1] = newWorld
	self.current_world = #self.worlds

	Viewport:SetWorld(newWorld)

	return self.current_world
end

function SceneMgr:CloseWorld(id)
    if id <= 0 or id > #self.worlds then return end
        
	local world = self.worlds[id]
	world:Close()

	Viewport:ClearWorld()
    table.remove(self.worlds, id)
    self.current_world = #self.worlds
end

function SceneMgr:SaveWorld(id)
	if id <= 0 or id > #self.worlds then return end
	local hr = self.worlds[id]:SaveAs(nil)
end

function SceneMgr:SaveAsWorld(id, path)
    if id <= 0 or id > #self.worlds then return end
	local hr = self.worlds[id]:SaveAs(path)
    MainWindow:SetCaption(path)
end