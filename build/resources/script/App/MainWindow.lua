if not MainWindow then MainWindow = {} end

local CAPTION_DELIM = " - "
local APP_NAME = "Appname"
local NEW_SCENE_NAME = "New scene"

function MainWindow.reloadTopBar()
    local root = CoreGui.GetMainRoot()
    
    if MainWindow.window then
        root:DetachChild(MainWindow.window.entity)
        MainWindow.window.entity:Destroy()
        MainWindow.window = nil
    end
    
    MainWindow.window = Gui.TopBar()
    root:AttachChild(MainWindow.window.entity)
    MainWindow.window.entity:UpdatePosSize()
    
    local caption_ent = MainWindow.window.entity:GetChildById('tb_caption')
    if caption_ent:is_null() then error("Require tb_caption string!") end
    
    MainWindow.caption_str = caption_ent:GetInherited()
    MainWindow.caption_str:SetString(APP_NAME)
	MainWindow.mainwin.caption_text = APP_NAME

    MainWindow.sys_max = MainWindow.window.entity:GetChildById('sys_max')
    if MainWindow.sys_max:is_null() then error("Require sys_max button!") end
    MainWindow.sys_rst = MainWindow.window.entity:GetChildById('sys_rst')
    if MainWindow.sys_rst:is_null() then error("Require sys_rst button!") end
end

function MainWindow:Init()
    print("MainWindow:Init")  

    self.filterOpen = dlgFilter()
	self.filterOpen:Add("Scene", "*.mls")
	self.filterOpen:Add("Packed scene", "*.mlp")
	self.filterOpen:Add("All files", "*.*")

    self.filterSave = dlgFilter()
	self.filterSave:Add("Scene", "*.mls")
	self.filterSave:Add("Packed scene", "*.mlp")
    
	self.mainwin = CoreGui.SysWindows.Create()
    
        -- todo
	self.mainwin:SetPosSize(200, 100, 1920, 1080)

    if self.mainwin:IsMaximized() then
        self.mainwin:SetCaptionBorderSize(235, 95, 0, 26 + SYSTEM_BORDER_SIZE, 4)
    else
        self.mainwin:SetCaptionBorderSize(235, 95, 0, 26, 4)
    end

    local main_win_bg = CoreGui.GetColor('main_win_bg')
	self.mainwin:SetColorBgPtr(main_win_bg)
	self.mainwin:SetColorBorderPtr(CoreGui.GetColor('main_win_brd'))
	self.mainwin:SetColorBorderFocusPtr(CoreGui.GetColor('main_win_brd_focus'))
	self.mainwin:SetAlpha(main_win_bg.w)
    
    self.mainWinRoot = GuiRoot(CoreGui.GetRootByWindow(self.mainwin))

    -- topbar
    loader.require("Menus.TB_File")
    loader.require("Menus.TB_Sets")
    loader.require("TopBar", MainWindow.reloadTopBar)
    self.reloadTopBar()
    
    -- viewport
    Tools:Init()
    Properties:Init()
    MaterialProps:Init()
    Viewport:Init()

    Resource.ForceTextureReload()

	self.mainwin:Show(true)
end

function MainWindow:Tick(dt)
    if self.mainwin:IsMaximized() then
        self.sys_max.enable = false
        self.sys_rst.enable = true
    else
        self.sys_max.enable = true
        self.sys_rst.enable = false
    end

    self.menu_just_closed = false

    Properties:Tick(dt)
    MaterialProps:Tick(dt)

    Viewport:Tick(dt)
end

function MainWindow:Exit()
    self.mainwin:UserClose()
end

function MainWindow:SetCaption(world_path)
    if world_path == nil then 
        self.caption_str:SetString(APP_NAME) 
        return
    end

    local world_name = ""
    if world_path:len() == 0 then
        world_name = NEW_SCENE_NAME
    else
        world_name = world_path
        while world_name:len() > 0 do
            local name_start = world_name:find("\\")
            if name_start == nil then name_start = world_name:find("/") end
            if name_start == nil then
                break
            end            
            world_name = world_name:sub(name_start+1)
        end
    end

    local caption = world_name..CAPTION_DELIM..APP_NAME
    self.caption_str:SetString(caption) 
    MainWindow.mainwin.caption_text = caption
end

---- menu buttons
function MainWindow:FileMenuPress(ent)
    if self.menu_just_closed then
        local fake_event = HEvent()
        fake_event.event = GUI_EVENTS.MOUSE_DOWN
        fake_event.key = KEYBOARD_CODES.KEY_LBUTTON
        ent:Callback(fake_event)
        return true
    end

    if self.menu_sets then
		self.menu_sets:SendCloseEvent() 
		self.menu_sets = nil
	end
	
    self.menu_file = Gui.TB_File()
    ent:GetInherited():AttachOverlay(self.menu_file)
    local corners = ent:GetCorners()
    self.menu_file:Open(corners.l, corners.b)

    if not SceneMgr:IsWorld() then
		self.menu_file:SetItemState("tb_save", false)
        self.menu_file:SetItemState("tb_saveas", false)
        self.menu_file:SetItemState("tb_close", false)
    end
	if not SceneMgr:IsUnsave() then
		self.menu_file:SetItemState("tb_save", false)
	end

    return true
end

function MainWindow:FileMenuHover(ent)
    if self.menu_sets then
		MainWindow:FileMenuPress(ent)
		ent:GetInherited():SetPressed(true)
	end
    return true
end

function MainWindow:FileMenuClose(btn)
    self.menu_file = nil
    self.menu_just_closed = true
    return btn:SetPressed(false) 
end

function MainWindow:FileMenuClick(btn, ev)
    self:FileMenuClose(btn)
    
    if ev.id == "tb_create" then
        print("Creating scene")
        if SceneMgr:CreateWorld() == 0 then
            error("Unable to create scene!")
        end

    elseif ev.id == "tb_open" then
        print("Opening scene")
        local res = dlgOpenFile(self.mainwin:GetHWND(), "Open scene", self.filterOpen)
        if res == "" then return true end
        if SceneMgr:LoadWorld(res) == 0 then
            error("Unable to open scene "..res)
        end

    elseif ev.id == "tb_save" then
        print("Saving scene")
        if SceneMgr.worlds[SceneMgr.current_world].path:len() == 0 then
            local res = dlgSaveFile(self.mainwin:GetHWND(), "Save scene", self.filterSave)
            if res == "" then return true end
            SceneMgr:SaveAsWorld(SceneMgr.current_world, res)
        else
            SceneMgr:SaveWorld(SceneMgr.current_world)
        end

    elseif ev.id == "tb_saveas" then
        print("Saving scene as")
        local res = dlgSaveFile(self.mainwin:GetHWND(), "Save scene", self.filterSave)
        if res == "" then return true end
        SceneMgr:SaveAsWorld(SceneMgr.current_world, res)

    elseif ev.id == "tb_close" then
        print("Closing scene")
        SceneMgr:CloseWorld(SceneMgr.current_world)

    elseif ev.id == "tb_exit" then
        print("Exiting app")
        self:Exit()
    end
    return true
end

function MainWindow:SetsMenuPress(ent)
    if self.menu_just_closed then
        local fake_event = HEvent()
        fake_event.event = GUI_EVENTS.MOUSE_DOWN
        fake_event.key = KEYBOARD_CODES.KEY_LBUTTON
        ent:Callback(fake_event)
        return true
    end

    if self.menu_file then
		self.menu_file:SendCloseEvent() 
		self.menu_file = nil
	end
	
    self.menu_sets = Gui.TB_Sets()
    ent:GetInherited():AttachOverlay(self.menu_sets)
    local corners = ent:GetCorners()
    self.menu_sets:Open(corners.l, corners.b)

    return true
end

function MainWindow:SetsMenuSub(ev)
    if ev.id ~= "test_sub_menu" then return true end 

    if not SceneMgr:IsWorld() then
        local sub = ev.entity:GetInherited()
		sub:SetItemState("tb_dev_skyrebake", false)
        sub:SetItemState("tb_dev_convert", false)
    end

    return true
end

function MainWindow:SetsMenuHover(ent)
    if self.menu_file then
		MainWindow:SetsMenuPress(ent)
		ent:GetInherited():SetPressed(true)
	end
    return true
end

function MainWindow:SetsMenuClose(btn)
    self.menu_sets = nil
    self.menu_just_closed = true
    return btn:SetPressed(false) 
end

function MainWindow:SetsMenuClick(btn, ev)
    self:SetsMenuClose(btn)
    
    if ev.id == "tb_render" then
        print("Opening render settings")

    elseif ev.id == "tb_interface" then
        print("Opening interface settings")

    elseif ev.id == "tb_dev_shbuf" then
        print("Opening shadows debug view")

    elseif ev.id == "tb_dev_skyrebake" then
        Viewport.lua_world.world:RebakeSky()
        print("Rebaking sky prob")

    elseif ev.id == "tb_dev_convert" then
        for i, ent in ipairs(Viewport.selection_set) do
            local mesh = Viewport.lua_world.world.staticMesh:GetMesh(ent)
            if mesh:len() ~= 0 then Resource.ConvertMeshToSTM(mesh) end
        end 
        print("Converting meshes")
        
    elseif ev.id == "tb_dev_profiler" then
        if self.profiler_win == nil then
            loader.require("ProfilerWindow")-- todo hot reload

            self.profiler_win = Gui.ProfilerWindow()
            self.profiler_win.entity:UpdatePosSize()

            print("Profiler window opening")
        end

    end
    return true
end