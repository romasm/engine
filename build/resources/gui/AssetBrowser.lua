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
        color_press = 'act_05',
        color_nonactive = 'bg_01',
    },

    text = { font = "", },

    icon = {
        rect = { l = 0, t = 0, w = 86, h = 66 },
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
    dbclick_activation = true,

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

GuiStyles.tool_text_button = {
    styles = {
        GuiStyles.tool_button,
    },
    holded = false,
    height = 30,
    width = 30,

    icon = {
        rect = { l = 0, t = 0, w = 30, h = 30 },
    },
}

function Gui.AssetBrowserWindow()
return GuiWindow({
    styles = {
        GuiStyles.props_window,
        GuiStyles.window_colors,
    },

    cleintarea_padding = { t = 38, },
    
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
        str = "Materials library"
    },
    
    events = {
        [GUI_EVENTS.WIN_SCROLL_WHEEL] = function(self, ev) 
            self.entity:SetHierarchyFocusOnMe(false)
            return false
        end,
    },
    
    GuiButton({
        styles = {
            GuiStyles.tool_text_button,
        },

        align = GUI_ALIGN.RIGHT,
        right = 4,
        top = 29,
        icon = {material = GuiMaterials.delete_icon},
        alt = "Delete selected material",
        id = 'delete_btn',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                AssetBrowser:DeleteSelected()
                return true 
            end,
        },
    }),
    
    GuiButton({
        styles = {
            GuiStyles.tool_text_button,
        },

        align = GUI_ALIGN.RIGHT,
        right = 38,
        top = 29,
        icon = {material = GuiMaterials.copy_mat_icon},
        alt = "Copy selected material",
        id = 'copy_btn',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                AssetBrowser:CopySelected()
                return true 
            end,
        },
    }),
    
    GuiButton({
        styles = {
            GuiStyles.tool_text_button,
        },

        align = GUI_ALIGN.RIGHT,
        right = 72,
        top = 29,
        icon = {material = GuiMaterials.new_mat_icon},
        alt = "Create new material",

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                AssetBrowser:CreateNew()
                return true 
            end,
        },
    }),

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
            GuiStyles.text_textfield,
        },
        top = 35,
        left = 5,
        width = 150,
        height = 20,
        border = {
            width = 0
        },

        events = {
            [GUI_EVENTS.TF_EDITING]  = function(self, ev)
                AssetBrowser:Find(self:GetText()) 
                return true
                end,
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev)
                if self:GetText():len() == 0 then 
                    self.entity:GetParent():GetChildById('find_sign').enable = true
                end
                return true
                end,
            [GUI_EVENTS.TF_ACTIVATE]  = function(self, ev)
                self.entity:GetParent():GetChildById('find_sign').enable = false
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
        
        id = 'find_sign',
        str = "Type to find",
        static = true,
        color = 'text_02',

        enable = true,

        top = 36,
        left = 8,
    }),

    GuiClientarea({
                
        GuiBody({
            width = 100,
            width_percent = true,
        }),
    }),
})
end

function Gui.AssetBrowserMaterial(filename, matname, topOffset, leftOffset, num)
local btn = GuiButton({
    styles = { GuiStyles.material_button, },
    icon = {material = {
        shader = "../resources/shaders/gui/rect_color_icon_alpha",
        textures = {filename..".tga"}
    }},
    alt = matname,
    top = topOffset,
    left = leftOffset,
    id = tostring(num),

    events = {
        [GUI_EVENTS.TF_ACTIVATION_WAIT] = function(self, ev)
            if not self.state_press then
                self:SetPressed(true)
                AssetBrowser:SetSelected(self)
                self.entity:SetHierarchyFocusOnMe(false)
            end
            return true 
        end,
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