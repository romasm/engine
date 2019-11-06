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

    local side_rect = Tools.right_side_area.entity:GetRectAbsolute()
    Viewport.window.entity.right = Grect.w - (side_rect.l - Grect.l)
    side_rect = Tools.left_side_area.entity:GetRectAbsolute()
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
    
    loader.require("Menus.Viewport")
    loader.require("RenderConfig")
    loader.require("ViewportWindow", Viewport.reload)
    self.reload()

    TransformControls:PreInit()

    -- configs
    self.movespeed = 0.02
    self.hidemouse = true
    self.selection_color = Vector4(1, 0.4, 0.1, 1)
    self.arrows_scale = 0.175

    -- states
    self.prev_coords = {x = 0, y = 0}
    self.rmouse_down = false

    self.freelook = false
    self.drawhud = true

    self.worldlive = false
    self.gamemode = false
    self.fullscreen = false
    self.collisionDraw = false
    self.sceneGraphDraw = false
    self.renderActive = true

    self.tc_action = false
	self.tc_hover = false
	self.tc_prevray = Vector3(0,0,0)
    self.tc_local = false
    self.tc_copied = false
    
    self.selection_mode = SELECTION_MODE.NONE
    self.selection_set = {}

    self.history = {
        transform_type = TRANSFORM_MODE.NONE,
        transform_new = {},
        transform_old = {},
        old_ents = {},
    }
    self.history_push = false

    self.frame_ms = 0
    self.frame_count = 0

    self.vp_menu = nil
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
	
    Tools:SetTransform(TRANSFORM_MODE.NONE)
    self:SetTransform(TRANSFORM_MODE.NONE)

    self.overlay_gui.enable = true
    self.collisionDraw = false
    self.sceneGraphDraw = false
	
	History:Clear()
    self.history_push = false
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

function Viewport:SetPhysicsDraw(draw)
    if not self.volumeWorld then return end
    
    self.collisionDraw = draw
    self.volumeWorld.coreWorld.collision:SetDebugDraw(self.collisionDraw)
end

function Viewport:SetSceneGraphDraw(draw)
    if not self.volumeWorld then return end
    
    self.sceneGraphDraw = draw
    self.volumeWorld.coreWorld:SetSceneGraphDebugDraw(self.sceneGraphDraw)
end

-- TEMP
function Viewport:SpawnPhysics(cam)
    if not self.volumeWorld or not cam then return end
    
    local phymodel = EntityTypes.PhysicsModel(self.volumeWorld.coreWorld)
    
    local pos = cam:GetPosition_W()
    local dir = cam:GetLookDir()
    pos = Vector3.Add(pos, Vector3.MulScalar(dir, 2.5))
    dir = Vector3.MulScalar(dir, 600)
    
    --phymodel:SetMesh(PATH.ROOT .. "content/statics/multimesh01.FBX")
    
    phymodel:SetPosition_L3F(pos.x, pos.y, pos.z)
    phymodel:ApplyCentralImpulse(dir)
end

function Viewport:ToggleGamemode()
    if not self.volumeWorld then return end
    
    if self.gamemode == true then
        local player = self.volumeWorld.coreWorld:GetLuaEntity( self.volumeWorld.coreWorld:GetEntityByName("Player0") )
        if player == nil then return end

        EditorCamera.camera:Enable()

        player:KillHi()

        self.drawhud = true
        --self.volumeWorld.sceneRenderer:GetConfig().editorGuiEnable = true
        
        self:SetMouseVis(true)
        
        self.volumeWorld.coreWorld.mode = WORLDMODES.NO_LIVE

        self.gamemode = false
    else
        local player = EntityTypes.TestPlayer(self.volumeWorld.coreWorld)
        player:Rename("Player0")

        local cameraPos = EditorCamera.camera:GetPosition_W()
        local cameraRot = EditorCamera.camera:GetRotationPYR_W()
        player:SetPosition_L3F(cameraPos.x, cameraPos.y, cameraPos.z)
        player.camera:SetRotationPYR_L3F(cameraRot.x, cameraRot.y, 0)
        
        player.camera:AssignScene(self.volumeWorld.sceneRenderer)

        self.drawhud = false
        --self.volumeWorld.sceneRenderer:GetConfig().editorGuiEnable = false

        self:SetFreelook(false)
        self:SetMouseVis(false)

        self.volumeWorld.coreWorld.mode = WORLDMODES.LIVE
        
        self.gamemode = true
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

function Viewport:MenuClose(ent)
    self.vp_menu = nil
    return true
end

function Viewport:MenuClick(ent, ev) -- to history
    self:MenuClose(ent)
    
    if ev.id == "vp_create_static" then
        local stmodel = EntityTypes.StaticModel(self.volumeWorld.coreWorld)
        self:PlaceAndSelect(stmodel.ent, self.prev_coords)
        
    elseif ev.id == "vp_create_physics" then
        local phymodel = EntityTypes.PhysicsModel(self.volumeWorld.coreWorld)
        self:PlaceAndSelect(phymodel.ent, self.prev_coords)

    elseif ev.id == "vp_create_light" then
        local light = EntityTypes.LocalLight(self.volumeWorld.coreWorld)
        self:PlaceAndSelect(light.ent, self.prev_coords)

    elseif ev.id == "vp_create_glight" then
        local glight = EntityTypes.GlobalLight(self.volumeWorld.coreWorld)
        self:PlaceAndSelect(glight.ent, self.prev_coords)

     elseif ev.id == "vp_dumb" then
        local test_ent = EntityTypes.TestEnt(self.volumeWorld.coreWorld)
        self:PlaceAndSelect(test_ent.ent, self.prev_coords)

     elseif ev.id == "vp_player" then
        local player = EntityTypes.TestPlayer(self.volumeWorld.coreWorld)
        player:Rename("Player0")
        self:PlaceAndSelect(player.ent, self.prev_coords)
		
	elseif ev.id == "vp_envprob" then
		local light = EntityTypes.EnvProb(self.volumeWorld.coreWorld)
		self:PlaceAndSelect(light.ent, self.prev_coords)

	elseif ev.id == "vp_newentity" then
        local test_ent = EntityTypes.BaseEntity(self.volumeWorld.coreWorld)
        test_ent.world:SetEntityType(test_ent.ent, "Custom")
        test_ent.transformSys:AddComponent(test_ent.ent)

        self:PlaceAndSelect(test_ent.ent, self.prev_coords)

    end
    return true
end

function Viewport:PlaceAndSelect(entity, mouse_coords)
    self:PlaceEntity(entity, mouse_coords)

    self:RememberSelection()
    local ents = {entity}
    self:SetSelection(ents)
    self:PushSelectHistory() -- temp
    TransformControls:UpdateTransform(self.selection_set)
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
		
		if self.tc_hover then
			self.tc_action = true
			
            self.history.transform_type = TransformControls.mode
            if self.history.transform_type == TRANSFORM_MODE.MOVE then
			    for i, ent in ipairs(self.selection_set) do
				    self.history.transform_old[i] = self.volumeWorld.coreWorld.transform:GetPosition_L(ent)
			    end
		    elseif self.history.transform_type == TRANSFORM_MODE.ROT then
			    for i, ent in ipairs(self.selection_set) do
				    self.history.transform_old[i] = self.volumeWorld.coreWorld.transform:GetRotation_L(ent)
			    end
		    elseif self.history.transform_type == TRANSFORM_MODE.SCALE then
			    for i, ent in ipairs(self.selection_set) do
				    self.history.transform_old[i] = self.volumeWorld.coreWorld.transform:GetScale_L(ent)
			    end
		    end

            local mcoords = self:GetMouseInVP(eventData.coords)
			self.tc_prevray = self.volumeWorld.coreWorld.camera:GetVectorFromScreen(EditorCamera.cameraEntity, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
		else		
			self.selection_mode = SELECTION_MODE.SIMPLE
			self:Select(eventData.coords)
		end
        return true
    end
    return true
end

function Viewport:onMouseUp(eventData)
    if not self.volumeWorld then return true end
    
    if eventData.key == KEYBOARD_CODES.KEY_RBUTTON then
        if self.rmouse_down then
            if self.drawhud then
                self.vp_menu = Gui.ViewportMenu()
                self.window:AttachOverlay(self.vp_menu)
                self.vp_menu:Open(eventData.coords.x, eventData.coords.y)
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
        if self.tc_action == true and TransformControls.mode == TRANSFORM_MODE.SCALE then
            self:PostScaleSelection()
        end

        self.window.entity:SetHierarchyFocusOnMe(false)
		self.tc_action = false
        self.selection_mode = SELECTION_MODE.NONE
        self.tc_copied = false

        self:PushHistory()
        return true
    end
    return true
end

function Viewport:onKeyDown(eventData)
    if not self.volumeWorld then return false end
    
    if self.freelook then
        EditorCamera:onStartMove(eventData.key)
        
        --TEMP
        if eventData.key == KEYBOARD_CODES.KEY_T then
            self:SpawnPhysics(EditorCamera.camera)
        end

        return true
     end

    if eventData.key == KEYBOARD_CODES.KEY_ESCAPE then
        if self.gamemode then
            self:ToggleGamemode()
        elseif self.drawhud then
            self:DropSelection()
            return true 
        end
    end

    if eventData.key == KEYBOARD_CODES.KEY_DELETE and self.drawhud then
        self:DeleteSelection()
        return true 
    end

    return false
end

function Viewport:DropSelection()
    self:RememberSelection()
    self:UnselectAll()
    self:PushSelectHistory()
    TransformControls:UpdateTransform(self.selection_set)
end

function Viewport:DeleteSelection() -- to history
    if not self.volumeWorld or self.gamemode then return end

    self.history.transform_type = TRANSFORM_MODE.NONE
    self.history.msg = "Delete"
    self.history.undo = function(self)
        local ents = {}
        for i = #self.old_ents, 1, -1 do
            --ents[#self.old_ents - i + 1] = Viewport.volumeWorld.coreWorld:RestoreEntity()
            -- deserialize from historypool
        end
        Viewport:SetSelection(ents)
    end
    self.history.redo = function(self)
        for i = #Viewport.selection_set, 1, -1 do 
            self.old_ents[#Viewport.selection_set - i + 1] = 1 -- serialize to historypool
            Viewport.volumeWorld.coreWorld:DestroyEntity(Viewport.selection_set[i])
            table.remove(Viewport.selection_set, i) 
        end
        TransformControls:UpdateTransform(self.selection_set)
    end

    self.history_push = true
    self.history:redo()
    self:PushHistory()
end

function Viewport:onKeyUp(eventData)
    if not self.volumeWorld then return true end
    
    if self.freelook then
        EditorCamera:onStopMove(eventData.key)
        return true
    end

    if eventData.key == KEYBOARD_CODES.KEY_CONTROL then 
        self.tc_copied = false
        return 
    end
    return true
end

function Viewport:onMouseMove(eventData)
    if not self.volumeWorld then return true end

    if self.gamemode then
        self:CenterMouse()
        return true
    end
    
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

    if self.selection_mode > SELECTION_MODE.NONE and is_ctrl and self.drawhud then
        self.selection_mode = SELECTION_MODE.SNAKE
        self:Select(mouse_pos)
        return true
    end
    
	local mcoords = self:GetMouseInVP(mouse_pos)
	local ray_dir = self.volumeWorld.coreWorld.camera:GetVectorFromScreen(EditorCamera.cameraEntity, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
	
	if self.tc_action then
        if is_ctrl and not self.tc_copied then
            self:PushHistory()

            self:CopySelection(self.history)
            self.tc_copied = true
            
            self.history_push = true
            self.history.msg = "Copy"
            self.history.undo = function(self)
                    for i = #Viewport.selection_set, 1, -1 do
                        Viewport.volumeWorld.coreWorld:DestroyEntity(Viewport.selection_set[i])
                    end
                    Viewport:SetSelection(self.old_ents)
                end
            self.history.redo = function(self)
                    Viewport:CopySelection(self)  -- SOMETHING WRONG on redo!!! -> 
                        --Do not remember all selection set on every history push, remember only changes 
                    if self.transform_type == TRANSFORM_MODE.MOVE then
	                    Viewport:SetPositionsToSelection(self.transform_new)
	                elseif self.transform_type == TRANSFORM_MODE.ROT then
		                Viewport:SetRotationsToSelection(self.transform_new)
	                elseif self.transform_type == TRANSFORM_MODE.SCALE then
		                Viewport:SetScalesToSelection(self.transform_new)
	                end
                end
        end 

		local tc_mode = TransformControls.mode
		
		if tc_mode == TRANSFORM_MODE.NONE then
			self.tc_action = false
		elseif tc_mode == TRANSFORM_MODE.MOVE then
            TransformControls:ApplyTransform(ray_dir, self.tc_prevray, self.selection_set)

            if not self.history_push then
                self.history_push = true
                self.history.msg = "Transform move"
                self.history.undo = function(self) Viewport:SetPositionsToSelection(self.transform_old) end
                self.history.redo = function(self) Viewport:SetPositionsToSelection(self.transform_new) end
            end
		elseif tc_mode == TRANSFORM_MODE.ROT then
            TransformControls:ApplyTransform(ray_dir, self.tc_prevray, self.selection_set)

            if not self.history_push then
                self.history_push = true
                self.history.msg = "Transform rotate"
                self.history.undo = function(self) Viewport:SetRotationsToSelection(self.transform_old) end
                self.history.redo = function(self) Viewport:SetRotationsToSelection(self.transform_new) end
            end
		elseif tc_mode == TRANSFORM_MODE.SCALE then
            TransformControls:ApplyTransform(ray_dir, self.tc_prevray, self.selection_set)

            if not self.history_push then
                self.history_push = true
                self.history.msg = "Transform scale"
                self.history.undo = function(self) Viewport:SetScalesToSelection(self.transform_old) end
                self.history.redo = function(self) Viewport:SetScalesToSelection(self.transform_new) end
            end
		end
		
		self.tc_prevray = ray_dir
	end

    
    if self.freelook then
		if not self.tc_action then
			TransformControls:Unhover()
			self.tc_hover = false
		end
		
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
    else
		if not self.tc_action then
			self.tc_hover = TransformControls:CheckHover(ray_dir)
		end
    end

    return true
end

function Viewport:onMouseWheel(eventData)
    if not self.volumeWorld or self.gamemode then return true end

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

        self:RememberSelection()
        self:SetSelection(entities)
        self:PushSelectHistory() -- temp
        TransformControls:UpdateTransform(self.selection_set)
    end
    return true
end

-- support
function Viewport:PushHistory()
    if not self.history_push then return end

    if self.history.transform_type == TRANSFORM_MODE.MOVE then
	    for i, ent in ipairs(self.selection_set) do
		    self.history.transform_new[i] = self.volumeWorld.coreWorld.transform:GetPosition_L(ent)
		end
	elseif self.history.transform_type == TRANSFORM_MODE.ROT then
		for i, ent in ipairs(self.selection_set) do
			self.history.transform_new[i] = self.volumeWorld.coreWorld.transform:GetRotation_L(ent)
		end
	elseif self.history.transform_type == TRANSFORM_MODE.SCALE then
		for i, ent in ipairs(self.selection_set) do
		    self.history.transform_new[i] = self.volumeWorld.coreWorld.transform:GetScale_L(ent)
		end
	end
    History:Push(self.history)
    self.history_push = false

    self.history.transform_type = TRANSFORM_MODE.NONE
    for i = #self.history.old_ents, 1, -1 do table.remove(self.history.old_ents, i) end
    for i = #self.history.transform_new, 1, -1 do table.remove(self.history.transform_new, i) end
    for i = #self.history.transform_old, 1, -1 do table.remove(self.history.transform_old, i) end
    
    self.volumeWorld.unsave = true
end

function Viewport:SetPositionsToSelection(positions)
    for i, ent in ipairs(self.selection_set) do
        if i > #positions then return end
        self.volumeWorld.coreWorld.transform:SetPosition_L(ent, positions[i])
        self.volumeWorld.coreWorld.transform:ForceUpdate(ent)
    end
    TransformControls:UpdateTransform(self.selection_set)
end

function Viewport:SetRotationsToSelection(rotations)
    for i, ent in ipairs(self.selection_set) do
        if i > #rotations then return end
        self.volumeWorld.coreWorld.transform:SetRotation_L(ent, rotations[i])
        self.volumeWorld.coreWorld.transform:ForceUpdate(ent)
    end
    TransformControls:UpdateTransform(self.selection_set)
end

function Viewport:SetScalesToSelection(scales)
    for i, ent in ipairs(self.selection_set) do
        if i > #scales then return end
        self.volumeWorld.coreWorld.transform:SetScale_L(ent, scales[i])
        self.volumeWorld.coreWorld.transform:ForceUpdate(ent)
    end
    TransformControls:UpdateTransform(self.selection_set)
end

function Viewport:PostScaleSelection()
    for i, ent in ipairs(self.selection_set) do
        self.volumeWorld.coreWorld.collision:PostScale(ent)
        --self.volumeWorld.coreWorld.physics:UpdateState(ent)
    end
end

function Viewport:SwitchHud()
    if not self.volumeWorld or self.gamemode then return end

    self.drawhud = not self.drawhud
        self.volumeWorld.sceneRenderer:GetConfig().editorGuiEnable = self.drawhud
    self.overlay_gui.enable = self.drawhud
end

function Viewport:SwitchTransform()
    if not self.volumeWorld then return end

    local cur_mode = TransformControls.mode
    cur_mode = cur_mode + 1
    if cur_mode > TRANSFORM_MODE.SCALE then cur_mode = TRANSFORM_MODE.NONE end
    self:SetTransform(cur_mode, true)
end

function Viewport:SetTransform(mode, tools)
    if not self.volumeWorld then return end

    TransformControls:SetMode(mode)

    if mode == TRANSFORM_MODE.NONE then
        if tools then Tools:SetTransform(TRANSFORM_MODE.NONE) end

    elseif mode == TRANSFORM_MODE.MOVE then
        if TransformControls.mode == TRANSFORM_MODE.MOVE then
            self.tc_local = not self.tc_local
        else
            TransformControls:SetMode(TRANSFORM_MODE.MOVE)
        end
        TransformControls:SetMoveLocal(self.tc_local)
        TransformControls:UpdateTransform(self.selection_set)
        if tools then Tools:SetTransform(TRANSFORM_MODE.MOVE) end

    elseif mode == TRANSFORM_MODE.ROT then
        if TransformControls.mode == TRANSFORM_MODE.ROT then
            self.tc_local = not self.tc_local
        else
            TransformControls:SetMode(TRANSFORM_MODE.ROT)
        end
        TransformControls:SetRotLocal(self.tc_local)
        TransformControls:UpdateTransform(self.selection_set)
        if tools then Tools:SetTransform(TRANSFORM_MODE.ROT) end

    elseif mode == TRANSFORM_MODE.SCALE then
        TransformControls:SetMode(TRANSFORM_MODE.SCALE)
        TransformControls:UpdateTransform(self.selection_set)
        if tools then Tools:SetTransform(TRANSFORM_MODE.SCALE) end
    end
end

function Viewport:Select(coords)
    if self.selection_mode ~= SELECTION_MODE.SNAKE then self:RememberSelection() end

    local mcoords = self:GetMouseInVP(coords)
    local click_dir = self.volumeWorld.coreWorld.camera:GetVectorFromScreen(EditorCamera.cameraEntity, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
    local click_origin = self.volumeWorld.coreWorld.camera:GetPos(EditorCamera.cameraEntity)

    local is_ctrl = CoreGui.Keys.Ctrl()

    if not is_ctrl and self.selection_mode ~= SELECTION_MODE.SNAKE then
        self:UnselectAll()
    end

    local s_ent = self.volumeWorld.coreWorld.visibility:CollideRay(click_origin, click_dir, self.volumeWorld.coreWorld.camera:GetFrustumId(EditorCamera.cameraEntity))
    if not s_ent:IsNull() then
        if is_ctrl then
            for i, e_ent in ipairs(self.selection_set) do
                if EntIsEq(s_ent, e_ent) then
                    if self.selection_mode ~= SELECTION_MODE.SNAKE then
                        self.volumeWorld.coreWorld.lineGeometry:DeleteComponent(e_ent)
                        table.remove(self.selection_set, i)

                        self:PushSelectHistory()
						
						TransformControls:UpdateTransform(self.selection_set)
                    end
                    return
                end
            end
        end

        local insert = true
        while self.volumeWorld.coreWorld:IsEntityType(s_ent, EDITOR_VARS.TYPE) do
            s_ent = self.volumeWorld.coreWorld.transform:GetParent(s_ent)
            if s_ent:IsNull() then 
                insert = false
                break
            end
        end

        if insert then
            table.insert(self.selection_set, s_ent)
            self.volumeWorld.coreWorld.lineGeometry:AddComponent(s_ent)
            self.volumeWorld.coreWorld.lineGeometry:SetFromVis(s_ent)
            self.volumeWorld.coreWorld.lineGeometry:SetColor(s_ent, self.selection_color)
		end
    end
	TransformControls:UpdateTransform(self.selection_set)
    
    if self.selection_mode ~= SELECTION_MODE.SNAKE then self:PushSelectHistory() end
end

function Viewport:RememberSelection()
    for i, ent in ipairs(self.selection_set) do self.history.old_ents[i] = ent end
end

function Viewport:PushSelectHistory()
    if #self.selection_set == #self.history.old_ents then
        local changed = false
        for i, ent in ipairs(self.selection_set) do
            if not EntIsEq(self.selection_set[i], self.history.old_ents[i]) then
                changed = true
                break 
            end
        end
        if not changed then return end
    end

    self.history_push = true
    self.history.msg = "Selection"
    self.history.undo = function(self)
            local e_old = {}
            for i, ent in ipairs(Viewport.selection_set) do e_old[i] = ent end
            Viewport:SetSelection(self.old_ents)
            for i = #self.old_ents, 1, -1 do table.remove(self.old_ents, i) end
            for i, ent in ipairs(e_old) do self.old_ents[i] = ent end
        end
    self.history.redo = self.history.undo
    self:PushHistory()
end

function Viewport:UnselectAll()
    for i = #self.selection_set, 1, -1 do
        self.volumeWorld.coreWorld.lineGeometry:DeleteComponent(self.selection_set[i])
        table.remove(self.selection_set, i)
    end
end

function Viewport:AddSelection(ents)
    for i, i_ent in ipairs(ents) do
        table.insert(self.selection_set, i_ent)
        self.volumeWorld.coreWorld.lineGeometry:AddComponent(i_ent)
        self.volumeWorld.coreWorld.lineGeometry:SetFromVis(i_ent)
        self.volumeWorld.coreWorld.lineGeometry:SetColor(i_ent, self.selection_color)
    end
    TransformControls:UpdateTransform(self.selection_set)
end

function Viewport:Unselect(s_ent)
    for i, e_ent in ipairs(self.selection_set) do
        if EntIsEq(s_ent, e_ent) then
            self.volumeWorld.coreWorld.lineGeometry:DeleteComponent(e_ent)
            table.remove(self.selection_set, i)
            return
        end
    end
end

function Viewport:SetSelection(ents)
    self:UnselectAll()
    if #ents == 0 then 
        TransformControls:UpdateTransform(self.selection_set)
        return
    end
    self:AddSelection(ents)
    --SceneBrowser:SyncSelection()
end

function Viewport:CopySelection(history)
    for i = #history.old_ents, 1, -1 do table.remove(history.old_ents, i) end
    
    local new_ents = {}
    for i, ent in ipairs(self.selection_set) do
        new_ents[#new_ents+1] = self.volumeWorld.coreWorld:CopyEntity(ent) -- SOMETHING WRONG on redo!!! 
        if new_ents[#new_ents]:IsNull() then table.remove(new_ents, #new_ents) end
        history.old_ents[i] = ent
    end
    Viewport:SetSelection(new_ents)
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