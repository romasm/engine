GuiStyles.vp_header_colors = {
    background = {
        color = 'bg_05_a4',
        color_hover = 'bg_05_a7',
        color_press = 'bg_05_a6',
    },

    text = {
        color = 'act_02',
        color_hover = 'act_03',
        color_press = 'act_02',
        font = "../resources/fonts/opensans_normal_18px",
    },
}

GuiStyles.vp_perf = {
    styles = {
        GuiStyles.ghost,
        GuiStyles.string_simple,
        GuiStyles.string_18,
    },

    static = false,
    length = 16,

    padding = { x = 0, },

    height_percent = true,
    height = 100,

    align = GUI_ALIGN.RIGHT,

    color = 'act_02',
}

function Gui.ViewportWindow()
return GuiEntity({
    styles = {
        GuiStyles.live,
    },
    align = GUI_ALIGN.BOTH,
    valign = GUI_VALIGN.BOTH,

    top = 4,
    bottom = 5,

    id = "viewport_window",
    
    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_70_l,
        },

        str = "No scene loaded",

        static = true,

        align = GUI_ALIGN.CENTER,
        valign = GUI_VALIGN.MIDDLE,

        color = 'text_02',
    }),

    GuiRect({
        styles = {
            GuiStyles.live,
        },
        visible = false,
        material = GuiMaterials.viewport,

        id = "viewport_rect",

        width = 100,
        height = 100,

        events = {
            [GUI_EVENTS.SYS_RESIZE] = function(self, ev) return Viewport:onResize() end,
            
            [GUI_EVENTS.MOUSE_DOWN] = function(self, ev) return Viewport:onMouseDown(ev) end,
            [GUI_EVENTS.MOUSE_UP] = function(self, ev) return Viewport:onMouseUp(ev) end,
            [GUI_EVENTS.KEY_DOWN] = function(self, ev) return Viewport:onKeyDown(ev) end,
            [GUI_EVENTS.KEY_UP] = function(self, ev) return Viewport:onKeyUp(ev) end,
            [GUI_EVENTS.MOUSE_MOVE] = function(self, ev) return Viewport:onMouseMove(ev) end,
            [GUI_EVENTS.MOUSE_HOVER] = function(self, ev) return Viewport:onMouseMove(ev) end,
            [GUI_EVENTS.MOUSE_WHEEL] = function(self, ev) return Viewport:onMouseWheel(ev) end,
            [GUI_EVENTS.ITEMS_DROPED] = function(self, ev) return Viewport:onItemDroped(ev) end,

            [GUI_EVENTS.MENU_CLOSE] = function(self, ev) Viewport:MenuClose(self) end,
            [GUI_EVENTS.MENU_CLICK] = function(self, ev) Viewport:MenuClick(self, ev) end, 
        },
    }),

    GuiDumb({
        styles = {
            GuiStyles.live,
        },
        top = 4,
        height = 25,
        
        align = GUI_ALIGN.BOTH,
        left = 4,
        right = 4,

        enable = false,
        
        id = "vp_overlay_gui",

        GuiCombo({
            styles = {
                GuiStyles.vp_header_colors,
                GuiStyles.common_button,
                GuiStyles.no_border,
            },

            id = "vp_rendermode",
        
            text = {
                offset = { x = 15, y = 0 },
                center = { x = false, y = true },
                length = 32,
            },
        
            list_h_offset = 4,

            str_height = 25,
            allow_none = false,

            width = 140,
            height = 100,
            height_percent = true,

            align = GUI_ALIGN.RIGHT,

            list = {
                "Lit",
                "Albedo",
                "Specular",
                "Roughness",
                "Normal",
                "Emissive",
                "Subsurface color",
                "Thickness",
                "Ambient occlusion",
                "Depth",
                "SSR",
                "Voxels Color",
                "Voxels Emissive",
                "Voxels Intensity",
                "Voxels Normal",
                "Voxels Opacity 0",
                "Voxels Opacity 1",
                "Voxels Opacity 2",
                "Voxels Opacity 3",
                "Voxels Opacity 4",
                "Voxels Opacity 5",
                "Voxels Emittance 0",
                "Voxels Emittance 1",
                "Voxels Emittance 2",
                "Voxels Emittance 3",
                "Voxels Emittance 4",
                "Voxels Emittance 5",
                "Diffuse",
                "Indirect Diffuse",
                "Specular",
                "Indirect Specular",
            },
            alt = "Frame components",

            events = {
                [GUI_EVENTS.COMBO_SELECT] = function(self, ev) return Viewport:SetMode(self, ev) end,
            },
        }),

        GuiRect({
            styles = {
                GuiStyles.dead,
                GuiStyles.vp_header_colors,
            },

            id = "vp_perf_rect",

            width = 135,

            height = 100,
            height_percent = true,

            GuiString({
                styles = {
                    GuiStyles.vp_perf,
                },
                id = 'vp_fps',
                str = "0.0 fps",
                right = 72,
            }),

            GuiString({
                styles = {
                    GuiStyles.vp_perf,
                },
                id = 'vp_ms',
                str = "0.0 ms",
                right = 7,
            }),
        }),
    }),
})
end