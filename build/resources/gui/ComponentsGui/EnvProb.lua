function Gui.EnvProbComp()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    
    id = "envprob",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 21,

        text = {
            str = "Env Prob",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 230,

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Parallax shape",
        left = 10,
        top = 33,
    }),
    
    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = true,
        left = 120,
        top = 30,
        width = 155,
        height = 21,
        list = {
            "None",
            "Sphere",
            "Box",
        },
        alt = "Parallax shape of the baked geometry",

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev)
                local selected = self:GetSelected()
                if selected <= 0 then return true end
                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.envprobs:SetType(ent, selected - 1)
                end
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                    local value = 0
                    for i, ent in ipairs(Viewport.selection_set) do
                        local tp = Viewport.lua_world.world.envprobs:GetType(ent)
                        if i > 1 and value ~= tp then 
                            self:SetSelected(0)
                            return true
                        else value = tp end
                    end
                    self:SetSelected( value + 1 )
                return true 
            end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Fade distance, m",
        left = 10,
        top = 58,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.common_dataslider,
        },
        top = 55,
        left = 120,
        width = 155,
        height = 20,
        data = {
            min = 0.01,
            max = 1.0,
            overflow_max = true,
        },
        alt = "Fade out distance from cubemap border, in meters",

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
                local value = self:GetValue()
                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.envprobs:SetFade(ent, value)
                end
                return true
            end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
                local value = self:GetValue()
                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.envprobs:SetFade(ent, value)
                end
                return true
            end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev)
                local value = self:GetValue()
                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.envprobs:SetFade(ent, value)
                end
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                    local value = 0
                    for i, ent in ipairs(Viewport.selection_set) do
                        local tp = Viewport.lua_world.world.envprobs:GetFade(ent)
                        if i > 1 and value ~= tp then 
                            self:SetValue()
                            return true
                        else value = tp end
                    end
                    self:SetValue( value )
                return true 
            end,
        },
    }),
    
    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Priority",
        left = 10,
        top = 83,
    }),
    
    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.int_textfield,
        },
        data = {
            min = 0,
            max = 64,
        },
        left = 120,
        top = 80,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev)
                local value = self:GetNum()
                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.envprobs:SetPriority(ent, value)
                end
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                    local value = 0
                    for i, ent in ipairs(Viewport.selection_set) do
                        local tp = Viewport.lua_world.world.envprobs:GetPriority(ent)
                        if i > 1 and value ~= tp then 
                            self:SetNum()
                            return true
                        else value = tp end
                    end
                    self:SetNum( value )
                return true 
            end,
        },
    }),

    -- BAKING

    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Quality",
        left = 10,
        top = 113,
    }),
    
    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = true,
        left = 120,
        top = 110,
        width = 155,
        height = 21,
        list = {
            "High",
            "Standart",
            "Low",
        },
        alt = "Baking / Storage quality",

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev)
                local selected = self:GetSelected()
                if selected <= 0 then return true end
                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.envprobs:SetQuality(ent, selected - 1)
                end
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                    local value = 0
                    for i, ent in ipairs(Viewport.selection_set) do
                        local tp = Viewport.lua_world.world.envprobs:GetQuality(ent)
                        if i > 1 and value ~= tp then 
                            self:SetSelected(0)
                            return true
                        else value = tp end
                    end
                    self:SetSelected( value + 1 )
                return true 
            end,
        },
    }),
    
    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Nearclip (Bake), m",
        left = 10,
        top = 138,
    }),
    
    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },
        left = 120,
        top = 135,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev)
                local value = self:GetNum()
                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.envprobs:SetNearClip(ent, value)
                end
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                    local value = 0
                    for i, ent in ipairs(Viewport.selection_set) do
                        local tp = Viewport.lua_world.world.envprobs:GetNearClip(ent)
                        if i > 1 and value ~= tp then 
                            self:SetNum()
                            return true
                        else value = tp end
                    end
                    self:SetNum( value )
                return true 
            end,
        },
    }),
    
    GuiString({
        styles = {GuiStyles.string_props_03,},
        str = "Farclip (Bake), m",
        left = 10,
        top = 163,
    }),
    
    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.float_textfield,
        },
        left = 120,
        top = 160,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev)
                local value = self:GetNum()
                for i, ent in ipairs(Viewport.selection_set) do 
                    Viewport.lua_world.world.envprobs:SetFarClip(ent, value)
                end
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev)
                    local value = 0
                    for i, ent in ipairs(Viewport.selection_set) do
                        local tp = Viewport.lua_world.world.envprobs:GetFarClip(ent)
                        if i > 1 and value ~= tp then 
                            self:SetNum()
                            return true
                        else value = tp end
                    end
                    self:SetNum( value )
                return true 
            end,
        },
    }),
  
    GuiButton({
        styles = {GuiStyles.colorwin_button,},
        top = 190,
        left = 120,
        id = 'bake_btn',
        align = GUI_ALIGN.LEFT,
        width = 155,
        height = 30,
        text = {
            font = "../resources/fonts/opensans_normal_18px",
            str = "Rebake",
        },
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.envprobs:Bake(ent)
                    Viewport.lua_world.world:UpdateEnvProbRenderData(ent)
                end
                return true
            end,
        },
    }),
    
})
end