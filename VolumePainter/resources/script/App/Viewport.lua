loader.require("App.EditorCamera")
loader.require("App.TransformControls")

if not Viewport then Viewport = {} end

function Viewport.reload()
    local root = CoreGui.GetMainRoot()

    if Viewport.window then
        root:DetachChild(Viewport.window.entity)
        Viewport.window.entity:Destroy()
        Viewport.window = nil
    end

    Viewport.window = Gui.ViewportWindow()
    root:AttachChild(Viewport.window.entity)

    Viewport.sys_win = CoreGui.SysWindows.GetByEntity(Viewport.window.entity)

    -- resize
    local Grect = root:GetRectAbsolute()

    local tool_rect = Tools.window.entity:GetCorners()
    Viewport.window.entity.top = tool_rect.b + Viewport.window.entity.top

    --side_rect = Tools.right_side_area.entity:GetRectAbsolute()
    Viewport.window.entity.right = 0 --Grect.w - (side_rect.l - Grect.l)
	local side_rect = Tools.left_side_area.entity:GetRectAbsolute()
    Viewport.window.entity.left = side_rect.l - Grect.l + side_rect.w

    Viewport.window.entity:UpdatePosSize()

    local ent = Viewport.window.entity:GetChildById('viewport_rect')
    if ent:is_null() then error("Require viewport_rect area!") end
    Viewport.viewport = ent:GetInherited()

    Viewport.overlay_gui = Viewport.window.entity:GetChildById('vp_overlay_gui')
    Viewport.fps_string = Viewport.overlay_gui:GetChildById('vp_fps'):GetInherited()
    Viewport.ms_string = Viewport.overlay_gui:GetChildById('vp_ms'):GetInherited()
    Viewport.rendercfg_btn = Viewport.overlay_gui:GetChildById('vp_rendercfg'):GetInherited()

    local vp_rect = Viewport.window.entity:GetRectAbsolute()
    Viewport.viewport.entity.width = vp_rect.w + vp_rect.w % 2
    Viewport.viewport.entity.height = vp_rect.h + vp_rect.h % 2

    if Viewport.volumeWorld then
        local srv = Viewport.volumeWorld.sceneRenderer:GetSRV()
        Viewport.viewport.rect_mat:SetShaderResourceByID(srv, 0, SHADERS.PS)
        Viewport.viewport.entity.visible = true
    end
end

function Viewport:Init()
    print("Viewport:Init") 
    
    loader.require("RenderConfig")
    loader.require("ViewportWindow", Viewport.reload)
    self.reload()

    TransformControls:PreInit()

    -- configs
    self.movespeed = 0.02
    self.hidemouse = true
    self.arrows_scale = 0.175

    -- states
    self.prev_coords = {x = 0, y = 0}
    self.rmouse_down = false

    self.freelook = false
    self.drawhud = true

    self.worldlive = false
    self.fullscreen = false
    self.renderActive = true
	
    self.frame_ms = 0
    self.frame_count = 0
end

function Viewport:Tick(dt, isActive)
    if not self.volumeWorld then return end

    if self.renderActive ~= isActive then
        local renderCfg = self.volumeWorld.sceneRenderer:GetConfig()
        renderCfg.active = isActive
        self.renderActive = isActive
    end

    if self.renderActive == false then return end

    EditorCamera:Tick(dt)
    TransformControls:Tick(EditorCamera.cameraEntity)

    if self.frame_ms < 100 then
        self.frame_ms = self.frame_ms + dt
        self.frame_count = self.frame_count + 1
    else
        local av_ms = self.frame_ms / self.frame_count
        self.frame_ms = 0
        self.frame_count = 0
        self.ms_string:SetString(string.format("%.1f ms", av_ms))
        local fps = 1000.0 / av_ms
        self.fps_string:SetString(string.format("%.1f fps", fps))
    end
end

function Viewport:SetWorld(world)
	if not world.coreWorld or world.sceneRenderer then return end
	self.volumeWorld = world

	EditorCamera:Init( self.volumeWorld.coreWorld )
	TransformControls:Init( self.volumeWorld.coreWorld )

    local vp_rect = self.viewport.entity:GetRectAbsolute()
	local srv = self.volumeWorld:AttachViewport(EditorCamera.cameraEntity, vp_rect.w, vp_rect.h)

	self.volumeWorld.coreWorld.active = true
	
	self.viewport.rect_mat:SetShaderResourceByID(srv, 0, SHADERS.PS)
    self.viewport.entity.visible = true
    
	self:onResize(true)
	self.window.entity:UpdatePosSize()
	
	MainWindow:SetCaption(self.volumeWorld.path)
    Tools:ActivateAll()
	
    self.overlay_gui.enable = true	
end

function Viewport:ClearWorld()
    EditorCamera:Close()
    TransformControls:Close()

    self.volumeWorld = nil
    self.viewport.rect_mat:ClearTextures()
    self.viewport.entity.visible = false
    
    self.overlay_gui.enable = false

    MainWindow:SetCaption()
	Tools:DeactivateAll()
	Properties:Disable()
end

function Viewport:OpenRenderConfig(btn)
    if self.renderConfig ~= nil then return true end

    self.renderConfig = Gui.RenderConfig()
    
    local root = self.window.entity:GetRoot()
    root:AttachChild(self.renderConfig.entity)

    local rect = btn.entity:GetRectAbsolute()
    self.renderConfig.entity.top = rect.t + rect.h + 4
    self.renderConfig.entity.left = rect.l + rect.w - self.renderConfig.entity.width
    self.renderConfig.entity:UpdatePosSize()
    root:SetHierarchyFocus(self.renderConfig.entity)
    
    local ev = HEvent()
    ev.event = GUI_EVENTS.UPDATE
    self.renderConfig.entity:SendEvent(ev)
    return true
end

function Viewport:CloseRenderConfig()
    if self.renderConfig == nil then return true end
    
    local root = self.window.entity:GetRoot()
    root:DetachChild(self.renderConfig.entity)
    self.renderConfig.entity:Destroy()
    self.renderConfig = nil

    local cursor = CoreGui.GetCursorPos()
    if not self.rendercfg_btn.entity:IsCollide(cursor.x, cursor.y) then 
        self.rendercfg_btn:SetPressed(false)
    end
    return true
end

function Viewport:MoveCameraToSelection()
    if #self.selection_set == 0 then return end

    pos = self.volumeWorld.coreWorld.transform:GetPosition_W(self.selection_set[1])
    EditorCamera.cameraNode:SetPosition_L3F(pos.x, pos.y, pos.z)
end

function Viewport:ToggleFullscreen()
    if not self.volumeWorld or not self.window then return end
    
    if self.fullscreen then
        self.window.entity.focus_mode = GUI_FOCUS_MODE.NORMAL
        self.window.entity.top = self.oldTop
        self.window.entity.bottom = self.oldBottom
        self.window.entity.left = self.oldLeft
        self.window.entity.right = self.oldRight
        
        self.window.entity:UpdatePosSize()
        self:onResize(true)

        self.fullscreen = false
    else
        self.oldTop = self.window.entity.top
        self.oldBottom = self.window.entity.bottom
        self.oldLeft = self.window.entity.left
        self.oldRight = self.window.entity.right
        
        self.window.entity.focus_mode = GUI_FOCUS_MODE.ONTOP
        self.window.entity.top = 0
        self.window.entity.bottom = 0
        self.window.entity.left = 0
        self.window.entity.right = 0
        
        self.window.entity:UpdatePosSize()
        self:onResize(true)
        
        local root = self.window.entity:GetRoot()
        root:SetFocus(self.window.entity)

        self.fullscreen = true
    end
end

function Viewport:ToggleWorldLive()
    if not self.volumeWorld then return end
    
    if self.worldlive == true then
        self.volumeWorld.coreWorld.mode = WORLDMODES.NO_LIVE
        self.worldlive = false
    else
        self.volumeWorld.coreWorld.mode = WORLDMODES.LIVE        
        self.worldlive = true
    end
end

-- callbacks
function Viewport:onResize(force)
    if MainWindow.mainwin:IsMinimized() then
        if self.volumeWorld then self.volumeWorld.coreWorld.active = false end
        --self.viewport.rect_mat:ClearTextures()
        return true 
    else
        if self.volumeWorld then self.volumeWorld.coreWorld.active = true end
    end
	
    local vp_rect = self.window.entity:GetRectRelative()
    if self.viewport and self.volumeWorld then 
        local vp_w = vp_rect.w + vp_rect.w % 2
        local vp_h = vp_rect.h + vp_rect.h % 2

        self.viewport.entity.width = vp_w
        self.viewport.entity.height = vp_h
		
		if self.sys_win:IsEndResize() or force then
            self.viewport.rect_mat:ClearTextures()
			local srv = self.volumeWorld:ResizeViewport(vp_w, vp_h)
			self.viewport.rect_mat:SetShaderResourceByID(srv, 0, SHADERS.PS)
			
			print("Viewport resize to "..vp_w.." x "..vp_h)
        end
		
		if force then 
            self.viewport.entity:UpdatePosSize()
        end
    end
    return true
end

function Viewport:PlaceEntity(entity, mouse_coords)
    local mcoords = self:GetMouseInVP(mouse_coords)

    local cam_dir = self.volumeWorld.coreWorld.camera:GetVectorFromScreen(EditorCamera.cameraEntity, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
    local cam_origin = self.volumeWorld.coreWorld.camera:GetPos(EditorCamera.cameraEntity)
    local coords = self.volumeWorld.coreWorld.visibility:CollideRayCoords(cam_origin, cam_dir, self.volumeWorld.coreWorld.camera:GetFrustumId(EditorCamera.cameraEntity))
    
    if coords.w >= SELECTION.MAXDIST then      
        coords.x = cam_origin.x + cam_dir.x * SELECTION.DEFAULT_CAM_OFFSET;
        coords.y = cam_origin.y + cam_dir.y * SELECTION.DEFAULT_CAM_OFFSET;
        coords.z = cam_origin.z + cam_dir.z * SELECTION.DEFAULT_CAM_OFFSET;
    else
        local size = self.volumeWorld.coreWorld.visibility:GetBoxSizeL(entity)
        local center = self.volumeWorld.coreWorld.visibility:GetBoxCenterL(entity)
        coords.y = coords.y + math.max(0, size.y - center.y)
    end

    self.volumeWorld.coreWorld.transform:SetPosition_L3F(entity, coords.x, coords.y, coords.z)
    self.volumeWorld.coreWorld.transform:ForceUpdate(entity)
end

function Viewport:onMouseDown(eventData)
    if not self.volumeWorld then return true end
    
    self.prev_coords.x = eventData.coords.x
    self.prev_coords.y = eventData.coords.y

    if eventData.key == KEYBOARD_CODES.KEY_RBUTTON then
        self.rmouse_down = true
        return true
    end

    if eventData.key == KEYBOARD_CODES.KEY_LBUTTON and self.drawhud then
        self.window.entity:SetHierarchyFocusOnMe(true)
		
		local mcoords = self:GetMouseInVP(eventData.coords)
		local ray = self.volumeWorld.coreWorld.camera:GetVectorFromScreen(EditorCamera.cameraEntity, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
		
		Tools:BrushStart(EditorCamera:GetPosition(), ray)
		Tools:TransformStart(ray)
		return true
    end
    return true
end

function Viewport:onMouseUp(eventData)
    if not self.volumeWorld then return true end
    
    if eventData.key == KEYBOARD_CODES.KEY_RBUTTON then
        if self.rmouse_down then
            if self.drawhud then
                --self.vp_menu = Gui.ViewportMenu()
                --self.window:AttachOverlay(self.vp_menu)
                --self.vp_menu:Open(eventData.coords.x, eventData.coords.y)
            end

            self.rmouse_down = false
        else
            self:SetFreelook(false)
            EditorCamera:onStopMove()

            self.window.entity:SetHierarchyFocusOnMe(false)
        end
        return true
    end

	if eventData.key == KEYBOARD_CODES.KEY_LBUTTON then
		self.window.entity:SetHierarchyFocusOnMe(false)

		Tools:BrushStop()
		Tools:TransformStop()
		return true
    end
    return true
end

function Viewport:onKeyDown(eventData)
    if not self.volumeWorld then return false end
    
    if self.freelook then
        EditorCamera:onStartMove(eventData.key)
        return true
     end

    if eventData.key == KEYBOARD_CODES.KEY_ESCAPE then
        -- todo
    end

    if eventData.key == KEYBOARD_CODES.KEY_DELETE and self.drawhud then
		-- todo
	end

    return false
end

function Viewport:onKeyUp(eventData)
    if not self.volumeWorld then return true end
    
    if self.freelook then
        EditorCamera:onStopMove(eventData.key)
        return true
    end
    return true
end

function Viewport:onMouseMove(eventData)
    if not self.volumeWorld then return true end
    
    local mouse_pos = {x = eventData.coords.x, y = eventData.coords.y}

    if self.rmouse_down then
        self.rmouse_down = false
        self.window.entity:SetHierarchyFocusOnMe(true)
        self:SetFreelook(true)

        local corners = self.window.entity:GetCorners()
        mouse_pos.x = (corners.l + corners.r) / 2
        mouse_pos.y = (corners.t + corners.b) / 2
    end

    local is_ctrl = CoreGui.Keys.Ctrl()
	    
	local mcoords = self:GetMouseInVP(mouse_pos)
	local ray = self.volumeWorld.coreWorld.camera:GetVectorFromScreen(EditorCamera.cameraEntity, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
	
	Tools:BrushAction(EditorCamera:GetPosition(), ray)
	Tools:TransformAction(ray)
    
    if self.freelook then
		Tools:TransformUnhover()
		
		if self.hidemouse then
            local rect = self.window.entity:GetCorners()
            local center_x = (rect.l + rect.r) / 2
            local center_y = (rect.t + rect.b) / 2  

            local delta_x = mouse_pos.x - center_x
            local delta_y = -mouse_pos.y + center_y

            EditorCamera:onDeltaRot(delta_x, delta_y)
            self:CenterMouse()
        else
            local delta_x = mouse_pos.x - self.prev_coords.x
            local delta_y = -mouse_pos.y + self.prev_coords.y

            EditorCamera:onDeltaRot(delta_x, delta_y)

            self.prev_coords.x = mouse_pos.x
            self.prev_coords.y = mouse_pos.y
        end
    end

    return true
end

function Viewport:onMouseWheel(eventData)
    if not self.volumeWorld then return true end

    if self.freelook then
        EditorCamera:onMoveSpeed(eventData.coords.x)
    end
    return true
end

function Viewport:onItemStartDrag(eventData)
    self.allowDrop = false

    local itemCount = CoreGui.DragDrop.GetCount()
    if itemCount == 0 then return true end
    
    local firstFile = CoreGui.DragDrop.GetItem(0)

    if not self.volumeWorld then
        if itemCount > 1 then return true end
        if string.find(firstFile, ".mls") == nil then return true end
    else
        if Resource.IsMeshSupported(firstFile) == false then return true end
    end
    
    self.allowDrop = true
    return true
end

function Viewport:onItemDrag(eventData)
    CoreGui.DragDrop.AllowDrop(self.allowDrop)
    return true
end

function Viewport:onItemStopDrag(eventData)
    self.allowDrop = false
    return true
end

function Viewport:onItemDroped(eventData)
    if not self.allowDrop then return true end
    
    local itemCount = CoreGui.DragDrop.GetCount()
    if itemCount == 0 then return true end

    if not self.volumeWorld then
        if itemCount > 1 then return true end
        
        local worldPath = CoreGui.DragDrop.GetItem(0)
        if string.find(worldPath, ".mls") ~= nil then
            SceneMgr:LoadWorld(worldPath)
        end
    else
        local entities = {}
        for i = 0, itemCount-1 do
            local meshName = CoreGui.DragDrop.GetItem(i)
            print("Drag&Drop: " .. meshName)

            local clbData = {}
            clbData.entity = EntityTypes.StaticModel(self.volumeWorld.coreWorld) -- to history
            if not clbData.entity:IsAlive() then return true end
            
            Importer:ImportLoadMesh(meshName, 
                function (resName, status, data)
                    data.entity:SetMesh( resName..EXT.MESH )
                end,
                clbData)

            self:PlaceEntity(clbData.entity.ent, eventData.coords)
            table.insert(entities, clbData.entity.ent)
        end
    end
    return true
end

function Viewport:SwitchHud()
    if not self.volumeWorld or self.gamemode then return end

    self.drawhud = not self.drawhud
    self.volumeWorld.sceneRenderer:GetConfig().editorGuiEnable = self.drawhud
    self.overlay_gui.enable = self.drawhud
end

function Viewport:SetFreelook(look)
    if self.gamemode == true then return end

    self.freelook = look
    if self.hidemouse then
        self:SetMouseVis(not self.freelook)
    end
end

function Viewport:SetMouseVis(show)
    self:CenterMouse()
    CoreGui.ShowCursor(show)
end

function Viewport:CenterMouse()
    local corners = self.window.entity:GetCorners()
    local center_x = (corners.l + corners.r) / 2
    local center_y = (corners.t + corners.b) / 2
    CoreGui.SetCursorPos(self.window.entity, center_x, center_y)
end

function Viewport:GetMouseInVP(coords)
    local vp_rect = self.viewport.entity:GetRectAbsolute()
    local res = {}
    res.x = coords.x - vp_rect.l
    res.y = coords.y - vp_rect.t
    res.w = vp_rect.w
    res.h = vp_rect.h
    return res
end