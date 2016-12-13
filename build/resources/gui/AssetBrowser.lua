GuiStyles.material_button = {
    styles = {
        GuiStyles.solid_button,
    },

    holded = true,

    height = 87,
    width = 87,
    
    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'act_00',
        color_nonactive = 'bg_01',
    },

    text = { font = "", },

    icon = {
        rect = { l = 0, t = 0, w = 87, h = 67 },
    },
}

GuiStyles.assetname_textfield = {
    styles = { GuiStyles.common_textfield, },

    color_selection = 'act_00',
    color_cursor = 'bg_05',

    background = {
        color = 'null',
        color_live = 'act_02',
        color_nonactive = 'null'
    },

    border = { width = 0 },

    show_tail = false,
    allow_none = false,

    text = {
        color = 'text_01',
        color_live = 'bg_06',
        color_nonactive = 'text_02',
        offset = { x = 3, y = 0 },
        center = { x = false, y = true },
        length = 128,
        font = "../resources/fonts/opensans_normal_16px",    
    },

    data = { d_type = GUI_TEXTFIELD.TEXT, },
}

function Gui.AssetBrowserWindow()
return GuiWindow({
    styles = {
        GuiStyles.props_window,
        GuiStyles.window_colors,
    },

    cleintarea_padding = { t = 50, },
    
    align = GUI_ALIGN.BOTH,
    valign = GUI_VALIGN.BOTH,

    left = 4,
    bottom = 4,
    right = 4,
    top = 4,

    id = "asset_browser_window",
    
    header = {
        styles = {
            GuiStyles.window_header,
        },
        str = "Asset Browser"
    },
    
    events = {
        [GUI_EVENTS.WIN_SCROLL_WHEEL] = function(self, ev) 
            self.entity:SetHierarchyFocusOnMe(false)
            return false
        end,
    },
    
    GuiClientarea({
                
        GuiBody({
            width = 100,
            width_percent = true,
        }),
    }),
})
end

function Gui.AssetBrowserMaterial(filename, matname, topOffset, leftOffset)
local btn = GuiButton({
    styles = { GuiStyles.material_button, },
    icon = {material = {
        shader = "../resources/shaders/gui/rect_color_icon_alpha",
        textures = {filename..".dds"}
    }},
    alt = matname,
    top = topOffset,
    left = leftOffset,
    
    events = {
        [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
            AssetBrowser:SetSelected(self)
            self.entity:SetFocus(HEntity())
            return true 
        end,
        [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
            AssetBrowser:UnSelected()
            self.entity:SetFocus(HEntity())
            return true 
        end,
        [GUI_EVENTS.UNFOCUS] = function(self, ev)
            if ev.entity:is_eq(self.entity) then
                self.entity:SetFocus(HEntity())
                return true
            end 
            return false
        end,
        [GUI_EVENTS.MOUSE_DOWN] = function(self, ev)
            if ev.entity:is_eq(self.entity) then
                self.entity:SetHierarchyFocusOnMe(false)
                self.entity:SetFocus(HEntity())
                return true
            end
            return false
        end,
    },

    GuiTextfield({
        styles = { GuiStyles.assetname_textfield, },
        
        text = {str = matname},
        alt = matname,

        top = 67,
        left = 2,
        width = 83,
        height = 18,

        events = {
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev) 
                AssetBrowser:Rename(self)
                return true
            end,
        },
    }),
})
btn.assetID = filename
return btn
end