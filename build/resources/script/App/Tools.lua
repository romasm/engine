loader.require("SideContainers")

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
    Tools.window.entity.top = Tools.window.entity.top + top_rect.b
        
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

    local Grect = root:GetRectAbsolute()
    
    if Tools.left_side_area then
        Tools.left_side_area.entity.top = top_rect.b
        Tools.left_side_area.entity:UpdatePosSize()

        local leftRect = Tools.left_side_area.entity:GetRectAbsolute()
        Tools.window.entity.left = leftRect.l - Grect.l + leftRect.w
    end

    if Tools.right_side_area then
        Tools.right_side_area.entity.top = top_rect.b
        Tools.right_side_area.entity:UpdatePosSize()

        local rightRect = Tools.right_side_area.entity:GetRectAbsolute()
        Tools.window.entity.right = Grect.w - (rightRect.l - Grect.l)
    end
    
    Tools.window.entity:UpdatePosSize()
end

function Tools.reloadSideArea()
    local root = CoreGui.GetMainRoot()

    local reloadProps = false
    local reloadMats = false

    if Tools.left_side_area and Tools.right_side_area then
        root:DetachChild(Tools.left_side_area.entity)
        Tools.left_side_area.entity:Destroy()
        Tools.left_side_area = nil

        root:DetachChild(Tools.right_side_area.entity)
        Tools.right_side_area.entity:Destroy()
        Tools.right_side_area = nil
        
        if Properties.window then reloadProps = true end
        if MaterialProps.window then reloadMats = true end

        Properties.window = nil
        MaterialProps.window = nil
    end

    Tools.left_side_area = Gui.SideArea(true)
    root:AttachChild(Tools.left_side_area.entity)

    Tools.right_side_area = Gui.SideArea(false)
    root:AttachChild(Tools.right_side_area.entity)
    
    local top_rect = MainWindow.window.entity:GetCorners()
    Tools.left_side_area.entity.top = top_rect.b
    Tools.right_side_area.entity.top = top_rect.b

    if reloadProps then Properties.reload() end
    if reloadMats then MaterialProps.reload() end

    CoreGui.UpdateLuaFuncs()

    Tools.left_side_area.entity:UpdatePosSize()
    Tools.right_side_area.entity:UpdatePosSize()
end

function Tools:Init()
    print("Tools:Init") 
    
    self.side_area_separator = {500, 600}

    loader.require("SideArea", Tools.reloadSideArea)
    self.reloadSideArea()
        
    loader.require("ToolBar", Tools.reloadToolbar)
    self.reloadToolbar()

    -- temp

    local sceneBrowser = Gui.SceneBrowserWindow()
    self.right_side_area.entity:AttachChild(sceneBrowser.entity)
    self.right_side_area.second_win = sceneBrowser.entity

    
    loader.require("AssetBrowser")

    local assetBrowser = Gui.AssetBrowserWindow()
    self.left_side_area.entity:AttachChild(assetBrowser.entity)
    self.left_side_area.second_win = assetBrowser.entity
end

function Tools:PostInitProperties()
    self.right_side_area.first_win = self.right_side_area.entity:GetChildById('properties_window')
    if self.right_side_area.first_win:is_null() then error("Require properties_window window!") end

    self.right_side_area.entity:UpdatePosSize()
end

function Tools:PostInitMaterialProps()
    self.left_side_area.first_win = self.left_side_area.entity:GetChildById('material_window')
    if self.left_side_area.first_win:is_null() then error("Require material_window window!") end

    self.left_side_area.entity:UpdatePosSize()
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