if not MainWindow then MainWindow = {} end

local CAPTION_DELIM = " - "

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
    
	MainWindow.mainwin.caption_text = lcl.apptitle
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
    self.mainwin:HideWinBorder(false)
	self.mainwin:SetPosSize(200, 100, 1920, 1080)
    self.mainwin:SetCaptionRect(200, 95, 0, 26)
    self.mainwin:SetBorderSize(4)
    self.mainwin:SetIcons("../resources/textures/icons/main_icon.ico", "../resources/textures/icons/main_icon.ico")

    local main_win_bg = CoreGui.GetColor('bg_01_v1')
	self.mainwin:SetColorBgPtr(main_win_bg)
	--self.mainwin:SetColorBorderPtr(CoreGui.GetColor('main_win_brd'))
	--self.mainwin:SetColorBorderFocusPtr(CoreGui.GetColor('main_win_brd_focus'))
	self.mainwin:SetAlpha(main_win_bg.w)
    
    self.mainWinRoot = GuiRoot(CoreGui.GetRootByWindow(self.mainwin))

    -- topbar
    self.menus = {}
    self.menu_just_closed = nil

    loader.require("Menus.TB_File")
    loader.require("Menus.TB_Asset")
    loader.require("Menus.TB_Sets")
    loader.require("TopBar", MainWindow.reloadTopBar)
    self.reloadTopBar()
    
    loader.require("ColorsWindow", MainWindow.reloadColors)
    self.colorsWindow = nil

    Tools:Init()
    Properties:Init()
    MaterialProps:Init()
    AssetBrowser:Init()
    SceneBrowser:Init()
    Viewport:Init()

    --Resource.WaitLoadingComplete()

	self.mainwin:Show(true)
end

function MainWindow:Tick(dt)
    self.menu_just_closed = false

    Properties:Tick(dt)
    MaterialProps:Tick(dt)

    Viewport:Tick(dt, self.mainwin:IsActive())
end

function MainWindow:Exit()
    self.mainwin:UserClose()
end

function MainWindow:SetCaption(world_path)
    if world_path == nil then 
        MainWindow.mainwin.caption_text = lcl.apptitle
        return
    end

    local world_name = ""
    if world_path:len() == 0 then
        world_name = lcl.newscene
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

    local caption = world_name..CAPTION_DELIM .. lcl.apptitle
    MainWindow.mainwin.caption_text = caption
end

function MainWindow.reloadColors()
    if MainWindow.colorsWindow == nil then return end
    MainWindow.colorsWindow:Close()
    MainWindow.colorsWindow = nil
    MainWindow.OpenColorsWindow()
end

function MainWindow.OpenColorsWindow()
    local x = MainWindow.mainwin:GetLeft() + MainWindow.mainwin:GetWidth() / 2
    local y = MainWindow.mainwin:GetTop() + MainWindow.mainwin:GetHeight() / 2

    MainWindow.colorsWindow = Gui.ColorsWindow(x, y)
    MainWindow.colorsWindow.entity:UpdatePosSize() 
end

---- menu buttons
function MainWindow:MenuPress(ent, menu)
    if self.menu_just_closed then
        local fake_event = HEvent()
        fake_event.event = GUI_EVENTS.MOUSE_DOWN
        fake_event.key = KEYBOARD_CODES.KEY_LBUTTON
        ent:Callback(fake_event)
        return true
    end

    for k, m in pairs(self.menus) do
		m:SendCloseEvent() 
		self.menus[k] = nil
    end
	
    self.menus[menu] = Gui["TB_"..menu]()
    ent:GetInherited():AttachOverlay(self.menus[menu])
    local corners = ent:GetCorners()
    self.menus[menu]:Open(corners.l, corners.b)

    if menu == "file" then
        if not SceneMgr:IsWorld() then
		    self.menus[menu]:SetItemState("tb_save", false)
            self.menus[menu]:SetItemState("tb_saveas", false)
            self.menus[menu]:SetItemState("tb_close", false)
        end
	    if not SceneMgr:IsUnsave() then
		    self.menus[menu]:SetItemState("tb_save", false)
	    end
    elseif menu == "asset" then
        
    elseif menu == "sets" then
        
    end

    return true
end

function MainWindow:MenuHover(ent, menu)
    if next(self.menus) ~= nil then
		self:MenuPress(ent, menu)
		ent:GetInherited():SetPressed(true)
	end
    return true
end

function MainWindow:MenuClose(ent, menu)
    self.menus[menu] = nil
    self.menu_just_closed = true
    return ent:SetPressed(false) 
end

function MainWindow:MenuSubOpen(ev)
    if ev.id == "dev_sub_menu" then 
        if not SceneMgr:IsWorld() then
            local sub = ev.entity:GetInherited()
		    sub:SetItemState("tb_dev_skyrebake", false)
            sub:SetItemState("tb_dev_convert", false)
        end
    end

    return true
end

function MainWindow:FileMenuClick(btn, ev)
    self:MenuClose(btn, "file")
    
    if ev.id == "tb_create" then
        if SceneMgr:CreateWorld() == 0 then
            error("Unable to create scene!")
        end

    elseif ev.id == "tb_open" then
        local res = dlgOpenFile(self.mainwin:GetHWND(), "Open scene", self.filterOpen)
        if res == "" then return true end
        SceneMgr:LoadWorld(res)

    elseif ev.id == "tb_save" then
        if SceneMgr.worlds[SceneMgr.current_world].path:len() == 0 then
            local res = dlgSaveFile(self.mainwin:GetHWND(), "Save scene", self.filterSave)
            if res == "" then return true end
            SceneMgr:SaveAsWorld(SceneMgr.current_world, res)
        else
            SceneMgr:SaveWorld(SceneMgr.current_world)
        end

    elseif ev.id == "tb_saveas" then
        local res = dlgSaveFile(self.mainwin:GetHWND(), "Save scene", self.filterSave)
        if res == "" then return true end
        SceneMgr:SaveAsWorld(SceneMgr.current_world, res)

    elseif ev.id == "tb_close" then
        SceneMgr:CloseWorld(SceneMgr.current_world)

    elseif ev.id == "tb_exit" then
        self:Exit()
    end
    return true
end

function MainWindow:AssetMenuClick(btn, ev)
    self:MenuClose(btn, "asset")
    
    if ev.id == "tb_import_mesh" then
        Importer:OpenMeshes(self.mainwin)

    elseif ev.id == "tb_import_tex" then
        

    end
    return true
end

function MainWindow:SetsMenuClick(btn, ev)
    self:MenuClose(btn, "sets")
    
    if ev.id == "tb_config" then
        print("Opening configuration")

    elseif ev.id == "tb_colors" then
        if MainWindow.colorsWindow == nil then
            MainWindow.OpenColorsWindow()
        end
        
    elseif ev.id == "tb_dev_skyrebake" then
        Viewport.lua_world.world:RebakeSky()

    elseif ev.id == "tb_dev_convert" then
        for i, ent in ipairs(Viewport.selection_set) do
            local mesh = Viewport.lua_world.world.staticMesh:GetMesh(ent)
            if mesh:len() ~= 0 then Resource.ConvertMeshToEngineFormat(mesh) end
        end 
        
    elseif ev.id == "tb_dev_profiler" then
        if not Profiler:IsInit() then
            Profiler:Init()
        end

    end
    return true
end