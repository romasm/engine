loader.require("Menus.Asset")

GuiStyles.material_button = {
    styles = {
        GuiStyles.solid_button,
    },

    holded = true,

    height = GUI_PREVIEW_SIZE.Y,
    width = GUI_PREVIEW_SIZE.X,
    
    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'act_05',
        color_nonactive = 'bg_01',
    },

    text = { font = "", },

    icon = {
        rect = { l = 0, t = 0, w = GUI_PREVIEW_SIZE.X, h = GUI_PREVIEW_SIZE.Y },
    },
}

GuiStyles.assetname_textfield = {
    styles = { GuiStyles.common_textfield, },

    color_selection = 'act_00',
    color_cursor = 'bg_05',

    background = {
        color = 'bg_05_a4',
        color_live = 'act_02',
        color_nonactive = 'bg_05_a4'
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

GuiStyles.tool_assets_button = {
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

    left = 0,
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
        [GUI_EVENTS.HK_COPY] = function(self, ev) 
            AssetBrowser:CopySelected()
            return true
        end,
        [GUI_EVENTS.HK_NEW] = function(self, ev) 
            AssetBrowser:CreateNew()
            return true
        end,
    },
    
    GuiButton({
        styles = {
            GuiStyles.tool_assets_button,
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
            GuiStyles.tool_assets_button,
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
            GuiStyles.tool_assets_button,
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
        top = 33,
        left = 5,
        width = 170,
        height = 24,
        border = {
            width = 0
        },
        id = 'find_field',

        events = {
            [GUI_EVENTS.TF_EDITING]  = function(self, ev)
                AssetBrowser:Find(self:GetText()) 
                return true
                end,
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev)
                if self:GetText():len() == 0 then 
                    self.entity:GetChildById('find_sign').enable = true
                    self.entity:GetParent():GetChildById('clear_btn').enable = false
                else
                    self.entity:GetParent():GetChildById('clear_btn').enable = true
                end
                return true
                end,
            [GUI_EVENTS.TF_ACTIVATE]  = function(self, ev)
                self.entity:GetChildById('find_sign').enable = false
                self.entity:GetParent():GetChildById('clear_btn').enable = false
                return true
                end,
        },

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
            color_nonactive = 'text_02',

            enable = true,

            top = 3,
            left = 3,
        }),        
    }),

    GuiButton({
        styles = {
            GuiStyles.tool_button,
        },
        holded = false,
        height = 24,
        width = 24,
        align = GUI_ALIGN.LEFT,
        left = 151,
        top = 33,
        icon = {
            material = GuiMaterials.clear_str_icon,
            rect = { l = 0, t = 0, w = 24, h = 24 },
        },
        background = {color = 'null',},
        alt = "Clear",
                
        enable = false,
        id = 'clear_btn',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                local tf = self.entity:GetParent():GetChildById('find_field')
                tf:GetInherited():SetText()
                tf:GetChildById('find_sign').enable = true
                self:SetPressed(false)
                self.entity.enable = false
                AssetBrowser:Find("")
                return true 
            end,
        },
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

if not FileIO.IsExist(filename..".dds") then 
    AssetBrowser:GeneratePreview(filename..".mtb")
end

local btn = GuiButton({
    styles = { GuiStyles.material_button, },
    icon = {material = {
        shader = "../resources/shaders/gui/rect_color_icon_alpha",
        textures = {colorTex = filename..".dds"}
    }},
    --alt = matname,
    top = topOffset,
    left = leftOffset,
    id = tostring(num),

    events = {
        [GUI_EVENTS.TF_ACTIVATION_WAIT] = function(self, ev)
            if not self.state_press then
                self:SetPressed(true)
                AssetBrowser:SetSelected(self, true, false)
                self.entity:SetHierarchyFocusOnMe(false)
            end
            return true 
        end,
        [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
            AssetBrowser:SetSelected(self, true, false)
            self.entity:SetFocus(HEntity())
            return true 
        end,
        [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
            AssetBrowser:SetSelected(nil, false, false)
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
            if ev.entity:is_eq(self.entity) or ev.entity:GetID() == 'mat_name_tf' then
                self.entity:SetHierarchyFocusOnMe(false)
                self.entity:SetFocus(HEntity())

                if ev.key == KEYBOARD_CODES.KEY_RBUTTON then
                    local menu = Gui.AssetMenu()
                    self:AttachOverlay(menu)
                    menu:Open(ev.coords.x, ev.coords.y)
                end
                return true
            end
            return false
        end,
        [GUI_EVENTS.HK_RENAME] = function(self, ev)
            if AssetBrowser.selectedMatBtn ~= self then return false end

            local name_tf = self.entity:GetChildById('mat_name_tf')
            local fake_event = HEvent()
            fake_event.entity = name_tf
            fake_event.event = GUI_EVENTS.MOUSE_DBLCLICK
            name_tf:SendEvent( fake_event )
            self.entity:SetFocus( name_tf )
            return true
        end,
        [GUI_EVENTS.MENU_CLICK] = function(self, ev)
            if ev.entity:GetID() == "asset_new" then
                AssetBrowser:CreateNew()
            elseif ev.entity:GetID() == "asset_rename" then
                local name_tf = self.entity:GetChildById('mat_name_tf')
                local fake_event = HEvent()
                fake_event.entity = name_tf
                fake_event.event = GUI_EVENTS.MOUSE_DBLCLICK
                name_tf:SendEvent( fake_event )
                self.entity:SetFocus( name_tf )
            elseif ev.entity:GetID() == "asset_copy" then
                AssetBrowser:Copy(self.assetID)
            elseif ev.entity:GetID() == "asset_delete" then
                AssetBrowser:Delete(self.assetID)
            end
            return true
        end,
    },

    GuiTextfield({
        styles = { GuiStyles.assetname_textfield, },
        
        text = {str = matname},
        alt = matname,

        height = 18,
        top = GUI_PREVIEW_SIZE.Y - 2 - 18,
        left = 2,
        width = GUI_PREVIEW_SIZE.X - 4,

        id = 'mat_name_tf',

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