loader.require("ComponentsGui.MaterialPropsCallback")

GuiStyles.mat_dataslider = {
    styles = {
        GuiStyles.common_dataslider,
    },

    left = 120,
    width = 155,
    height = 20,
}

function Gui.MaterialProps()
return {GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    
    id = "general",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "General",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 70,

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Shader",
        left = 10,
        top = 35,
    }),
    
    GuiFilefield({
        styles = {GuiStyles.common_filefield,},
        left = 80,
        top = 33,
        browse_header = "Choose shader file",
        filetypes = {
            {"Shader", "*.hlsl"},
        },
        allow_none = false,

        events = {
            [GUI_EVENTS.FF_SET] = function(self, ev) return MaterialPropsCallback.SetShader(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) 
                self:SetPath("../content/shaders/objects/opaque_main.hlsl")
                self.entity:Deactivate()
                local group = self.entity:GetParent()
                group:GetInherited():SetState(false)
                --group:GetInherited():UpdateH()
                return true
            end,
        }
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
}),

-- ALBEDO
GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "albedo",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "Albedo",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 187,
    
    Gui.Texture({
        width = 265,
        top = 35,
        left = 10,
        id = 'albedo_texture',
        allow_autoreload = true,
        str = "Albedo",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetAlbedoTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetAlbedoTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdAlbedoTex(self, ev) end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Albedo color",
        left = 10,
        top = 152,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 150,
        width = 155,
        alt = "Pick albedo color (multiplier)",

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialPropsCallback.StartColorPicking(self, 0, "Albedo") end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialPropsCallback.ColorPicking(self, 0) end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialPropsCallback.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdColor(self, 0) end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
}),

-- NORMAL
GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "normal",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "Geometry",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 213,
    
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Normal map",
        left = 107,
        top = 30,
    }),

    Gui.Texture({
        width = 265,
        top = 55,
        left = 10,
        id = 'normal_map',
        allow_autoreload = true,
        str = "Normal map",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetNormalTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetNormalTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdNormalTex(self, ev) end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Normals space",
        left = 10,
        top = 177,
        id = 'normal_space_str',
    }),

    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = false,
        left = 120,
        top = 175,
        width = 155,
        height = 21,
        list = {
            "Tangent space",
            "Object space",
        },
        alt = "Normal map space",
        id = 'normal_space',

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev) return MaterialPropsCallback.SetNormalSpace(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdNormalSpace(self, ev) end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
}),

-- ROUGHNESS
GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "roughness",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "Microfacets",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 277,

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Parametrization",
        left = 10,
        top = 37,
    }),

    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = false,
        left = 120,
        top = 35,
        width = 155,
        height = 21,
        list = {
            "Roughness",
            "Glossiness",
        },
        alt = "Roughness or glossiness (inverted roughness) parametrization",

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev) return MaterialPropsCallback.SetRoughnessType(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdRoughnessType(self, ev) end,
        },
    }),

    Gui.Texture({
        width = 265,
        top = 65,
        left = 10,
        id = 'roughness_texture',
        allow_autoreload = true,
        str = "Microfacets",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetRoughnessTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetRoughnessTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdRoughnessTex(self, ev) end,
        }
    }),

    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 180,
        width = 180,
        height = 18,
        text = { str = "Two channels, anisotropic" },
        alt = "Separate for U and V",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return MaterialPropsCallback.SetAnisoRG(self, ev, true) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return MaterialPropsCallback.SetAnisoRG(self, ev, false) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdAnisoRG(self, ev) end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Microfacets U",
        static = false,
        length = 16,
        left = 10,
        top = 212,
        id = 'roughness_u_str',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 210,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Roughness / Glossiness for U",
        id = 'roughness_u',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialPropsCallback.StartRoughU(self) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialPropsCallback.DragRoughU(self) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialPropsCallback.EndRoughU(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdRoughU(self) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Microfacets V",
        static = false,
        length = 16,
        left = 10,
        top = 242,
        id = 'roughness_v_str',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 240,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Roughness / Glossiness for V",
        id = 'roughness_v',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialPropsCallback.StartValue(self, 4, "Microfacets V") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialPropsCallback.DragValue(self, 4) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialPropsCallback.EndValue(self, 4) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdValue(self, 4) end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
}),

-- SPECULAR
GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "reflectivity",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "Reflectivity",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 222,

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Parametrization",
        left = 10,
        top = 37,
    }),

    GuiCombo({
        styles = {GuiStyles.props_combo,},   
        allow_none = false,
        left = 120,
        top = 35,
        width = 155,
        height = 21,
        list = {
            "Metalness",
            "Specular (F0)",
        },
        alt = "Metalness or Specular (F0) parametrization",

        events = {
            [GUI_EVENTS.COMBO_SELECT] = function(self, ev) return MaterialPropsCallback.SetReflectivityType(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdReflectivityType(self, ev) end,
        },
    }),

    Gui.Texture({
        width = 265,
        top = 65,
        left = 10,
        allow_autoreload = true,
        str = "Reflectivity",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetReflectivityTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetReflectivityTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdReflectivityTex(self, ev) end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Metalness",
        static = false,
        length = 16,
        left = 10,
        top = 187,
        id = 'reflectivity_header',
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 185,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Metalness",
        id = 'metalness_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialPropsCallback.StartMetalness(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialPropsCallback.DragMetalness(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialPropsCallback.EndMetalness(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdMetalness(self, ev) end,
        },
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 185,
        width = 155,
        alt = "Pick specular color (multiplier)",
        id = 'specular_picker',

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialPropsCallback.StartColorPicking(self, 1, "Specular") end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialPropsCallback.ColorPicking(self, 1) end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialPropsCallback.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdColor(self, 1) end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
}),

-- AO
GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "ao",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "Ambient occlusion",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 152,
    
    Gui.Texture({
        width = 265,
        top = 35,
        left = 10,
        id = 'ao_texture',
        allow_autoreload = true,
        str = "AO",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetAOTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetAOTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdAOTex(self, ev) end,
        }
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
}),

-- EMISSIVE
GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "emissive",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "Emissive",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 222,
    
    Gui.Texture({
        width = 265,
        top = 35,
        left = 10,
        id = 'emissive_texture',
        allow_autoreload = true,
        str = "Emissive",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetEmissiveTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetEmissiveTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdEmissiveTex(self, ev) end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Intensity",
        left = 10,
        top = 157,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 155,
        data = {
            min = 0,
            max = 50,
            decimal = 2,
            overflow_max = true,
        },
        alt = "Emissive light intensity",
        id = 'emissive_intensity',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialPropsCallback.StartEmissive(self) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialPropsCallback.DragEmissive(self) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialPropsCallback.EndEmissive(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdEmissive(self) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Emissive color",
        left = 10,
        top = 187,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 185,
        width = 155,
        alt = "Pick emissive color (multiplier)",
        id = 'emissive_color',

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialPropsCallback.StartEmissivePicking(self) end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialPropsCallback.EmissivePicking(self) end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialPropsCallback.EmissivePicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdEmissiveColor(self) end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
}),

-- SUBSURFACE
GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = "subsurf",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "Subsurface",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 412,
    
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Subsurface color",
        left = 90,
        top = 35,
    }),

    Gui.Texture({
        width = 265,
        top = 65,
        left = 10,
        id = 'subsurf_texture',
        allow_autoreload = true,
        str = "Subsurface",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetSubsurfaceTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetSubsurfaceTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdSubsurfaceTex(self, ev) end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Subsurface color",
        left = 10,
        top = 187,
    }),
    
    GuiButton({
        styles = {GuiStyles.color_button,},
        left = 120,
        top = 185,
        width = 155,
        alt = "Pick subsurface color (multiplier)",

        background = {
            color_nonactive = 'bg_03',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED]  = function(self, ev) return MaterialPropsCallback.StartColorPicking(self, 3, "Subsurface") end,
            [GUI_EVENTS.COLOR_PICKING]  = function(self, ev) return MaterialPropsCallback.ColorPicking(self, 3) end,
            [GUI_EVENTS.COLOR_PICKED]  = function(self, ev) return MaterialPropsCallback.ColorPicked(self) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdColor(self, 3) end,
        },
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Thickness",
        left = 110,
        top = 225,
    }),

    Gui.Texture({
        width = 265,
        top = 255,
        left = 10,
        id = 'thickness_texture',
        allow_autoreload = true,
        str = "Thickness",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetThicknessTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetThicknessTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdThicknessTex(self, ev) end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Thickness",
        left = 10,
        top = 377,
    }),

    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 375,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Thickness value",
        id = 'thickness_slider',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialPropsCallback.StartThickness(self, ev) end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialPropsCallback.DragThickness(self, ev) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialPropsCallback.EndThickness(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdThickness(self, ev) end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
}),

-- ALPHATEST
GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    id = 'alphatest',
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        left = 0,
        right = 0,
        height = 23,

        text = {
            str = "Alphatest",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,

    height = 192,
    
    Gui.Texture({
        width = 265,
        top = 35,
        left = 10,
        id = 'alphatest_texture',
        allow_autoreload = true,
        str = "Alphatest",

        events = {
            [GUI_EVENTS.TEXTURE_SET] = function(self, ev) return MaterialPropsCallback.SetAlphatestTex(self, ev) end,
            [GUI_EVENTS.TEXTURE_DELETE] = function(self, ev) return MaterialPropsCallback.SetAlphatestTex(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdAlphatestTex(self, ev) end,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Alphatest ref",
        left = 10,
        top = 157,
        id = 'alpha_ref_str',
    }),
    
    GuiDataSlider({
        styles = {
            GuiStyles.mat_dataslider,
        },
        top = 155,
        data = {
            min = 0,
            max = 1,
            decimal = 3,
        },
        alt = "Alphatest reference (cut off value)",
        id = 'alpha_ref',

        events = {
            [GUI_EVENTS.SLIDER_START_DRAG]  = function(self, ev) return MaterialPropsCallback.StartValue(self, 6, "Alphatest") end,
            [GUI_EVENTS.SLIDER_DRAG]  = function(self, ev) return MaterialPropsCallback.DragValue(self, 6) end,
            [GUI_EVENTS.SLIDER_END_DRAG]  = function(self, ev) return MaterialPropsCallback.EndValue(self, 6) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return MaterialPropsCallback.UpdValue(self, 6) end,
        },
    }),

    GuiRect({
        styles = {
            GuiStyles.ghost,
            GuiStyles.no_border,
        },  
        valign = GUI_VALIGN.BOTTOM,
        width = 100,
        width_percent = true,
        height = 2,
        bottom = 0,
        background = {color = 'text_06'},
    }),
}),

}
end