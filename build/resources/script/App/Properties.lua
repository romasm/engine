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

    self.reload()

    self.update_time = 0
    self.update_transf_need = false
    self.update_light_need = false
    self.update_Glight_need = false
    self.update_stmesh_need = false
    self.update_script_need = false
    self.update_collision_need = false
    self.update_envprob_need = false
end

function Properties:Tick(dt)
    self.update_time = self.update_time + dt

    if self.update_time < COMMON_UPDATE_WAIT then return end
    self.update_time = 0

    local update_need = self.update_transf_need or self.update_light_need or self.update_Glight_need or self.update_stmesh_need or self.update_script_need
    if update_need then
        local ev = HEvent()
        ev.event = GUI_EVENTS.UPDATE

        if self.update_transf_need and Properties.transf_gr then Properties.transf_gr.entity:SendEvent(ev) end 
        if self.update_light_need and Properties.light_gr then Properties.light_gr.entity:SendEvent(ev) end 
        if self.update_Glight_need and Properties.Glight_gr then Properties.Glight_gr.entity:SendEvent(ev) end 
        if self.update_stmesh_need and Properties.stmesh_gr then Properties.stmesh_gr.entity:SendEvent(ev) end                         
        if self.update_script_need and Properties.script_gr then Properties.script_gr.entity:SendEvent(ev) end                         
        if self.update_collision_need and Properties.collision_gr then Properties.collision_gr.entity:SendEvent(ev) end                  
        if self.update_envprob_need and Properties.envprob_gr then Properties.envprob_gr.entity:SendEvent(ev) end                       

        self.update_transf_need = false
        self.update_light_need = false
        self.update_Glight_need = false
        self.update_stmesh_need = false
        self.update_script_need = false
        self.update_collision_need = false
        self.update_envprob_need = false
    end
end

function Properties:Update()
    self.body:ClearGroups()
    self.transf_gr = nil
    self.light_gr = nil
    self.Glight_gr = nil
    self.stmesh_gr = nil
    self.script_gr = nil
    self.collision_gr = nil
    self.envprob_gr = nil
    
    if not Viewport.selection_set or #Viewport.selection_set == 0 or not Viewport.lua_world then
        self.none_msg.enable = true
        return 
    end

    local has_transform = true
    local has_stmesh = true
    local has_light = true
    local has_globallight = true
    local has_script = true
    local has_collision = true
    local has_envprob = true

    local ent_type = nil
    local lua_entity = nil
    
    for i, ent in ipairs(Viewport.selection_set) do
        has_transform = has_transform and Viewport.lua_world.world.transform:HasComponent(ent)

        has_light = has_light and Viewport.lua_world.world.light:HasComponent(ent)
        has_globallight = has_globallight and Viewport.lua_world.world.globalLight:HasComponent(ent)
        has_stmesh = has_stmesh and Viewport.lua_world.world.staticMesh:HasComponent(ent)
        has_collision = has_collision and Viewport.lua_world.world.collision:HasComponent(ent)
        has_envprob = has_envprob and Viewport.lua_world.world.envprobs:HasComponent(ent)
        
        if has_script then
            lua_entity = Viewport.lua_world.world.script:GetLuaEntity(ent)
            if lua_entity == nil then 
                has_script = false
            else
                local current_type = Viewport.lua_world.world:GetEntityType(ent)
                if ent_type == nil then 
                    ent_type = current_type
                else
                    if ent_type ~= current_type then has_script = false end
                end
            end
        end
    end
    
    if has_transform then
        self.transf_gr = Gui.TransformComp()
        self.body:AddGroup(self.transf_gr)
    end

    if has_light then
        self.light_gr = Gui.LightComp()
        self.body:AddGroup(self.light_gr)
    end

    if has_globallight then
        self.Glight_gr = Gui.GlobalLightComp()
        self.body:AddGroup(self.Glight_gr)
    end

    if has_stmesh then
        self.stmesh_gr = Gui.StaticMeshComp()
        self.body:AddGroup(self.stmesh_gr)
    end
    
    if has_collision then
        self.collision_gr = Gui.CollisionComp()
        self.body:AddGroup(self.collision_gr)
    end
    
    if has_envprob then
        self.envprob_gr = Gui.EnvProbComp()
        self.body:AddGroup(self.envprob_gr)
    end

    if has_script and ent_type ~= nil then
        local varsCount = Viewport.lua_world.world.script:GetLuaVarsCount(lua_entity.ent)
        local varsNames = {}
        for i = 0, varsCount - 1 do
            varsNames[#varsNames + 1] = Viewport.lua_world.world.script:GetLuaVarName(lua_entity.ent, i)
        end

        -- TODO: sort vars
        
        self.script_gr = Gui.ScriptComp()
        
        local topOffset = 33
        for j = 1, #varsNames do
            local varType = type(lua_entity[varsNames[j]])
            local varValue = lua_entity[varsNames[j]]

            local varGuiFunc = Gui["ScriptVar_"..varType]
            if varGuiFunc ~= nil then
                local varGui = varGuiFunc(varsNames[j], topOffset)
                topOffset = topOffset + varGui.entity.height + 10
                self.script_gr.entity:AttachChild(varGui.entity)   
            end         
        end

        local groupH = topOffset + 10
        self.script_gr.entity.height = groupH
        self.script_gr.opened_h = groupH
        self.body:AddGroup(self.script_gr)
    end
    
    self:UpdateData(true)
    self.none_msg.enable = false
end

function Properties:UpdateData(force, comp)
    if not force then
        self.update_transf_need = self.transf_gr ~= nil
        self.update_light_need = self.light_gr ~= nil
        self.update_Glight_need = self.Glight_gr ~= nil
        self.update_stmesh_need = self.stmesh_gr ~= nil
        self.update_script_need = self.script_gr ~= nil
        self.update_collision_need = self.collision_gr ~= nil
        self.update_envprob_need = self.envprob_gr ~= nil
        if comp ~= nil then
            if comp ~= COMPONENTS.TRANSFORM then self.update_transf_need = false end
            if comp ~= COMPONENTS.LIGHT then self.update_light_need = false end
            if comp ~= COMPONENTS.GLIGHT then self.update_Glight_need = false end
            if comp ~= COMPONENTS.STATIC then self.update_stmesh_need = false end
            if comp ~= COMPONENTS.SCRIPT then self.update_script_need = false end
            if comp ~= COMPONENTS.COLLISION then self.update_collision_need = false end
            if comp ~= COMPONENTS.ENVPROB then self.update_envprob_need = false end
        end
        return
    end

    local ev = HEvent()
    ev.event = GUI_EVENTS.UPDATE

    if comp == nil then
        if self.transf_gr then self.transf_gr.entity:SendEvent(ev) end
        if self.light_gr then self.light_gr.entity:SendEvent(ev) end
        if self.Glight_gr then self.Glight_gr.entity:SendEvent(ev) end
        if self.stmesh_gr then self.stmesh_gr.entity:SendEvent(ev) end
        if self.script_gr then self.script_gr.entity:SendEvent(ev) end
        if self.collision_gr then self.collision_gr.entity:SendEvent(ev) end
        if self.envprob_gr then self.envprob_gr.entity:SendEvent(ev) end
    else
        if comp == COMPONENTS.TRANSFORM then 
            if self.transf_gr then  self.transf_gr.entity:SendEvent(ev) end
        elseif comp == COMPONENTS.LIGHT then 
            if self.light_gr then self.light_gr.entity:SendEvent(ev) end
        elseif comp == COMPONENTS.GLIGHT then 
            if self.Glight_gr then self.Glight_gr.entity:SendEvent(ev) end
        elseif comp == COMPONENTS.STATIC then 
            if self.stmesh_gr then self.stmesh_gr.entity:SendEvent(ev) end
        elseif comp == COMPONENTS.SCRIPT then 
            if self.script_gr then self.script_gr.entity:SendEvent(ev) end
        elseif comp == COMPONENTS.COLLISION then 
            if self.collision_gr then self.collision_gr.entity:SendEvent(ev) end
        elseif comp == COMPONENTS.ENVPROB then 
            if self.envprob_gr then self.envprob_gr.entity:SendEvent(ev) end
        end
    end

    self.update_time = 0
    self.update_transf_need = false
    self.update_light_need = false
    self.update_Glight_need = false
    self.update_stmesh_need = false
    self.update_script_need = false
    self.update_collision_need = false
    self.update_envprob_need = false
end