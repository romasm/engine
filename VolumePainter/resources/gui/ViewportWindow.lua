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
    bottom = 4,

    id = "viewport_window",
    
    events = {            
        [GUI_EVENTS.MOUSE_DOWN] = function(self, ev) return Viewport:onMouseDown(ev) end,
        [GUI_EVENTS.MOUSE_UP] = function(self, ev) return Viewport:onMouseUp(ev) end,
        [GUI_EVENTS.KEY_DOWN] = function(self, ev) return Viewport:onKeyDown(ev) end,
        [GUI_EVENTS.KEY_UP] = function(self, ev) return Viewport:onKeyUp(ev) end,
        [GUI_EVENTS.MOUSE_MOVE] = function(self, ev) return Viewport:onMouseMove(ev) end,
        [GUI_EVENTS.MOUSE_HOVER] = function(self, ev) return Viewport:onMouseMove(ev) end,
        [GUI_EVENTS.MOUSE_WHEEL] = function(self, ev) return Viewport:onMouseWheel(ev) end,

        [GUI_EVENTS.ITEMS_DROPED] = function(self, ev) return Viewport:onItemDroped(ev) end,
        [GUI_EVENTS.ITEMS_DRAG_ENTER] = function(self, ev) return Viewport:onItemStartDrag(ev) end,
        [GUI_EVENTS.ITEMS_DRAG_LEAVE] = function(self, ev) return Viewport:onItemStopDrag(ev) end,
        [GUI_EVENTS.ITEMS_DRAG_MOVE] = function(self, ev) return Viewport:onItemDrag(ev) end,

        [GUI_EVENTS.MENU_CLOSE] = function(self, ev) Viewport:MenuClose(self) end,
        [GUI_EVENTS.MENU_CLICK] = function(self, ev) Viewport:MenuClick(self, ev) end, 
    },

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
        },
    }),

    GuiDumb({
        styles = {
            GuiStyles.live_ghost,
        },

        top = 4,
        height = 25,
        
        align = GUI_ALIGN.BOTH,
        left = 4,
        right = 4,

        enable = false,
        
        id = "vp_overlay_gui",

        GuiButton({
            styles = {
                GuiStyles.solid_button,
                GuiStyles.vp_header_colors,
            },

            id = "vp_rendercfg",
            holded = true,
        
            text = {
                str = "Debug",
            },
            alt = "Rendering configuration",
            
            width = 100,
            height = 100,
            height_percent = true,

            align = GUI_ALIGN.RIGHT,

            events = {
                [GUI_EVENTS.MOUSE_UP] = function(self, ev) 
                    if Viewport.renderConfig ~= nil then return true
                    else return false end 
                end,
                [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return Viewport:OpenRenderConfig(self) end,
                [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) return Viewport:CloseRenderConfig() end,
            },
        }),

        GuiRect({
            styles = {
                GuiStyles.live,
                GuiStyles.vp_header_colors,
            },

            id = "vp_perf_rect",

            width = 135,

            height = 100,
            height_percent = true,

            events = {
                [GUI_EVENTS.MOUSE_DOWN] = function(self, ev)
                    return true
                end,
            },

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