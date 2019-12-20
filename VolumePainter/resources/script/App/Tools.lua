if not Tools then Tools = {} end

loader.require("App.Properties")
loader.require ("App.VisualizationSettings")

loader.require("App.Brush")
loader.require("App.WorkingPlane")

function Tools.reloadToolbar()
    local root = CoreGui.GetMainRoot()

    if Tools.window then
        root:DetachChild(Tools.window.entity)
        Tools.window.entity:Destroy()
        Tools.window = nil
    end

    Tools.window = Gui.ToolBar()
    root:AttachChild(Tools.window.entity)

    local top_rect = MainWindow.window.entity:GetCorners()
    Tools.window.entity.top = Tools.window.entity.top + top_rect.b
        
	local tool_brush = Tools.window.entity:GetChildById('tool_brush')
	if tool_brush:is_null() then error("Require tool_brush button!") end
	Tools.tool_brush = tool_brush:GetInherited()
	
	local tool_plane = Tools.window.entity:GetChildById('tool_plane')
	if tool_plane:is_null() then error("Require tool_plane button!") end
	Tools.tool_plane = tool_plane:GetInherited()

	local tool_move = Tools.window.entity:GetChildById('tool_move')
    if tool_move:is_null() then error("Require tool_move button!") end
    Tools.tool_move = tool_move:GetInherited()

    local tool_rot = Tools.window.entity:GetChildById('tool_rot')
    if tool_rot:is_null() then error("Require tool_rot button!") end
    Tools.tool_rot = tool_rot:GetInherited()

    local tool_scale = Tools.window.entity:GetChildById('tool_scale')
    if tool_scale:is_null() then error("Require tool_scale button!") end
    Tools.tool_scale = tool_scale:GetInherited()

    if not Viewport or not Viewport.volumeWorld then
		Tools:DeactivateAll()
	end

	local Grect = root:GetRectAbsolute()
    
    if Tools.left_side_area then
        Tools.left_side_area.entity.top = top_rect.b
        Tools.left_side_area.entity:UpdatePosSize()

        local leftRect = Tools.left_side_area.entity:GetRectAbsolute()
        Tools.window.entity.left = leftRect.l - Grect.l + leftRect.w
    end

   --[[ if Tools.right_side_area then
        Tools.right_side_area.entity.top = top_rect.b
        Tools.right_side_area.entity:UpdatePosSize()

        local rightRect = Tools.right_side_area.entity:GetRectAbsolute()
        Tools.window.entity.right = Grect.w - (rightRect.l - Grect.l)
    end
    --]]
    Tools.window.entity:UpdatePosSize()
end

function Tools.reloadSideArea()
    local root = CoreGui.GetMainRoot()

    local reloadProps = false
    local reloadVis = false

	if Tools.left_side_area then -- and Tools.right_side_area
		root:DetachChild(Tools.left_side_area.entity)
        Tools.left_side_area.entity:Destroy()
        Tools.left_side_area = nil

        --root:DetachChild(Tools.right_side_area.entity)
        --Tools.right_side_area.entity:Destroy()
        --Tools.right_side_area = nil
        
        if Properties.window then reloadProps = true end
		if VisualizationSettings.window then reloadVis = true end

        Properties.window = nil
		VisualizationSettings.window = nil
    end

    Tools.left_side_area = Gui.SideArea(true)
    root:AttachChild(Tools.left_side_area.entity)

    --Tools.right_side_area = Gui.SideArea(false)
    --root:AttachChild(Tools.right_side_area.entity)
    
    local top_rect = MainWindow.window.entity:GetCorners()
    Tools.left_side_area.entity.top = top_rect.b
    --Tools.right_side_area.entity.top = top_rect.b

    if reloadProps then Properties.reload() end
	if reloadVis then VisualizationSettings.reload() end

    CoreGui.UpdateLuaFuncs()

    Tools.left_side_area.entity:UpdatePosSize()
    --Tools.right_side_area.entity:UpdatePosSize()
end

function Tools:Init()
    print("Tools:Init") 
    
    self.side_area_separator = {500, 400}

    loader.require("SideArea", Tools.reloadSideArea)
    self.reloadSideArea()
        
    loader.require("ToolBar", Tools.reloadToolbar)
	self.reloadToolbar()

	self.toolMode = TOOL_MODE.BRUSH
	self.cutPlaneState = CUT_PLANE_STATE.VIS_UNCUT

	self.transformEntity = nil
	self.transformInAction = false
	self.transformHover = false
	self.transformPrevRay = Vector3(0,0,0)

	self.brushInAction = false
end

function Tools:DeactivateAll()
	self.tool_brush.entity:Deactivate()
	self.tool_plane.entity:Deactivate()

	self:TransformDeactivateAll()
end

function Tools:ActivateAll()
	self.tool_brush.entity:Activate()
	self.tool_plane.entity:Activate()

	self:SetToolMode(TOOL_MODE.BRUSH)
end

-- MODES
function Tools:SetToolMode(mode)
	if not VolumeWorld.initialized then return end
	self:ModeUnpressAll()
	
	self.toolMode = mode
	if self.toolMode == TOOL_MODE.BRUSH then
		self.transformEntity = nil
		self:SetTransform(TRANSFORM_MODE.NONE)
		self.tool_brush:SetPressed(true)
		self:TransformDeactivateAll()	
		Properties:EnableBrush()
		
	else
		if self.toolMode == TOOL_MODE.PLANE then
			self.transformEntity = WorkingPlane.planeEnt
			self.tool_plane:SetPressed(true)
			Properties:EnablePlane()
		end
		
		self:TransformActivateAll()	
		self:SetTransform(TRANSFORM_MODE.MOVE)
	end

end

function Tools:ModeUnpressAll()
	self.tool_brush:SetPressed(false)
	self.tool_plane:SetPressed(false)
end

-- BRUSH
function Tools:BrushStart(rayPos, rayDir)
	if self.toolMode ~= TOOL_MODE.BRUSH then return end
	self.brushInAction = true
	self:BrushAction(rayPos, rayDir)
end

function Tools:BrushStop()
	self.brushInAction = false

	Brush:StopDraw()
end

function Tools:BrushAction(rayPos, rayDir)
	if self.brushInAction == false then return end

	Brush:DrawBrushFromViewport(rayPos, rayDir)
end

-- TRANSFORM
function Tools:SetTransform(mode)
	TransformControls:SetTransform(mode, {self.transformEntity})
	if self.toolMode == TOOL_MODE.BRUSH then return end
	
	self:TransformUnpressAll()

	if not self.transformEntity then return end

	if mode == TRANSFORM_MODE.MOVE then
		self.tool_move:SetPressed(true)
	elseif mode == TRANSFORM_MODE.ROT then
		self.tool_rot:SetPressed(true)
	elseif mode == TRANSFORM_MODE.SCALE then
		self.tool_scale:SetPressed(true)
	end
end

function Tools:TransformStart(ray)
	if self.transformHover then
		self.transformInAction = true
		self.transformPrevRay = ray
	end
end

function Tools:TransformStop()
	self.transformInAction = false
end

function Tools:TransformAction(rayDir)
	if not self.transformEntity then return end

	if self.transformInAction then
		TransformControls:ApplyTransform(rayDir, self.transformPrevRay, {self.transformEntity})
		self.transformPrevRay = rayDir
	else
		self.transformHover = TransformControls:CheckHover(rayDir)
	end
end

function Tools:TransformUnhover()
	TransformControls:Unhover()
	self.transformHover = false
	self.transformInAction = false
end

function Tools:TransformUnpressAll()
	self.tool_move:SetPressed(false)
	self.tool_rot:SetPressed(false)
	self.tool_scale:SetPressed(false)
end

function Tools:TransformActivateAll()
	self.tool_move.entity:Activate()
	self.tool_rot.entity:Activate()
	self.tool_scale.entity:Activate()
end

function Tools:TransformDeactivateAll()
	self.tool_move.entity:Deactivate()
	self.tool_rot.entity:Deactivate()
	self.tool_scale.entity:Deactivate()
end