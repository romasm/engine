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

    self.frame_rect = self.profiler_win.entity:GetChildById('frame_rect')

    self.ids_count = self.profiler:GetIDsCount()
    self.threads_count = self.profiler:GetThreadsCount()

    local line_h = self.frame_rect.height / self.threads_count
    local offset_h = 0

    self.threads_lines = {}
    for i = 1, self.threads_count do
        self.threads_lines[i] = GuiDumb({
            styles = {GuiStyles.live,},
            width_percent = true,
            width = 100,
            height = line_h,
            top = offset_h,

            GuiRect({
                styles = {GuiStyles.ghost,},
                width_percent = true,
                width = 100,
                height = 1,
                valign = GUI_VALIGN.BOTTOM,
                background = {color = 'act_01',},               
            }),
        })
        offset_h = offset_h + line_h
        self.frame_rect:AttachChild( self.threads_lines[i].entity )
    end

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