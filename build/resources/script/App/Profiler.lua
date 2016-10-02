if not Profiler then Profiler = {} end

function Profiler.reload()
    if Profiler.profiler_win ~= nil then
        local left = Profiler.profiler_win.sys_win:GetLeft()
        local top = Profiler.profiler_win.sys_win:GetTop()

        Profiler.profiler_win:Close()

        Profiler.profiler_win = Gui.ProfilerWindow()
        Profiler.profiler_win.sys_win:SetPos(left, top)
        Profiler.profiler_win.entity:UpdatePosSize()
    end
end

function Profiler:Init()
    print("Profiler:Init") 
    
    self.profiler = Util.GetProfiler()
    if self.profiler == nil then
        error("Profiler does not initialized!")
        return
    end

    loader.require("ProfilerWindow", Profiler.reload)
    
    self.profiler_win = Gui.ProfilerWindow()
    self.profiler_win.entity:UpdatePosSize() 

    self.update_time = 0
end

function Profiler:IsInit()
    return self.profiler_win ~= nil
end

function Profiler:Close()
    if self.IsInit() then 
        self.profiler_win:Close()
        self:SysClose()
    end
end

function Profiler:SysClose()
    self.profiler:Stop()
    self.profiler_win = nil
end

function Profiler:Tick(dt)
    if not Profiler:IsInit() then return end
    if not self.profiler:IsRunning() then return end

	self.update_time = self.update_time + dt

    if self.update_time < 500 then return end
    self.update_time = 0


end

function Profiler:SetDump(dump)
    if self.profiler == nil then return end
    self.profiler.dump = dump
end

function Profiler:Start()
    if self.profiler == nil then return end
    self.profiler:Start()
end

function Profiler:Stop()
    if self.profiler == nil then return end
    self.profiler:Stop()
end

function Profiler:SetRealtime(realtime)
    if not Profiler:IsInit() then return end


end