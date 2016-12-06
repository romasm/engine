if not Tools then Tools = {} end

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
    Tools.window.entity.top = top_rect.b

    Tools.window.entity:UpdatePosSize()
    
    local tool_none = Tools.window.entity:GetChildById('tool_none')
    if tool_none:is_null() then error("Require tool_none button!") end
    Tools.tool_none = tool_none:GetInherited()

    local tool_move = Tools.window.entity:GetChildById('tool_move')
    if tool_move:is_null() then error("Require tool_move button!") end
    Tools.tool_move = tool_move:GetInherited()

    local tool_rot = Tools.window.entity:GetChildById('tool_rot')
    if tool_rot:is_null() then error("Require tool_rot button!") end
    Tools.tool_rot = tool_rot:GetInherited()

    local tool_scale = Tools.window.entity:GetChildById('tool_scale')
    if tool_scale:is_null() then error("Require tool_scale button!") end
    Tools.tool_scale = tool_scale:GetInherited()

    if not Viewport or not Viewport.lua_world then
        Tools:DeactivateAll()
    else
        Tools:SetTransform(Viewport.lua_world.world.transformControls.mode)
    end

    if Tools.side_area then
        local top_rect = MainWindow.window.entity:GetCorners()
        Tools.side_area.entity.top = top_rect.b
        Tools.side_area.entity:UpdatePosSize()
    end
end

function Tools.reloadSideArea()
    local root = CoreGui.GetMainRoot()

    local reloadProps = false
    local reloadMats = false

    if Tools.side_area then
        root:DetachChild(Tools.side_area.entity)
        Tools.side_area.entity:Destroy()
        Tools.side_area = nil
        
        if Properties.window then reloadProps = true end
        if MaterialProps.window then reloadMats = true end

        Properties.window = nil
        MaterialProps.window = nil
    end

    Tools.side_area = Gui.SideArea()
    root:AttachChild(Tools.side_area.entity)
    
    local top_rect = MainWindow.window.entity:GetCorners()
    Tools.side_area.entity.top = top_rect.b

    if reloadProps then Properties.reload() end
    if reloadMats then MaterialProps.reload() end

    CoreGui.UpdateLuaFuncs()

    Tools.side_area.entity:UpdatePosSize()
end

function Tools:Init()
    print("Tools:Init") 
    
    self.side_area_separator = 400

    loader.require("ToolBar", Tools.reloadToolbar)
    self.reloadToolbar()

    loader.require("SideArea", Tools.reloadSideArea)
    self.reloadSideArea()
end

function Tools:PostInitProperties()
    self.side_area.props_win = self.side_area.entity:GetChildById('properties_window')
    if self.side_area.props_win:is_null() then error("Require properties_window window!") end

    self.side_area.entity:UpdatePosSize()
end

function Tools:PostInitMaterialProps()
    self.side_area.mats_win = self.side_area.entity:GetChildById('material_window')
    if self.side_area.mats_win:is_null() then error("Require material_window window!") end

    self.side_area.entity:UpdatePosSize()
end

function Tools:UnpressAll(exclude)
    if not exclude then
        self.tool_none:SetPressed(false)
        self.tool_move:SetPressed(false)
        self.tool_rot:SetPressed(false)
        self.tool_scale:SetPressed(false)
        return
    end
    
    if not exclude:is_eq(self.tool_none.entity) then
        self.tool_none:SetPressed(false) end
    if not exclude:is_eq(self.tool_move.entity) then
        self.tool_move:SetPressed(false) end
    if not exclude:is_eq(self.tool_rot.entity) then
        self.tool_rot:SetPressed(false) end
    if not exclude:is_eq(self.tool_scale.entity) then
        self.tool_scale:SetPressed(false) end
end

function Tools:DeactivateAll()
    self.tool_none.entity:Deactivate()
    self.tool_move.entity:Deactivate()
    self.tool_rot.entity:Deactivate()
    self.tool_scale.entity:Deactivate()
end

function Tools:ActivateAll()
    self.tool_none.entity:Activate()
    self.tool_move.entity:Activate()
    self.tool_rot.entity:Activate()
    self.tool_scale.entity:Activate()
end

function Tools:SetTransform(mode, exclude)
    Tools:UnpressAll(exclude)
    if mode == TRANSFORM_MODE.NONE then
        Tools.tool_none:SetPressed(true)
    elseif mode == TRANSFORM_MODE.MOVE then
        Tools.tool_move:SetPressed(true)
    elseif mode == TRANSFORM_MODE.ROT then
        Tools.tool_rot:SetPressed(true)
    elseif mode == TRANSFORM_MODE.SCALE then
        Tools.tool_scale:SetPressed(true)
    end
end