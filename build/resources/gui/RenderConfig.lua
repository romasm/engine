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
    height = 546,
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
                local checkID = self:GetCheck()
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.bufferViewMode = checkID - 1
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

    -- VOXELS
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

    GuiCheckGroup({
        styles = {
            GuiStyles.live,
        },
        top = 320,
        left = 20,
        width = 190,
        height = 161,
        id = 'voxels_cbg',

        events = {
            [GUI_EVENTS.CBGROUP_CHECK] = function(self, ev) 
                local checkID = self:GetCheck()
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.voxelViewMode = checkID - 1
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetCheck( renderConfig.voxelViewMode + 1 )
                return true
            end,
        },
          
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 0,
            text = { str = "None" },
            alt = "Do not draw voxels",
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 28,
            text = { str = "Color" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 51,
            text = { str = "Emissive" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 74,
            text = { str = "Intensity" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 97,
            text = { str = "Normal" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 120,
            text = { str = "Opacity" },
        }),
        GuiCheck({
            styles = {GuiStyles.rendercfg_check,},
            top = 143,
            text = { str = "Emittance" },
        }),
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
        top = 493,
        left = 20,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.common_dataslider,
        },
        left = 20,
        width = 170,
        height = 20,
        top = 516,
        data = {
            min = 0,
            max = 11,
            decimal = 0,
            step = 1
        },
        alt = "Number of first voxel cascade",

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev)
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.voxelCascade = self:GetValue()  
                return true
            end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev)
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.voxelCascade = self:GetValue()  
                return true
            end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) 
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetValue(renderConfig.voxelCascade)
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
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.analyticLightDiffuse = 0
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
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.analyticLightSpecular = 0
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
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.ambientLightDiffuse = 0
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
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                renderConfig.ambientLightSpecular = 0
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                local renderConfig = Viewport.lua_world.scenepl:GetConfig()
                self:SetCheck( renderConfig.ambientLightSpecular > 0 )
                return true
            end,
        },
    }),

    GuiCheck({
        styles = {GuiStyles.rendercfg_check,},
        top = 157,
        left = 220,
        text = { str = "Draw collision hulls" },
        alt = "Visualisation of collision convex hulls",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) 
                Viewport:SetPhysicsDraw(true)
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                Viewport:SetPhysicsDraw(false)
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                self:SetCheck( Viewport.collisionDraw )
                return true
            end,
        },
    }),

    GuiCheck({
        styles = {GuiStyles.rendercfg_check,},
        top = 180,
        left = 220,
        text = { str = "Draw scene graph nodes" },
        alt = "Visualisation of scene graph hierarchy",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) 
                Viewport:SetSceneGraphDraw(true)
                return true
            end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) 
                Viewport:SetSceneGraphDraw(false)
                return true
            end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                self:SetCheck( Viewport.sceneGraphDraw )
                return true
            end,
        },
    }),
})
end