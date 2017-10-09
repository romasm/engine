if not Profiler then Profiler = {} end

function Profiler.reload()
    if Profiler.profiler_win ~= nil then
        local left = Profiler.profiler_win.sys_win:GetLeft()
        local top = Profiler.profiler_win.sys_win:GetTop()

        Profiler.profiler_win:Close()

        Profiler.profiler:Start()
        
        Profiler.profiler_win = Gui.ProfilerWindow()

        Profiler:FillWindow()
        Profiler:SetStatsParam(nil)

        Profiler.profiler_win.sys_win:SetPos(left, top)
        Profiler.profiler_win.entity:UpdatePosSize()
    end
end

function Profiler:FillWindow()
    local realtime_check = self.profiler_win.entity:GetChildById('isrealtime')
    realtime_check:GetInherited():SetCheck(self.realtime)

    local stats_dumb = self.profiler_win.entity:GetChildById('param_stats')
    self.stats_cur_ms = stats_dumb:GetChildById('cur_ms'):GetInherited()
    self.stats_avg_ms = stats_dumb:GetChildById('avg_ms'):GetInherited()
    self.stats_max_ms = stats_dumb:GetChildById('max_ms'):GetInherited()
    self.stats_cur_prc = stats_dumb:GetChildById('cur_prc'):GetInherited()
    self.stats_avg_prc = stats_dumb:GetChildById('avg_prc'):GetInherited()
    self.stats_max_prc = stats_dumb:GetChildById('max_prc'):GetInherited()

    local stats_common_dumb = self.profiler_win.entity:GetChildById('common_stats')
    self.stats_frame_ms = stats_common_dumb:GetChildById('frame_ms'):GetInherited()
    self.stats_frame_fps = stats_common_dumb:GetChildById('frame_fps'):GetInherited()
    self.stats_cpuframe = stats_common_dumb:GetChildById('cpu_frame_ms'):GetInherited()
    self.stats_gpuframe = stats_common_dumb:GetChildById('gpu_frame_ms'):GetInherited()

    self.frame_rect = self.profiler_win.entity:GetChildById('frame_rect')

    self.ids_count = self.profiler:GetIDsCount()
    self.threads_count = self.profiler:GetThreadsCount()

    local line_h = self.frame_rect.height / self.threads_count
    self.line_w = self.frame_rect.width
    local offset_h = 0

    local body = self.profiler_win:GetBody()
    local thread_name_left = self.frame_rect.left - 40 
    local thread_name_top = self.frame_rect.top + line_h / 2 - 11

    self.threads_lines = {}
    for i = 1, self.threads_count do
        
        local str = GuiString({
            styles = {GuiStyles.bg_str_normal,},
            str = i == 1 and "Main" or "Th "..tostring(i - 1),
            top = thread_name_top,
            left = thread_name_left,
        })

        body.entity:AttachChild( str.entity )
        thread_name_top = thread_name_top + line_h

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
                enable = i ~= self.threads_count,             
            }),
        })
        offset_h = offset_h + line_h
        self.frame_rect:AttachChild( self.threads_lines[i].entity )
    end

    self.ids_btns = {}
    for i = 1, self.threads_count do self.ids_btns[i] = {} end
    for j = 1, self.ids_count do 
        local param_id = j - 1
        local name = self.profiler:GetIDName(param_id)
        local depth = self.profiler:GetIDDepth(param_id)

        local id_color = nil
        local color_lerp = depth / 2.0
        if color_lerp > 2 then
            color_lerp = math.min(1.0, color_lerp - 2)
            id_color = CMath.ColorLerp( Vector4(1.0, 0.0, 0.0, 0.4), Vector4(1.0, 0.0, 1.0, 0.4), color_lerp)
        elseif color_lerp > 1 then 
            color_lerp = math.min(1.0, color_lerp - 1)
            id_color = CMath.ColorLerp( Vector4(1.0, 1.0, 0.0, 0.4), Vector4(1.0, 0.0, 0.0, 0.4), color_lerp)
        else
            id_color = CMath.ColorLerp( Vector4(0.0, 1.0, 0.0, 0.4), Vector4(1.0, 1.0, 0.0, 0.4), color_lerp)
        end

        for i = 1, self.threads_count do
            local thread_id = i - 1
            self.ids_btns[i][j] = GuiButton({
                styles = {GuiStyles.perf_id,},
                background = {color = id_color,},
                text = {str = name},
                enable = false,
                height_percent = true,
                height = 90 - math.min(80, depth * 15),
                bottom = 5,
                bottom_percent = true,
                valign = GUI_VALIGN.BOTTOM,
                id = tostring(param_id).."_"..tostring(thread_id),
                alt = "Detail view of "..name,
                events = {
                    [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return Profiler:SetStatsParam(self, false) end,
                    [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) return Profiler:SetStatsParam(nil, false) end,},
            })
            self.threads_lines[i].entity:AttachChild( self.ids_btns[i][j].entity )
        end
    end

    self.gru_rect = self.profiler_win.entity:GetChildById('gru_rect')
    self.gpu_ids_count = self.profiler:GetGpuIDsCount()
    
    self.gpu_ids_btns = {}
    for j = 1, self.gpu_ids_count do 
        local param_id = j - 1
        local name = self.profiler:GetGpuIDName(param_id)
        local depth = self.profiler:GetGpuIDDepth(param_id)

        local id_color = nil
        local color_lerp = depth / 2.0
        if color_lerp > 2 then
            color_lerp = math.min(1.0, color_lerp - 2)
            id_color = CMath.ColorLerp( Vector4(1.0, 0.0, 0.0, 0.4), Vector4(1.0, 0.0, 1.0, 0.4), color_lerp)
        elseif color_lerp > 1 then 
            color_lerp = math.min(1.0, color_lerp - 1)
            id_color = CMath.ColorLerp( Vector4(1.0, 1.0, 0.0, 0.4), Vector4(1.0, 0.0, 0.0, 0.4), color_lerp)
        else
            id_color = CMath.ColorLerp( Vector4(0.0, 1.0, 0.0, 0.4), Vector4(1.0, 1.0, 0.0, 0.4), color_lerp)
        end
        
        self.gpu_ids_btns[j] = GuiButton({
                styles = {GuiStyles.perf_id,},
                background = {color = id_color,},
                text = {str = name},
                enable = false,
                height_percent = true,
                height = 90 - math.min(80, depth * 15),
                bottom = 5,
                bottom_percent = true,
                valign = GUI_VALIGN.BOTTOM,
                id = tostring(param_id),
                alt = "Detail view of "..name,
                events = {
                    [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return Profiler:SetStatsParam(self, true) end,
                    [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) return Profiler:SetStatsParam(nil, true) end,},
        })
        self.gru_rect:AttachChild( self.gpu_ids_btns[j].entity )
    end
end

function Profiler:Init()
    print("Profiler:Init") 
    
    self.profiler = Util.GetProfiler()
    if self.profiler == nil then
        error("Profiler does not initialized!")
        return
    end

    self.profiler:Start()
    
    self.realtime = true

    self.stats = false
    self.stats_param = 0
    self.stats_thread = 0
    self.stats_avg = 0
    self.stats_avg_p = 0
    self.stats_frame_count = 0
    self.stats_max = 0
    self.stats_max_p = 0
    self.stats_cur = 0
    self.stats_cur_p = 0
    self.stats_cur_count = 0

    self.total_frame_time = 0
    self.cpu_frame_time = 0
    self.gpu_frame_time = 0
    self.total_frame_count = 0

    loader.require("ProfilerWindow", Profiler.reload)
    
    self.profiler_win = Gui.ProfilerWindow()

    self:FillWindow()

    self.profiler_win.entity:UpdatePosSize() 

    self.update_time = 0
    self.stats_update_time = 0

    self.stats_btn = nil
end

function Profiler:IsInit()
    return self.profiler_win ~= nil
end

function Profiler:Close()
    if self:IsInit() then 
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
    if not self.profiler:IsRunning() or not self.realtime then return end
    
	self.update_time = self.update_time + dt
    
    local frame_time = self.profiler:GetCurrentTimeSlice(0, 0)
    local total_time = frame_time.x + frame_time.y
    
    local gpu_frame_time = self.profiler:GetGpuCurrentTimeSlice(0)
    local gpu_total_time = gpu_frame_time.x + gpu_frame_time.y

    self.cpu_frame_time = self.cpu_frame_time + total_time
    self.gpu_frame_time = self.gpu_frame_time + gpu_total_time
    self.total_frame_time = self.total_frame_time + dt

    self.total_frame_count = self.total_frame_count + 1
    if self.total_frame_time > 100 then
        local total_time = self.total_frame_time / self.total_frame_count
        local fps = 1000.0 / total_time

        self.stats_frame_ms:SetString(string.format("%.2f ms", total_time))
        self.stats_frame_fps:SetString(string.format("%.1f fps", fps))
        
        self.stats_cpuframe:SetString(string.format("%.2f ms", self.cpu_frame_time / self.total_frame_count))
        self.stats_gpuframe:SetString(string.format("%.2f ms", self.gpu_frame_time / self.total_frame_count))
        
        self.total_frame_count = 0
        self.total_frame_time = 0
        self.cpu_frame_time = 0
        self.gpu_frame_time = 0
    end

    if self.stats then
        self.stats_update_time = self.stats_update_time + dt

        local id_time = Vector2(0,0)
        local relative_time = 0

        if self.stats_thread < 0 then
            id_time = self.profiler:GetGpuCurrentTimeSlice(self.stats_param)
            relative_time = id_time.y / gpu_frame_time.y
        else
            id_time = self.profiler:GetCurrentTimeSlice(self.stats_param, self.stats_thread)
            relative_time = id_time.y / frame_time.y
        end

        self.stats_cur = self.stats_cur + id_time.y
        self.stats_cur_p = self.stats_cur_p + relative_time
        self.stats_cur_count = self.stats_cur_count + 1
        self.stats_frame_count = self.stats_frame_count + 1

        local delta = 1.0 / self.stats_frame_count
        self.stats_avg = self.stats_avg * (1 - delta) + id_time.y * delta
        self.stats_avg_p = self.stats_avg_p * (1 - delta) + relative_time * delta
        self.stats_max = math.max(self.stats_max, id_time.y)
        self.stats_max_p = math.max(self.stats_max_p, relative_time)

        if self.stats_update_time > 100 then
            self.stats_update_time = 0
            
            self.stats_avg_ms:SetString(string.format("%.2f ms", self.stats_avg))
            self.stats_max_ms:SetString(string.format("%.2f ms", self.stats_max))
            self.stats_cur_ms:SetString(string.format("%.2f ms", self.stats_cur / self.stats_cur_count))
            self.stats_avg_prc:SetString(string.format("%.1f", self.stats_avg_p * 100).." %")
            self.stats_max_prc:SetString(string.format("%.1f", self.stats_max_p * 100).." %")
            self.stats_cur_prc:SetString(string.format("%.1f", self.stats_cur_p * 100 / self.stats_cur_count).." %")

            self.stats_cur = 0
            self.stats_cur_p = 0
            self.stats_cur_count = 0
        end
    end

    if self.update_time < 500 then return end
    self.update_time = 0
    
    for i = 1, self.threads_count do
        for j = 1, self.ids_count do 
            local id_time = self.profiler:GetCurrentTimeSlice(j - 1, i - 1)
            self.ids_btns[i][j].entity.left = CMath.Round( self.line_w * math.min(1, id_time.x / total_time) )
            self.ids_btns[i][j].entity.width = CMath.Round( self.line_w * math.min(1, id_time.y / total_time) )
            self.ids_btns[i][j].entity.enable = true
            self.ids_btns[i][j].entity:UpdatePosSize()
        end
    end
    
    for j = 1, self.gpu_ids_count do 
        local id_time = self.profiler:GetGpuCurrentTimeSlice(j - 1)
        self.gpu_ids_btns[j].entity.left = CMath.Round( self.line_w * math.min(1, id_time.x / gpu_total_time) )
        self.gpu_ids_btns[j].entity.width = CMath.Round( self.line_w * math.min(1, id_time.y / gpu_total_time) )
        self.gpu_ids_btns[j].entity.enable = true
        self.gpu_ids_btns[j].entity:UpdatePosSize()
    end
end

function Profiler:SetDump(dump)
    if self.profiler == nil then return end
    self.profiler.dump = dump
end

function Profiler:SetRealtime(realtime)
    if not Profiler:IsInit() then return end

    self.realtime = realtime
end

function Profiler:SetStatsParam(btn, gpu)
    self.stats_avg = 0
    self.stats_avg_p = 0
    self.stats_frame_count = 0
    self.stats_max = 0
    self.stats_max_p = 0
    self.stats_cur = 0
    self.stats_cur_p = 0
    self.stats_cur_count = 0

    self.stats_avg_ms:SetString("")
    self.stats_max_ms:SetString("")
    self.stats_cur_ms:SetString("")
    self.stats_avg_prc:SetString("")
    self.stats_max_prc:SetString("")
    self.stats_cur_prc:SetString("")

    local name = ""
    
    if btn == nil then
        self.stats = false
        self.stats_param = 0
        self.stats_thread = 0
        self.stats_btn = nil
    else
        if self.stats_btn ~= nil then 
            self.stats_btn:SetPressed(false)
        end

        local btn_id = btn.entity:GetID()

        self.stats = true
        if gpu then
            self.stats_param = tonumber(btn_id)        
            self.stats_thread = -1
            name = "GPU_".. self.profiler:GetGpuIDName(self.stats_param)
        else
            local param, thread = btn_id:match("([^_]+)_([^_]+)")
            self.stats_param = tonumber(param)        
            self.stats_thread = tonumber(thread)
            name = self.profiler:GetIDName(self.stats_param)
        end
        self.stats_btn = btn
    end

    local stats_dumb = self.profiler_win.entity:GetChildById('param_stats')
    stats_dumb:GetChildById('param_name'):GetInherited():SetString(name)    
    
    return true
end