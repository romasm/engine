GuiStyles.rendercfg_check = {
    styles = {
        GuiStyles.props_check,
    },
    width = 150,
    height = 18,
}

function Gui.RenderConfig()
return GuiRect({
    styles = {
        GuiStyles.live,
    },
    width = 400,
    height = 400,
    align = GUI_ALIGN.RIGHT,
    top = 33,
    right = 4,
    background = {
        color = 'bg_05_a6',
    },
    id = 'render_config',

     events = {
        [GUI_EVENTS.MOUSE_DOWN] = function(self, ev) 
            return true
        end,
        [GUI_EVENTS.UNFOCUS] = function(self, ev) 
            if not self.entity:is_eq(ev.entity) then return false end
            return Viewport:CloseRenderConfig()
        end,
        [GUI_EVENTS.KEY_DOWN] = function(self, ev) 
            if ev.key == KEYBOARD_CODES.KEY_ESCAPE or
               ev.key == KEYBOARD_CODES.KEY_RETURN then 
                return Viewport:CloseRenderConfig()
            end
            return false
        end,
    },

    -- BUFFERS
    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_18,
        },
        color = 'act_02',
        str = "Buffer visualization",
        static = true,
        top = 10,
        left = 10,
    }),

    GuiCheckGroup({
        styles = {
            GuiStyles.live,
        },
        top = 35,
        left = 20,
        width = 190,
        height = 260,
        id = 'buffers_cbg',

        events = {
            [GUI_EVENTS.CBGROUP_CHECK] = function(self, ev) 
                local voxel_cb = self.entity:GetParent():GetChildById('voxel_cb')
                voxel_cb:GetInherited():SetCheck(false)

                local checkID = self:GetCheck()
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.bufferViewMode = checkID - 1
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetCheck( renderConfig.bufferViewMode + 1 )
                return true
            end,
        },
          
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 0,
            text = { str = "None" },
            alt = "Draw full scene",
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 28,
            text = { str = "Albedo" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 51,
            text = { str = "Specular" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 74,
            text = { str = "Roughness" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 97,
            text = { str = "Normal" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 120,
            text = { str = "Emissive" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 143,
            text = { str = "Subsurface" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 166,
            text = { str = "Thickness" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 189,
            text = { str = "Ambient occlusion" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 212,
            text = { str = "Depth" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 235,
            text = { str = "SSR" },
        }),
    }),

    -- VOXELS -- TODO: change shader logic
    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_18,
        },
        color = 'act_02',
        str = "Voxels visualization",
        static = true,
        top = 295,
        left = 10,
    }),

    GuiCheck({
        styles = {GuiStyles.rendercfg_check,},
        top = 320,
        left = 20,
        text = { str = "Draw voxels" },
        id = 'voxel_cb',

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) 
                local buffers_cbg = self.entity:GetParent():GetChildById('buffers_cbg')
                buffers_cbg:GetInherited():SetCheck(1)

                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.bufferViewMode = 15
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.bufferViewMode = 0
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetCheck( renderConfig.bufferViewMode >= 15 )
                return true
            end,
        },
    }),

    GuiCheck({
        styles = {GuiStyles.rendercfg_check,},
        top = 343,
        left = 20,
        text = { str = "Emittance" },
        alt = "If checked emittance voxels are rendering, otherwise opacity voxels",
        id = 'emitt_cb',

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                if renderConfig.bufferViewMode < 15 then return true end
                renderConfig.bufferViewMode = renderConfig.bufferViewMode + 6
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                if renderConfig.bufferViewMode < 15 then return true end
                renderConfig.bufferViewMode = renderConfig.bufferViewMode - 6
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetCheck( renderConfig.bufferViewMode >= 21 )
                return true
            end,
        },
    }),

    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_18,
        },
        color = 'act_02',
        str = "First cascade:",
        static = true,
        top = 366,
        left = 20,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.common_dataslider,
        },
        left = 110,
        width = 80,
        height = 20,
        top = 366,
        data = {
            min = 0,
            max = 5,
            decimal = 0,
            step = 1
        },
        alt = "Number of first voxel cascade",

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
                local cascade = self:GetValue()    
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                if renderConfig.bufferViewMode < 15 then return true 
                elseif renderConfig.bufferViewMode < 21 then
                    renderConfig.bufferViewMode = 15 + cascade
                else
                    renderConfig.bufferViewMode = 21 + cascade
                end
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
                local cascade = self:GetValue()    
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                if renderConfig.bufferViewMode < 15 then return true 
                elseif renderConfig.bufferViewMode < 21 then
                    renderConfig.bufferViewMode = 15 + cascade
                else
                    renderConfig.bufferViewMode = 21 + cascade
                end
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) 
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                if renderConfig.bufferViewMode < 15 then self:SetValue(0)
                elseif renderConfig.bufferViewMode < 21 then self:SetValue(renderConfig.bufferViewMode - 15)
                else self:SetValue(renderConfig.bufferViewMode - 21)
                end
            end,
        },
    }),

    -- LIGHTING
    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_18,
        },
        color = 'act_02',
        str = "Lighting components",
        static = true,
        top = 10,
        left = 210,
    }),

    GuiCheck({
        styles = {GuiStyles.rendercfg_check,},
        top = 35,
        left = 220,
        text = { str = "Direct diffuse" },
        alt = "Diffuse component of direct lighting",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.analyticLightDiffuse = 1
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.analyticLightDiffuse = 0
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetCheck( renderConfig.analyticLightDiffuse > 0 )
                return true
            end,
        },
    }),

    GuiCheck({
        styles = {GuiStyles.rendercfg_check,},
        top = 58,
        left = 220,
        text = { str = "Direct specular" },
        alt = "Specular component of direct lighting",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.analyticLightSpecular = 1
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.analyticLightSpecular = 0
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetCheck( renderConfig.analyticLightSpecular > 0 )
                return true
            end,
        },
    }),

    GuiCheck({
        styles = {GuiStyles.rendercfg_check,},
        top = 86,
        left = 220,
        text = { str = "Indirect diffuse" },
        alt = "Diffuse component of indirect lighting",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.ambientLightDiffuse = 1
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.ambientLightDiffuse = 0
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetCheck( renderConfig.ambientLightDiffuse > 0 )
                return true
            end,
        },
    }),

    GuiCheck({
        styles = {GuiStyles.rendercfg_check,},
        top = 109,
        left = 220,
        text = { str = "Indirect specular" },
        alt = "Specular component of indirect lighting",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.ambientLightSpecular = 1
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.ambientLightSpecular = 0
                Viewport.lua_world.scenepl:ApplyConfig()
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetCheck( renderConfig.ambientLightSpecular > 0 )
                return true
            end,
        },
    }),
})
end