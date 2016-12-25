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

    local vp_rect = Viewport.window.entity:GetRectAbsolute()
    Viewport.viewport.entity.width = vp_rect.w + vp_rect.w % 2
    Viewport.viewport.entity.height = vp_rect.h + vp_rect.h % 2

    if Viewport.lua_world then
        local srv = Viewport.lua_world.scenepl:GetSRV()
        Viewport.viewport.rect_mat:SetTexture(srv, 0, SHADERS.PS)
        Viewport.viewport.entity.visible = true
    end
end

function Viewport:Init()
    print("Viewport:Init") 
    
    loader.require("Menus.Viewport")
    loader.require("ViewportWindow", Viewport.reload)
    self.reload()

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

    self.tc_action = false
	self.tc_hover = false
	self.tc_prevray = Vector3(0,0,0)
    self.tc_local = false
    self.tc_copied = false
    
    self.selection_mode = SELECTION_MODE.SIMPLE
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

function Viewport:Tick(dt)
    if not self.lua_world then return end

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

function Viewport:SetWorld(WLD)
    if not WLD.world or WLD.scenepl then return end
    
    local vp_rect = self.viewport.entity:GetRectAbsolute()
    WLD.scenepl = WLD.world:CreateScene(WLD.freecam, vp_rect.w, vp_rect.h, false)
    
    local srv = WLD.scenepl:GetSRV()
    self.viewport.rect_mat:SetTexture(srv, 0, SHADERS.PS)
    self.viewport.entity.visible = true

    WLD.world.active = true

    WLD.world.controller:SendInput(WLD.freecam, CTRL_CMDS.CC_MOVE_SPEED_CHANGE, self.movespeed, 0)
    WLD.world.transformControls.scale = self.arrows_scale

    self.lua_world = WLD

    self:onResize(true)
    self.window.entity:UpdatePosSize()

    MainWindow:SetCaption(WLD.path)
    Tools:ActivateAll()
    Tools:SetTransform(TRANSFORM_MODE.NONE)
    self:SetTransform(TRANSFORM_MODE.NONE)

    self.overlay_gui.enable = true
    local rendermode_btn = self.overlay_gui:GetChildById('vp_rendermode')
    rendermode_btn:GetInherited():SetSelected(1)

    History:Clear()
    self.history_push = false
end

function Viewport:ClearWorld()
    self.lua_world = nil
    self.viewport.rect_mat:ClearTextures()
    self.viewport.entity.visible = false
    
    self.overlay_gui.enable = false

    MainWindow:SetCaption()
    Tools:DeactivateAll()
    Properties:Update()
    AssetBrowser:SetSelected(nil, false, false)
end

function Viewport:SetMode(combo, ev)
    if not self.lua_world then return end

    local mode = combo:GetSelected() - 1

    if mode < 27 then
        self.lua_world.scenepl:SetComponents(true, true, true, true)
        self.lua_world.scenepl.mode = mode
    else
        self.lua_world.scenepl.mode = 0
        if mode == 27 then
            self.lua_world.scenepl:SetComponents(false, false, true, true)
        elseif mode == 28 then
            self.lua_world.scenepl:SetComponents(false, false, false, true)
        elseif mode == 29 then
            self.lua_world.scenepl:SetComponents(true, true, false, false)
        elseif mode == 30 then
            self.lua_world.scenepl:SetComponents(false, true, false, false)
        end
    end
end

-- callbacks
function Viewport:onResize(force)
    if MainWindow.mainwin:IsMinimized() then
        if self.lua_world then self.lua_world.world.active = false end
        --self.viewport.rect_mat:ClearTextures()
        return true 
    else
        if self.lua_world then self.lua_world.world.active = true end
    end

    local vp_rect = self.window.entity:GetRectRelative()
    if self.viewport and self.lua_world then 
        local vp_w = vp_rect.w + vp_rect.w % 2
        local vp_h = vp_rect.h + vp_rect.h % 2

        self.viewport.entity.width = vp_w
        self.viewport.entity.height = vp_h

        if self.sys_win:IsEndResize() or force then
            self.viewport.rect_mat:ClearTextures()
            self.lua_world.scenepl:Resize(vp_w, vp_h)
            local srv = self.lua_world.scenepl:GetSRV()
            self.viewport.rect_mat:SetTexture(srv, 0, SHADERS.PS)

            local rendermode_btn = self.overlay_gui:GetChildById('vp_rendermode')
            self.lua_world.scenepl.mode = rendermode_btn:GetInherited():GetSelected() - 1

            print("Viewport resize to "..vp_w.." x "..vp_h)
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
        local stmodel = EntityTypes.StaticModel(self.lua_world.world)
        self:PlaceAndSelect(stmodel.ent, self.prev_coords)

    elseif ev.id == "vp_create_light" then
        local light = EntityTypes.LocalLight(self.lua_world.world)
        self:PlaceAndSelect(light.ent, self.prev_coords)

    elseif ev.id == "vp_create_glight" then
        local glight = EntityTypes.GlobalLight(self.lua_world.world)
        self:PlaceAndSelect(glight.ent, self.prev_coords)

     elseif ev.id == "vp_dumb" then
        local test_ent = EntityTypes.TestEnt(self.lua_world.world)
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
    self:PlaceArrows()
    Properties:Update()
end

function Viewport:PlaceEntity(entity, mouse_coords)
    local mcoords = self:GetMouseInVP(mouse_coords)

    local cam_dir = self.lua_world.world.camera:GetVectorFromScreen(self.lua_world.freecam, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
    local cam_origin = self.lua_world.world.camera:GetPos(self.lua_world.freecam)
    local coords = self.lua_world.world.visibility:CollideRayCoords(cam_origin, cam_dir, self.lua_world.world.camera:GetFrustumId(self.lua_world.freecam))
    
    local size = self.lua_world.world.visibility:GetBoxSizeL(entity)
    local center = self.lua_world.world.visibility:GetBoxCenterL(entity)
    local y_offset = math.max(0, size.y - center.y)

    self.lua_world.world.transform:SetPosition(entity, coords.x, coords.y + y_offset, coords.z)
    self.lua_world.world.transform:ForceUpdate(entity)
end

function Viewport:onMouseDown(eventData)
    if not self.lua_world then return true end
    
    self.prev_coords.x = eventData.coords.x
    self.prev_coords.y = eventData.coords.y

    if eventData.key == KEYBOARD_CODES.KEY_RBUTTON then
        self.rmouse_down = true
        return true
    end

    if eventData.key == KEYBOARD_CODES.KEY_LBUTTON and self.drawhud then
        self.viewport.entity:SetHierarchyFocusOnMe(true)
		
		if self.tc_hover then
			self.tc_action = true
			
            self.history.transform_type = self.lua_world.world.transformControls.mode
            if self.history.transform_type == TRANSFORM_MODE.MOVE then
			    for i, ent in ipairs(self.selection_set) do
				    self.history.transform_old[i] = self.lua_world.world.transform:GetPositionL(ent)
			    end
		    elseif self.history.transform_type == TRANSFORM_MODE.ROT then
			    for i, ent in ipairs(self.selection_set) do
				    self.history.transform_old[i] = self.lua_world.world.transform:GetRotationL(ent)
			    end
		    elseif self.history.transform_type == TRANSFORM_MODE.SCALE then
			    for i, ent in ipairs(self.selection_set) do
				    self.history.transform_old[i] = self.lua_world.world.transform:GetScaleL(ent)
			    end
		    end

            local mcoords = self:GetMouseInVP(eventData.coords)
			self.tc_prevray = self.lua_world.world.camera:GetVectorFromScreen(self.lua_world.freecam, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
		else		
			self.selection_mode = SELECTION_MODE.SIMPLE
			self:Select(eventData.coords)
		end
        return true
    end
    return true
end

function Viewport:onMouseUp(eventData)
    if not self.lua_world then return true end
    
    if eventData.key == KEYBOARD_CODES.KEY_RBUTTON then
        if self.rmouse_down then
            if self.drawhud then
                self.vp_menu = Gui.ViewportMenu()
                self.viewport:AttachOverlay(self.vp_menu)
                local corners = self.viewport.entity:GetCorners()
                self.vp_menu:Open(eventData.coords.x, eventData.coords.y)
            end

            self.rmouse_down = false
        else
            self:SetFreelook(false)

            self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_FORWARD_END, 0, 0)
            self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_BACK_END, 0, 0)
            self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_LEFT_END, 0, 0)
            self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_RIGHT_END, 0, 0)
            self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_UP_END, 0, 0)
            self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_DOWN_END, 0, 0)

            self.viewport.entity:SetHierarchyFocusOnMe(false)
        end
        return true
    end

    if eventData.key == KEYBOARD_CODES.KEY_LBUTTON then
        self.viewport.entity:SetHierarchyFocusOnMe(false)
		self.tc_action = false
        self.selection_mode = SELECTION_MODE.NONE
        self.tc_copied = false

        self:PushHistory()
        return true
    end
    return true
end

function Viewport:onKeyDown(eventData)
    if not self.lua_world then return false end
    
    if self.freelook then
        if eventData.key == KEYBOARD_CODES.KEY_W then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_FORWARD_START, 0, 0) return true end
        if eventData.key == KEYBOARD_CODES.KEY_S then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_BACK_START, 0, 0) return true end
        if eventData.key == KEYBOARD_CODES.KEY_A then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_LEFT_START, 0, 0) return true end
        if eventData.key == KEYBOARD_CODES.KEY_D then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_RIGHT_START, 0, 0) return true end
        if eventData.key == KEYBOARD_CODES.KEY_E then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_UP_START, 0, 0) return true end
        if eventData.key == KEYBOARD_CODES.KEY_Q then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_DOWN_START, 0, 0) return true end
    end

    if eventData.key == KEYBOARD_CODES.KEY_ESCAPE and self.drawhud then
        self:RememberSelection()
        self:UnselectAll()
        self:PushSelectHistory()
        self:PlaceArrows()
        Properties:Update()
        return true 
    end

    if eventData.key == KEYBOARD_CODES.KEY_DELETE and self.drawhud then -- to history
        self.history.transform_type = TRANSFORM_MODE.NONE
        self.history.msg = "Delete"
        self.history.undo = function(self)
            local ents = {}
            for i = #self.old_ents, 1, -1 do
                ents[#self.old_ents - i + 1] = Viewport.lua_world.world:RestoreEntity()
                -- deserialize from historypool
            end
            Viewport:SetSelection(ents)
        end
        self.history.redo = function(self)
            for i = #Viewport.selection_set, 1, -1 do 
                self.old_ents[#Viewport.selection_set - i + 1] = 1 -- serialize to historypool
                Viewport.lua_world.world:DestroyEntity(Viewport.selection_set[i])
                table.remove(Viewport.selection_set, i) 
            end
		    Viewport:PlaceArrows()
            Properties:Update()
        end

        self.history_push = true
        self.history:redo()
        self:PushHistory()
        return true 
    end

    return false
end

function Viewport:onKeyUp(eventData)
    if not self.lua_world then return true end
    
    if self.freelook then
        if eventData.key == KEYBOARD_CODES.KEY_W then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_FORWARD_END, 0, 0) return end
        if eventData.key == KEYBOARD_CODES.KEY_S then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_BACK_END, 0, 0) return end
        if eventData.key == KEYBOARD_CODES.KEY_A then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_LEFT_END, 0, 0) return end
        if eventData.key == KEYBOARD_CODES.KEY_D then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_RIGHT_END, 0, 0) return end
        if eventData.key == KEYBOARD_CODES.KEY_E then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_UP_END, 0, 0) return end
        if eventData.key == KEYBOARD_CODES.KEY_Q then self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_DOWN_END, 0, 0) return end
    end

    if eventData.key == KEYBOARD_CODES.KEY_CONTROL then 
        self.tc_copied = false
        return 
    end
    return true
end

function Viewport:onMouseMove(eventData)
    if not self.lua_world then return true end
    
    local mouse_pos = {x = eventData.coords.x, y = eventData.coords.y}

    if self.rmouse_down then
        self.rmouse_down = false
        self.viewport.entity:SetHierarchyFocusOnMe(true)
        self:SetFreelook(true)

        local corners = self.viewport.entity:GetCorners()
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
	local ray_dir = self.lua_world.world.camera:GetVectorFromScreen(self.lua_world.freecam, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
	
	if self.tc_action then
        if is_ctrl and not self.tc_copied then
            self:PushHistory()

            self:CopySelection(self.history)
            self.tc_copied = true
            
            self.history_push = true
            self.history.msg = "Copy"
            self.history.undo = function(self)
                    for i = #Viewport.selection_set, 1, -1 do
                        Viewport.lua_world.world:DestroyEntity(Viewport.selection_set[i])
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

            Properties:Update()
        end 

		local tc_mode = self.lua_world.world.transformControls.mode
		
		if tc_mode == TRANSFORM_MODE.NONE then
			self.tc_action = false
		elseif tc_mode == TRANSFORM_MODE.MOVE then		
			local tc_move = self.lua_world.world.transformControls:CalcMove(ray_dir, self.tc_prevray, self.lua_world.freecam)
			for i, ent in ipairs(self.selection_set) do
				self.lua_world.world.transformControls:ApplyMove(tc_move, ent)
			end

            if not self.history_push then
                self.history_push = true
                self.history.msg = "Transform move"
                self.history.undo = function(self) Viewport:SetPositionsToSelection(self.transform_old) end
                self.history.redo = function(self) Viewport:SetPositionsToSelection(self.transform_new) end
            end
		elseif tc_mode == TRANSFORM_MODE.ROT then
			local tc_rot = self.lua_world.world.transformControls:CalcRot(ray_dir, self.tc_prevray, self.lua_world.freecam)
			for i, ent in ipairs(self.selection_set) do
				self.lua_world.world.transformControls:ApplyRot(tc_rot, ent)
			end

            if not self.history_push then
                self.history_push = true
                self.history.msg = "Transform rotate"
                self.history.undo = function(self) Viewport:SetRotationsToSelection(self.transform_old) end
                self.history.redo = function(self) Viewport:SetRotationsToSelection(self.transform_new) end
            end
		elseif tc_mode == TRANSFORM_MODE.SCALE then
			local tc_scale = self.lua_world.world.transformControls:CalcScale(ray_dir, self.tc_prevray, self.lua_world.freecam)
			for i, ent in ipairs(self.selection_set) do
				self.lua_world.world.transformControls:ApplyScale(tc_scale, ent)
			end

            if not self.history_push then
                self.history_push = true
                self.history.msg = "Transform scale"
                self.history.undo = function(self) Viewport:SetScalesToSelection(self.transform_old) end
                self.history.redo = function(self) Viewport:SetScalesToSelection(self.transform_new) end
            end
		end

        Properties:UpdateData(false, COMPONENTS.TRANSFORM)
		
		self.tc_prevray = ray_dir
	end
	
    if self.freelook then
		if not self.tc_action then
			self.lua_world.world.transformControls:Unhover()
			self.tc_hover = false
		end
		
        if self.hidemouse then
            local rect = self.viewport.entity:GetCorners()
            local center_x = (rect.l + rect.r) / 2
            local center_y = (rect.t + rect.b) / 2  
        
            local delta_x = mouse_pos.x - center_x
            local delta_y = -mouse_pos.y + center_y

            self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_DELTA_ROT, delta_x, delta_y)

            CoreGui.SetCursorPos(self.viewport.entity, center_x, center_y)
        else
            local delta_x = mouse_pos.x - self.prev_coords.x
            local delta_y = -mouse_pos.y + self.prev_coords.y

            self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_DELTA_ROT, delta_x, delta_y)

            self.prev_coords.x = mouse_pos.x
            self.prev_coords.y = mouse_pos.y
        end
    else
		if not self.tc_action then
			self.tc_hover = self.lua_world.world.transformControls:CheckHover(ray_dir, self.lua_world.freecam)
		end
    end
    return true
end

function Viewport:onMouseWheel(eventData)
    if not self.lua_world then return true end

    if self.freelook then
        if eventData.coords.x > 0 then
            self.movespeed = self.movespeed * 1.25
            self.movespeed = math.min(self.movespeed, 2.0)
        else
            self.movespeed = self.movespeed * 0.75
            self.movespeed = math.max(self.movespeed, 0.0002)
        end
        self.lua_world.world.controller:SendInput(self.lua_world.freecam, CTRL_CMDS.CC_MOVE_SPEED_CHANGE, self.movespeed, 0)
    end
    return true
end

function Viewport:onItemDroped(eventData)
    local itemCount = CoreGui.DropedItems.GetCount()
    if itemCount == 0 then return true end

    if not self.lua_world then
        if itemCount > 1 then return true end
        
        local worldPath = CoreGui.DropedItems.GetItem(0)
        if string.find(worldPath, ".mls") ~= nil then
            SceneMgr:LoadWorld(worldPath)
        end
    else
        local entities = {}
        for i = 0, itemCount-1 do
            local meshName = CoreGui.DropedItems.GetItem(i)
            print("Droped: " .. meshName)

            local entity = EntityTypes.StaticModel(self.lua_world.world) -- to history
            if not entity:IsAlive() then return true end

            if not entity:SetMesh(meshName) then
                entity:Kill()
                return true
            end

            self:PlaceEntity(entity.ent, eventData.coords)
            table.insert(entities, entity.ent)
        end

        self:RememberSelection()
        self:SetSelection(entities)
        self:PushSelectHistory() -- temp
        self:PlaceArrows()
        Properties:Update()
    end
    return true
end

-- support
function Viewport:PushHistory()
    if not self.history_push then return end

    if self.history.transform_type == TRANSFORM_MODE.MOVE then
	    for i, ent in ipairs(self.selection_set) do
		    self.history.transform_new[i] = self.lua_world.world.transform:GetPositionL(ent)
		end
	elseif self.history.transform_type == TRANSFORM_MODE.ROT then
		for i, ent in ipairs(self.selection_set) do
			self.history.transform_new[i] = self.lua_world.world.transform:GetRotationL(ent)
		end
	elseif self.history.transform_type == TRANSFORM_MODE.SCALE then
		for i, ent in ipairs(self.selection_set) do
		    self.history.transform_new[i] = self.lua_world.world.transform:GetScaleL(ent)
		end
	end
    History:Push(self.history)
    self.history_push = false

    self.history.transform_type = TRANSFORM_MODE.NONE
    for i = #self.history.old_ents, 1, -1 do table.remove(self.history.old_ents, i) end
    for i = #self.history.transform_new, 1, -1 do table.remove(self.history.transform_new, i) end
    for i = #self.history.transform_old, 1, -1 do table.remove(self.history.transform_old, i) end
    
    self.lua_world.unsave = true
end

function Viewport:SetPositionsToSelection(positions)
    for i, ent in ipairs(self.selection_set) do
        if i > #positions then return end
        self.lua_world.world.transform:SetPosition(ent, positions[i].x, positions[i].y, positions[i].z)
        self.lua_world.world.transform:ForceUpdate(ent)
    end
    Properties:UpdateData(false, COMPONENTS.TRANSFORM)
    self:PlaceArrows()
end

function Viewport:SetRotationsToSelection(rotations)
    for i, ent in ipairs(self.selection_set) do
        if i > #rotations then return end
        self.lua_world.world.transform:SetRotation(ent, rotations[i].x, rotations[i].y, rotations[i].z)
        self.lua_world.world.transform:ForceUpdate(ent)
    end
    Properties:UpdateData(false, COMPONENTS.TRANSFORM)
    self:PlaceArrows()
end

function Viewport:SetScalesToSelection(scales)
    for i, ent in ipairs(self.selection_set) do
        if i > #scales then return end
        self.lua_world.world.transform:SetScale(ent, scales[i].x, scales[i].y, scales[i].z)
        self.lua_world.world.transform:ForceUpdate(ent)
    end
    Properties:UpdateData(false, COMPONENTS.TRANSFORM)
    self:PlaceArrows()
end

function Viewport:SwitchHud()
    if not self.lua_world then return end

    self.drawhud = not self.drawhud
    self.lua_world.scenepl:SetHud(self.drawhud)
    self.overlay_gui.enable = self.drawhud
end

function Viewport:SwitchTransform()
    if not self.lua_world then return end

    local cur_mode = self.lua_world.world.transformControls.mode
    cur_mode = cur_mode + 1
    if cur_mode > TRANSFORM_MODE.SCALE then cur_mode = TRANSFORM_MODE.NONE end
    self:SetTransform(cur_mode, true)
end

function Viewport:SetTransform(mode, tools)
    if not self.lua_world then return end

    if mode == TRANSFORM_MODE.NONE then
        self.lua_world.world.transformControls.mode = TRANSFORM_MODE.NONE
        if tools then Tools:SetTransform(TRANSFORM_MODE.NONE) end

    elseif mode == TRANSFORM_MODE.MOVE then
        if self.lua_world.world.transformControls.mode == TRANSFORM_MODE.MOVE then
            self.tc_local = not self.tc_local
        else
            self.lua_world.world.transformControls.mode = TRANSFORM_MODE.MOVE
        end
        self.lua_world.world.transformControls.islocal = self.tc_local
        self:PlaceArrows()
        if tools then Tools:SetTransform(TRANSFORM_MODE.MOVE) end

    elseif mode == TRANSFORM_MODE.ROT then
        if self.lua_world.world.transformControls.mode == TRANSFORM_MODE.ROT then
            self.tc_local = not self.tc_local
        else
            self.lua_world.world.transformControls.mode = TRANSFORM_MODE.ROT
        end
        self.lua_world.world.transformControls.islocal = self.tc_local
        self:PlaceArrows()
        if tools then Tools:SetTransform(TRANSFORM_MODE.ROT) end

    elseif mode == TRANSFORM_MODE.SCALE then
        self.lua_world.world.transformControls.mode = TRANSFORM_MODE.SCALE
        self.lua_world.world.transformControls.islocal = true
        self:PlaceArrows()
        if tools then Tools:SetTransform(TRANSFORM_MODE.SCALE) end
    end
end

function Viewport:Select(coords)
    if self.selection_mode ~= SELECTION_MODE.SNAKE then self:RememberSelection() end

    local mcoords = self:GetMouseInVP(coords)
    local click_dir = self.lua_world.world.camera:GetVectorFromScreen(self.lua_world.freecam, mcoords.x, mcoords.y, mcoords.w, mcoords.h)
    local click_origin = self.lua_world.world.camera:GetPos(self.lua_world.freecam)

    local is_ctrl = CoreGui.Keys.Ctrl()

    if not is_ctrl and self.selection_mode ~= SELECTION_MODE.SNAKE then
        self:UnselectAll()
    end

    local s_ent = self.lua_world.world.visibility:CollideRay(click_origin, click_dir, self.lua_world.world.camera:GetFrustumId(self.lua_world.freecam))
    if not s_ent:IsNull() then
        if is_ctrl then
            for i, e_ent in ipairs(self.selection_set) do
                if EntIsEq(s_ent, e_ent) then
                    if self.selection_mode ~= SELECTION_MODE.SNAKE then
                        self.lua_world.world.lineGeometry:DeleteComponent(e_ent)
                        table.remove(self.selection_set, i)

                        self:PushSelectHistory()
						
						self:PlaceArrows()
                        Properties:Update()
                    end
                    return
                end
            end
        end

        table.insert(self.selection_set, s_ent)
		
		self:PlaceArrows()

        self.lua_world.world.lineGeometry:AddComponent(s_ent)
        self.lua_world.world.lineGeometry:SetFromVis(s_ent)
        self.lua_world.world.lineGeometry:SetColor(s_ent, self.selection_color)
    end
    
    if self.selection_mode ~= SELECTION_MODE.SNAKE then self:PushSelectHistory() end
    Properties:Update()
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
        self.lua_world.world.lineGeometry:DeleteComponent(self.selection_set[i])
        table.remove(self.selection_set, i)
    end
end

function Viewport:AddSelection(ents)
    for i, i_ent in ipairs(ents) do
        table.insert(self.selection_set, i_ent)
        self.lua_world.world.lineGeometry:AddComponent(i_ent)
        self.lua_world.world.lineGeometry:SetFromVis(i_ent)
        self.lua_world.world.lineGeometry:SetColor(i_ent, self.selection_color)
    end
    self:PlaceArrows()
    Properties:Update()
end

function Viewport:SetSelection(ents)
    self:UnselectAll()
    if #ents == 0 then 
        self:PlaceArrows()
        Properties:Update()
        return
    end
    self:AddSelection(ents)
end

function Viewport:CopySelection(history)
    for i = #history.old_ents, 1, -1 do table.remove(history.old_ents, i) end
    
    local new_ents = {}
    for i, ent in ipairs(self.selection_set) do
        new_ents[#new_ents+1] = self.lua_world.world:CopyEntity(ent) -- SOMETHING WRONG on redo!!! 
        if new_ents[#new_ents]:IsNull() then table.remove(new_ents, #new_ents) end
        history.old_ents[i] = ent
    end
    Viewport:SetSelection(new_ents)
end

function Viewport:PlaceArrows()
	if #self.selection_set == 0 then 
		self.lua_world.world.transformControls:Deactivate() 
		return
	end
	
	self.lua_world.world.transformControls:Activate()
	for i, ent in ipairs(self.selection_set) do
		self.lua_world.world.transformControls:SetFromEntity(ent)
	end
end

function Viewport:SetFreelook(look)
    self.freelook = look
    if self.hidemouse then
        local corners = self.viewport.entity:GetCorners()
        local center_x = (corners.l + corners.r) / 2
        local center_y = (corners.t + corners.b) / 2
        CoreGui.SetCursorPos(self.viewport.entity, center_x, center_y)
        CoreGui.ShowCursor(not self.freelook)
    end
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