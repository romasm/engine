function Gui.ScriptComp()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    
    id = "script",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        height = 23,

        text = {
            str = "Script",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,
})
end

function Gui.ScriptSetBoolean(self, ev)
    local value = self:GetCheck()
    for i, ent in ipairs(Viewport.selection_set) do
        local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
        local varName = self.entity:GetID()
        if lua_entity[varName] ~= nil then
            lua_entity[varName] = value
        end
    end
end

function Gui.ScriptVar_boolean(varName, topOffset)
local res = GuiCheck({
    styles = {GuiStyles.props_check,},
    left = 10,
    top = topOffset,
    width = 120,
    height = 18,
    text = { str = varName:sub(3) },
    id = varName,

    events = {
        [GUI_EVENTS.CB_CHECKED] = Gui.ScriptSetBoolean,
        [GUI_EVENTS.CB_UNCHECKED] = Gui.ScriptSetBoolean,
        [GUI_EVENTS.UPDATE] = function(self, ev)
            local value = false
            for i, ent in ipairs(Viewport.selection_set) do
                local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
                if lua_entity[varName] ~= nil then
                    if i > 1 and value ~= lua_entity[varName] then
                        self:SetCheck(nil)
                        return true
                    else value = lua_entity[varName] end
                end
            end
            self:SetCheck(value)
        end,
    },
})
return res
end

function Gui.ScriptSetNumber(self, ev)
    local value = self:GetValue()
    for i, ent in ipairs(Viewport.selection_set) do
        local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
        local varName = self.entity:GetID()
        if lua_entity[varName] ~= nil then
            lua_entity[varName] = value
        end
    end
end

function Gui.ScriptVar_number(varName, topOffset)
local res = GuiDumb({
    styles = {GuiStyles.live,},
    width = 100,
    width_percent = true,
    top = topOffset,
    height = 20,

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = varName:sub(3),
        left = 10,
        top = 2,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.common_dataslider,
        },

        data = {
            min = 0,
            max = 1,
            decimal = 3,
            overflow_max = true,
            overflow_min = true,
        },
        id = varName,

        left = 110,
        width = 150,
        height = 20,
        
        events = {
            [GUI_EVENTS.SLIDER_START_DRAG] = Gui.ScriptSetNumber,
            [GUI_EVENTS.SLIDER_DRAG] = Gui.ScriptSetNumber,
            [GUI_EVENTS.SLIDER_END_DRAG] = Gui.ScriptSetNumber,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                local value = 0
                for i, ent in ipairs(Viewport.selection_set) do
                    local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
                    if lua_entity[varName] ~= nil then
                        if i > 1 and value ~= lua_entity[varName] then
                            self:SetValue(nil)
                            return true
                        else value = lua_entity[varName] end
                    end
                end
                self:SetValue(value)
            end,
        },
    }),
})
return res
end

function Gui.ScriptVar_string(varName, topOffset)
local res = GuiDumb({
    styles = {GuiStyles.live,},
    width = 100,
    width_percent = true,
    top = topOffset,
    height = 20,

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = varName:sub(3),
        left = 10,
        top = 2,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.text_textfield,
        },
        id = varName,

        top = 0,
        left = 120,
        width = 155,
        height = 20,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev) 
                local value = self:GetText()
                for i, ent in ipairs(Viewport.selection_set) do
                    local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
                    local varName = self.entity:GetID()
                    if lua_entity[varName] ~= nil then
                        lua_entity[varName] = value
                    end
                end 
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                local value = 0
                for i, ent in ipairs(Viewport.selection_set) do
                    local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
                    if lua_entity[varName] ~= nil then
                        if i > 1 and value ~= lua_entity[varName] then
                            self:SetText(nil)
                            return true
                        else value = lua_entity[varName] end
                    end
                end
                self:SetText(value)
            end,
        },
    }),
})
return res
end

function Gui.ScriptVar_function(varName, topOffset)
local res = GuiDumb({
    styles = {GuiStyles.live,},
    width = 100,
    width_percent = true,
    top = topOffset,
    height = 20,

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = varName:sub(3),
        left = 10,
        top = 2,
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.text_textfield,
        },
        id = varName,
        text = {
            length = 1024,
        },

        top = 0,
        left = 120,
        width = 155,
        height = 20,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev) 
                local code = self:GetText()
                local func, errorMsg = load("return function(self, activator, time)"..code.." end", "EditorCode")
                if func == nil then
                    error(errorMsg)
                    return
                end

                for i, ent in ipairs(Viewport.selection_set) do
                    local lua_entity = EntityTypes.wrap(Viewport.lua_world.world, ent)
                    local varName = self.entity:GetID()
                    if lua_entity[varName] ~= nil then
                        lua_entity[varName] = func()
                    end
                    Viewport.lua_world.world:UpdateScript(lua_entity.ent)
                end 
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                -- TODO: load function code somehow (store code in luaEntity.p_code_FUNCNAME)
                self:SetText()
            end,
        },
    }),
})
return res
end