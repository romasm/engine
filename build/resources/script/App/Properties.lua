if not Properties then Properties = {} end

function Properties.reloadComponents()
    Properties:Update()
end

loader.require("EntityProps")

loader.require("ComponentsGui.Transform", Properties.reloadComponents)
loader.require("ComponentsGui.Light", Properties.reloadComponents)
loader.require("ComponentsGui.GlobalLight", Properties.reloadComponents)
loader.require("ComponentsGui.StaticMesh", Properties.reloadComponents)
loader.require("ComponentsGui.Script", Properties.reloadComponents)
loader.require("ComponentsGui.Collision", Properties.reloadComponents)
loader.require("ComponentsGui.EnvProb", Properties.reloadComponents)

loader.require("Menus.AddComp")

function Properties.reload()
    if Properties.window then
        Tools.right_side_area.entity:DetachChild(Properties.window.entity)
        Properties.window.entity:Destroy()
        Properties.window = nil
    end

    Properties.window = Gui.EntityPropsWindow()
    Tools.right_side_area.entity:AttachChild(Properties.window.entity)
    
    Tools.right_side_area.first_win = Properties.window.entity

    Properties.window.entity:UpdatePosSize()

    local body = Properties.window:GetBody().entity
    Properties.body = body:GetInherited()
    Properties.none_msg = Properties.window.entity:GetChildById('none_msg')
    Properties:Update()

    Tools.right_side_area.entity:UpdatePosSize()
end

function Properties:Init()
    print("Properties:Init") 
    
    self.updateTime = 0	
	self.compGroups = {}	
	self.updateNeed = {}
	
	for i = 1, COMPONENTS.MAX - 1 do
		self.updateNeed[#self.updateNeed + 1] = false
	end
    
    self.reload()
end

function Properties:Tick(dt)
    self.updateTime = self.updateTime + dt

    if self.updateTime < COMMON_UPDATE_WAIT then return end
    self.updateTime = 0

	local ev = HEvent()
	ev.event = GUI_EVENTS.UPDATE
	
	for i = 1, #self.updateNeed do
		if self.updateNeed[i] == true and self.compGroups[i] ~= nil then
			self.compGroups[i].entity:SendEvent(ev)
		end
	end
end

function Properties:Update()
    self.body:ClearGroups()
	
	for i, gr in pairs(self.compGroups) do
		self.compGroups[i] = nil
	end
    
    if not Viewport.selection_set or #Viewport.selection_set == 0 or not Viewport.lua_world then
        self.none_msg.enable = true
        return 
    end

    local hasComp = {}
	for i = 1, COMPONENTS.MAX - 1 do
		hasComp[#hasComp + 1] = true
	end	

    local entType = nil
    local luaEntity = nil
    
    for i, ent in ipairs(Viewport.selection_set) do
        hasComp[COMPONENTS.TRANSFORM] = hasComp[COMPONENTS.TRANSFORM] and Viewport.lua_world.world.transform:HasComponent(ent)
        hasComp[COMPONENTS.LIGHT] = hasComp[COMPONENTS.LIGHT] and Viewport.lua_world.world.light:HasComponent(ent)
        hasComp[COMPONENTS.GLIGHT] = hasComp[COMPONENTS.GLIGHT] and Viewport.lua_world.world.globalLight:HasComponent(ent)
        hasComp[COMPONENTS.STATIC] = hasComp[COMPONENTS.STATIC] and Viewport.lua_world.world.staticMesh:HasComponent(ent)
        hasComp[COMPONENTS.COLLISION] = hasComp[COMPONENTS.COLLISION] and Viewport.lua_world.world.collision:HasComponent(ent)
        hasComp[COMPONENTS.ENVPROB] = hasComp[COMPONENTS.ENVPROB] and Viewport.lua_world.world.envprobs:HasComponent(ent)
        
        if hasComp[COMPONENTS.SCRIPT] then
            luaEntity = Viewport.lua_world.world.script:GetLuaEntity(ent)
            if luaEntity == nil then 
                has_script = false
            else
                local currentType = Viewport.lua_world.world:GetEntityType(ent)
                if entType == nil then 
                    entType = currentType
                else
                    if entType ~= currentType then hasComp[COMPONENTS.SCRIPT] = false end
                end
            end
        end
    end
    
	for i, has in ipairs(hasComp) do
		if has then
			if i ~= COMPONENTS.SCRIPT then
				self.compGroups[i] = Gui[COMPONENTS_NAMES[i] .. "Comp"]()
				self.body:AddGroup(self.compGroups[i])
			else
				if entType ~= nil then
					local varsCount = Viewport.lua_world.world.script:GetLuaVarsCount(luaEntity.ent)
					local varsNames = {}
					for i = 0, varsCount - 1 do
						varsNames[#varsNames + 1] = Viewport.lua_world.world.script:GetLuaVarName(luaEntity.ent, i)
					end
					
					-- TODO: sort vars
					
					self.compGroups[i] = Gui[COMPONENTS_NAMES[i] .. "Comp"]()
					
					local topOffset = 33
					for j = 1, #varsNames do
						local varType = type(luaEntity[varsNames[j]])
						local varValue = luaEntity[varsNames[j]]

						local varGuiFunc = Gui["ScriptVar_"..varType]
						if varGuiFunc ~= nil then
							local varGui = varGuiFunc(varsNames[j], topOffset)
							topOffset = topOffset + varGui.entity.height + 10
							self.compGroups[i].entity:AttachChild(varGui.entity)   
						end         
					end

					local groupH = topOffset + 10
					self.compGroups[i].entity.height = groupH
					self.compGroups[i].opened_h = groupH
					self.body:AddGroup(self.compGroups[i])
				end
			end
		end
	end
    
    self:UpdateData(true)
    self.none_msg.enable = false
end

function Properties:UpdateData(force, comp)
    if not force then
        for i = 1, #self.updateNeed do
            self.updateNeed[i] = self.compGroups[i] ~= nil
            if comp ~= i then self.updateNeed[i] = false end
	    end
        return
    end

    local ev = HEvent()
    ev.event = GUI_EVENTS.UPDATE

    if comp == nil then
        for i = 1, COMPONENTS.MAX - 1 do
            if self.compGroups[i] ~= nil then self.compGroups[i].entity:SendEvent(ev) end
	    end
    else
        if self.compGroups[comp] ~= nil then self.compGroups[comp].entity:SendEvent(ev) end
    end

    self.updateTime = 0
    for i = 1, #self.updateNeed do
		self.updateNeed[i] = false
	end
end

function Properties:OpenAddCompMenu(btn)
    if self.addCompMenu ~= nil then return true end

	self.addCompMenu = Gui.AddCompMenu()
    btn:AttachOverlay(self.addCompMenu)
		
	local corners = btn.entity:GetCorners()
    self.addCompMenu:Open(corners.l, corners.b)
    
	-- TODO: set allowed comps
	
    return true
end

function Properties:AddMenuClose(btn)
    if self.addCompMenu == nil then return true end
    self.addCompMenu = nil	
	return btn:SetPressed(false) 
end

function Properties:AddMenuClick(btn, ev)
    self:AddMenuClose(btn)
    
    if ev.id == "ac_transform" then
		
    elseif ev.id == "ac_vis" then
		
	end
	return true
end