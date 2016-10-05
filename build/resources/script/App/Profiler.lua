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
    self.line_w = self.frame_rect.width
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
                focus_mode = GUI_FOCUS_MODE.ONTOP,
                background = {color = 'act_01',},               
            }),
        })
        offset_h = offset_h + line_h
        self.frame_rect:AttachChild( self.threads_lines[i].entity )
    end

    self.ids_btns = {}
    for i = 1, self.threads_count do self.ids_btns[i] = {} end
    for j = 1, self.ids_count do 
        local name = self.profiler:GetIDName(j - 1)
        local depth = self.profiler:GetIDDepth(j - 1)

        for i = 1, self.threads_count do
            self.ids_btns[i][j] = GuiButton({
                styles = {GuiStyles.perf_id,},
                text = {str = name},
                enable = false,
                height_percent = true,
                height = 100 - math.min(90, depth * 15),
                valign = GUI_VALIGN.BOTTOM,
            })
            self.threads_lines[i].entity:AttachChild( self.ids_btns[i][j].entity )
        end
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

    local frame_time = self.profiler:GetCurrentTimeSlice(0, 0)
    local total_time = frame_time.x + frame_time.y
    
    for i = 1, self.threads_count do
        for j = 1, self.ids_count do 
            local id_time = self.profiler:GetCurrentTimeSlice(j - 1, i - 1)
            if id_time.x ~= 0 or id_time.y ~= 0 then
                self.ids_btns[i][j].entity.left = self.line_w * math.min(1, id_time.x / total_time)
                self.ids_btns[i][j].entity.width = self.line_w * math.min(1, id_time.y / total_time)
                self.ids_btns[i][j].entity.enable = true
                self.ids_btns[i][j].entity:UpdatePosSize()
            end
        end
    end
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