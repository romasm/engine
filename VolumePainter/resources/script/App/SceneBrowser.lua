if not SceneBrowser then SceneBrowser = {} end

function SceneBrowser.reload()
    if SceneBrowser.window then
        SceneBrowser:Deactivate()
        Tools.right_side_area.entity:DetachChild(SceneBrowser.window.entity)
        SceneBrowser.window.entity:Destroy()
        SceneBrowser.window = nil
    end

    SceneBrowser.window = Gui.SceneBrowserWindow()
    Tools.right_side_area.entity:AttachChild(SceneBrowser.window.entity)
    
    Tools.right_side_area.second_win = SceneBrowser.window.entity
    
    SceneBrowser.body = SceneBrowser.window:GetBody().entity
    SceneBrowser.none_msg = SceneBrowser.window:GetClient().entity:GetChildById('none_msg')
    SceneBrowser.selected_counter = SceneBrowser.window.entity:GetChildById('selected_count'):GetInherited()

    SceneBrowser.window.entity:UpdatePosSize()

    Tools.right_side_area.entity:UpdatePosSize()

    if Viewport.volumeWorld then SceneBrowser:Activate(Viewport.volumeWorld.coreWorld) 
    else SceneBrowser:Deactivate() end
end

function SceneBrowser:Init()
    print("SceneBrowser:Init") 
    
    self.entTypes = {"Node", "Camera", "LocalLight", "GlobalLight", "EnvProb", "TestEnt", "StaticModel", "PhysicsModel", "SkinnedMesh", "TestPlayer", "Custom"}--{EDITOR_VARS.TYPE}

    self.filtredList = {}
    self.findstr = ""
    self.entList = {}
    self.topOffset = 0

    self.selectedBtns = {}
    self.prevSelectedNum = 0

    self.world = nil
        
    loader.require("SceneBrowser", SceneBrowser.reload)
    self.reload()
end

function SceneBrowser:Refill()
    self:Clear()
    self.entList = {}

    self.entList = {}
    for i, entType in ipairs(self.entTypes) do
        local currentEnt = self.world:GetFirstEntityByType( entType )
        while not currentEnt:IsNull() do
            local name = self.world:GetEntityName(currentEnt)
            if name:find(EDITOR_VARS.TYPE) == nil and name:find(EDITOR_VARS.INVIS) == nil then
                self.entList[#self.entList + 1] = currentEnt
            end
            currentEnt = self.world:GetNextEntityByType()
        end
    end
    
    self:FillBody()
    self.window.entity:UpdatePosSize()
end

function SceneBrowser:Deactivate()
    self:Clear()
    self.entList = {}
    self.window.entity:Deactivate()
    self.none_msg.enable = true
    self.world = nil
end

function SceneBrowser:Activate(world)
    self.window.entity:Activate()
    self.none_msg.enable = false

    self.world = world
    
    self:Refill()
end

function SceneBrowser:HideEnt(btn)
    local parent = btn.entity:GetParent():GetInherited()
    if self:FindSelected( parent ) == 0 then
        self.world:SetEntityEditorVisible( parent.linked_ent, false )
    else
        for i, selected in ipairs(self.selectedBtns) do
            selected.vis_btn:SetPressed(true)
            self.world:SetEntityEditorVisible( selected.linked_ent, false )
        end
    end
end

function SceneBrowser:ShowEnt(btn)
    local parent = btn.entity:GetParent():GetInherited()
    if self:FindSelected( parent ) == 0 then
        self.world:SetEntityEditorVisible( parent.linked_ent, true )
    else
        for i, selected in ipairs(self.selectedBtns) do
            selected.vis_btn:SetPressed(false)
            self.world:SetEntityEditorVisible( selected.linked_ent, true )
        end
    end
end

function SceneBrowser:Find(str)
    self:Clear()

    self.findstr = CStr.MakeFindMask(str)

    self:FillBody()
    self.window.entity:UpdatePosSize()

    self:SyncSelection()
end

--------------------------------------------------------------------------------SELECTION [

function SceneBrowser:GetSelectedCount()
    return #self.selectedBtns
end

function SceneBrowser:SetSelected(btn)
    for i, selected in ipairs(self.selectedBtns) do
        selected:SetPressed(false)
    end

    self.selectedBtns = {}

    self.prevSelectedNum = tonumber( btn.entity:GetID() )
    self.selectedBtns[1] = btn
    self.selected_counter:SetString( "1 selected" )

    self:SelectEntities({btn.linked_ent}, false)
end

function SceneBrowser:AddSelected(btn)
    self.selectedBtns[#self.selectedBtns + 1] = btn
    self.prevSelectedNum = tonumber( btn.entity:GetID() )
    self.selected_counter:SetString( tostring(#self.selectedBtns) .. " selected" )

    self:SelectEntities({btn.linked_ent}, true)
end

function SceneBrowser:GroupSelected(btn)
    if self.prevSelectedNum == 0 then
        self:AddSelected(btn)
        return
    end

    local nextNum = tonumber( btn.entity:GetID() )

    local fromNum = self.prevSelectedNum
    local toNum = nextNum
    if fromNum > toNum then 
        local temp = fromNum
        fromNum = toNum
        toNum = temp
    end

    local entities = {}
    for i = fromNum, toNum do
        local shiftBtn = self.body:GetChildById( tostring(i) ):GetInherited()
        if self:FindSelected(shiftBtn) == 0 then
            self.selectedBtns[#self.selectedBtns + 1] = shiftBtn
            entities[#entities + 1] = shiftBtn.linked_ent
        end
        shiftBtn:SetPressed(true)
    end

    self.prevSelectedNum = nextNum
    self.selected_counter:SetString( tostring(#self.selectedBtns) .. " selected" )
    
    self:SelectEntities(entities, true)
end

function SceneBrowser:ClearSelected(btn)
    local founded = self:FindSelected(btn)
    if founded == 0 then return end
    
    self:UnselectEntity(self.selectedBtns[founded].linked_ent)

    self.prevSelectedNum = 0
    table.remove(self.selectedBtns, founded)
    self.selected_counter:SetString( tostring(#self.selectedBtns) .. " selected" )
end

function SceneBrowser:FindSelected(btn)
    local founded = 0
    for i, selected in ipairs(self.selectedBtns) do
        if selected.entity:is_eq(btn.entity) then
            founded = i
            break
        end
    end
    return founded
end

function SceneBrowser:SelectEntities(entities, add)
    Viewport:RememberSelection()

    if add ~= true then Viewport:UnselectAll() end
    if #entities > 0 then Viewport:AddSelection(entities) end

    Viewport:PushSelectHistory()
    TransformControls:UpdateTransform(Viewport.selection_set)
    Properties:Update()
end

function SceneBrowser:UnselectEntity(entity)
    Viewport:RememberSelection()
    
    Viewport:Unselect(entity)
    
    Viewport:PushSelectHistory()
    TransformControls:UpdateTransform(Viewport.selection_set)
    Properties:Update()
end

function SceneBrowser:SyncSelection()
    for i, selected in ipairs(self.selectedBtns) do
        selected:SetPressed(false)
    end
    self.selectedBtns = {}
    self.prevSelectedNum = 0

    for i, ent in ipairs(Viewport.selection_set) do -- ERROR: ui entities overflow?
        local entNum = self:GetEntityNum(ent)
        if entNum > 0 then
            local btn = self.body:GetChildById( tostring(entNum) ):GetInherited() 
            self.selectedBtns[#self.selectedBtns + 1] = btn
            btn:SetPressed(true)
        end
    end
    self.selected_counter:SetString( #self.selectedBtns .. " selected" )
end

function SceneBrowser:GetEntityNum(entity)
    local founded = 0
    for i, ent in ipairs(self.filtredList) do
        if EntIsEq(ent[1], entity) then
            founded = i
            break
        end
    end
    return founded
end

--------------------------------------------------------------------------------SELECTION ]

function SceneBrowser:Clear()
    for i = 1, #self.filtredList do
        local btn = self.body:GetChildById( tostring(i) )
        self.body:DetachChild(btn)
        btn:Destroy()
    end
    
    self.selectedBtns = {}
    self.filtredList = {}
    self.findstr = ""
    self.topOffset = 0
end

function SceneBrowser:AddButton(num)
    self.topOffset = self.topOffset + GUI_SCENELIST_SIZE.PADDING

    local name = self.world:GetEntityName( self.filtredList[num][1] )
    local entType = self.world:GetEntityType( self.filtredList[num][1] )
    local btn = Gui.SceneBrowserEntity( name, entType, self.filtredList[num][1], self.topOffset, num )

    self.body:AttachChild( btn.entity )
    self.topOffset = self.topOffset + GUI_SCENELIST_SIZE.Y
    return btn
end

function SceneBrowser:FillBody()
    self.filtredList = {}
    for i, ent in ipairs(self.entList) do
        if self.findstr:len() > 0 then
            local name = self.world:GetEntityName(ent)
            if name:find(self.findstr) ~= nil then 
                local entType = self.world:GetEntityType(ent)
                self.filtredList[#self.filtredList + 1] = {ent, name, entType}
            end
        else
            local name = self.world:GetEntityName(ent)
            local entType = self.world:GetEntityType(ent)
            self.filtredList[#self.filtredList + 1] = {ent, name, entType}
        end
    end

    table.sort(self.filtredList, function (a, b)
        local a_name = a[2]:upper()
        local b_name = b[2]:upper()
        if a_name:len() == 0 and b_name:len() == 0 then 
            a_name = a[3]:upper()
            b_name = b[3]:upper()
        elseif a_name:len() == 0 then
            return false
        elseif b_name:len() == 0 then
            return true
        end
        return a_name:upper() < b_name:upper() 
    end)
    
    self.topOffset = 0
    for i, ent in ipairs(self.filtredList) do
        self:AddButton(i)
    end
    
    self.body.height = self.topOffset
end