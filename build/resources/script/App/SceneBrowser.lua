if not SceneBrowser then SceneBrowser = {} end

function SceneBrowser.reload()
    if SceneBrowser.window then
        SceneBrowser:Clear()
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

    if SceneBrowser.active == true then SceneBrowser:Activate() 
    else SceneBrowser:Deactivate() end
end

function SceneBrowser:Init()
    print("SceneBrowser:Init") 
    
    self.filtredList = {}
    self.findstr = ""
    self.entList = {}
    self.topOffset = 0

    self.selectedBtns = {}
    self.prevSelectedNum = 0

    self.active = false
        
    loader.require("SceneBrowser", SceneBrowser.reload)
    self.reload()
end

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
end

function SceneBrowser:AddSelected(btn)
    self.selectedBtns[#self.selectedBtns + 1] = btn
    self.prevSelectedNum = tonumber( btn.entity:GetID() )
    self.selected_counter:SetString( tostring(#self.selectedBtns) .. " selected" )
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

    for i = fromNum, toNum do
        local shiftBtn = self.body:GetChildById( tostring(i) ):GetInherited()
        if self:FindSelected(shiftBtn) == 0 then
            self.selectedBtns[#self.selectedBtns + 1] = shiftBtn
        end
        shiftBtn:SetPressed(true)
    end

    self.prevSelectedNum = nextNum
    self.selected_counter:SetString( tostring(#self.selectedBtns) .. " selected" )
end

function SceneBrowser:DeleteSelected(btn)
    local founded = self:FindSelected(btn)
    if founded == 0 then return end
    
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

function SceneBrowser:Deactivate()
    self:Clear()
    self.window.entity:Deactivate()
    self.none_msg.enable = true
end

function SceneBrowser:Activate()
    self.window.entity:Activate()
    self.none_msg.enable = false

    --temp
    self.entList = {}
    for i = 1, 20 do
        self.entList[#self.entList + 1] = "Test"
    end

    self:FillBody()
    self.window.entity:UpdatePosSize()
end

function SceneBrowser:Clear()
    for i = 1, #self.filtredList do
        local btn = self.body:GetChildById( tostring(i) )
        self.body:DetachChild(btn)
        btn:Destroy()
    end
    
    self.selectedBtns = {}
    self.filtredList = {}
    self.findstr = ""
    self.entList = {}
    self.topOffset = 0
end

function SceneBrowser:AddButton(num)
    self.topOffset = self.topOffset + GUI_SCENELIST_SIZE.PADDING
    local btn = Gui.SceneBrowserEntity( self.filtredList[num], "Static model", 0, self.topOffset, num )

    self.body:AttachChild( btn.entity )
    self.topOffset = self.topOffset + GUI_SCENELIST_SIZE.Y
    return btn
end

function SceneBrowser:FillBody()
    self.filtredList = {}
    for i, ent in ipairs(self.entList) do
        if self.findstr:len() > 0 then
            if ent:find(self.findstr) ~= nil then 
                self.filtredList[#self.filtredList + 1] = ent
            end
        else
            self.filtredList[#self.filtredList + 1] = ent
        end
    end

    table.sort(self.filtredList, function (a, b) return a:upper() < b:upper() end)
    
    self.topOffset = 0
    for i, ent in ipairs(self.filtredList) do
        self:AddButton(i)
    end
    
    self.body.height = self.topOffset
end