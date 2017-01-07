
GuiStyles.entity_button = {
    styles = {
        GuiStyles.solid_button,
    },

    holded = true,

    height = GUI_SCENELIST_SIZE.Y,
    align = GUI_ALIGN.BOTH,
    
    background = {
        color = 'bg_01',
        color_hover = 'act_01',
        color_press = 'act_05',
        color_nonactive = 'bg_01',
    },

    text = { font = "", },

    icon = {
        rect = { l = 24, t = 0, w = 20, h = 20 },
    },
}

GuiStyles.entityname_textfield = {
    styles = { 
        GuiStyles.common_textfield, 
    },
    
    collide_through = true,
    ignore_events = true,

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
        color_nonactive = 'text_01',
        offset = { x = 3, y = 0 },
        center = { x = false, y = true },
        length = 128,
        font = "../resources/fonts/opensans_normal_16px",    
    },

    data = { d_type = GUI_TEXTFIELD.TEXT, },
}

function Gui.SceneBrowserWindow()
return GuiWindow({
    styles = {
        GuiStyles.props_window,
        GuiStyles.window_colors,
    },

    cleintarea_padding = { t = 38, b = 20 },
    
    align = GUI_ALIGN.BOTH,
    valign = GUI_VALIGN.BOTH,

    left = 4,
    bottom = 4,
    right = 4,
    top = 4,

    id = "scene_browser_window",

    header = {
        styles = {
            GuiStyles.window_header,
        },
        str = "Scene Browser"
    },
    
    events = {
        [GUI_EVENTS.WIN_SCROLL_WHEEL] = function(self, ev) 
            self.entity:SetHierarchyFocusOnMe(false)
            return false
        end,
    },

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
                --SceneBrowser:Find(self:GetText()) 
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
                --SceneBrowser:Find("")
                return true 
            end,
        },
    }),

    GuiClientarea({
        
        GuiString({
            styles = {
                GuiStyles.ghost,
                GuiStyles.string_autosize,
                GuiStyles.string_25,
            },

            id = 'none_msg',

            str = "No scene loaded",

            static = true,

            align = GUI_ALIGN.CENTER,
            valign = GUI_VALIGN.MIDDLE,

            color = 'text_02',
            color_nonactive = 'text_02',
        }),

        GuiBody({
            width = 100,
            width_percent = true,
            groupstack = true,
        }),
    }),

    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_18,
        },

        id = 'selected_count',

        str = "0 selected",

        static = false,
        length = 20,

        valign = GUI_VALIGN.BOTTOM,
        bottom = 1,
        left = 10,

        color = 'text_01',
        color_nonactive = 'text_02',
    }),
})
end

function Gui.SceneBrowserEntity(name, ent_type, ent, topOffset, num)
local label = name:len() > 0 and name or ent_type

local btn = GuiButton({
    styles = { GuiStyles.entity_button, },
    icon = {material = {
        shader = "../resources/shaders/gui/rect_color_icon_alpha",
        textures = {"../resources/textures/editor_hud/" .. ent_type .. ".dds"}
    }},
    top = topOffset,
    id = tostring(num),

    events = {
        [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
            if not CoreGui.Keys.Shift() then
                if not CoreGui.Keys.Ctrl() then
                    SceneBrowser:SetSelected(self)
                else
                    SceneBrowser:AddSelected(self)
                end
            else
                SceneBrowser:GroupSelected(self)
            end

            self.entity:SetFocus(HEntity())
            return true 
        end,
        [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
            if not CoreGui.Keys.Shift() then
                if not CoreGui.Keys.Ctrl() and SceneBrowser:GetSelectedCount() > 1 then
                    SceneBrowser:SetSelected(self)
                    self:SetPressed(true)
                else
                    SceneBrowser:DeleteSelected(self)
                end
            else
                self:SetPressed(true)
                SceneBrowser:GroupSelected(self)
            end
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
        --[[
        [GUI_EVENTS.MOUSE_DOWN] = function(self, ev)
            if ev.entity:is_eq(self.entity) or ev.entity:GetID() == 'ent_name_tf' then
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
            if SceneBrowser.selectedMatBtn ~= self then return false end

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
                SceneBrowser:CreateNew()
            elseif ev.entity:GetID() == "asset_rename" then
                local name_tf = self.entity:GetChildById('mat_name_tf')
                local fake_event = HEvent()
                fake_event.entity = name_tf
                fake_event.event = GUI_EVENTS.MOUSE_DBLCLICK
                name_tf:SendEvent( fake_event )
                self.entity:SetFocus( name_tf )
            elseif ev.entity:GetID() == "asset_copy" then
                SceneBrowser:Copy(self.assetID)
            elseif ev.entity:GetID() == "asset_delete" then
                SceneBrowser:Delete(self.assetID)
            end
            return true
        end,--]]
    },

    GuiTextfield({
        styles = { GuiStyles.entityname_textfield, },
        
        text = {str = label},

        height = 18,
        top = 1,
        left = 48,
        align = GUI_ALIGN.BOTH,

        id = 'ent_name_tf',

        events = {
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev) 
                --SceneBrowser:Rename(self)
                self.entity:Deactivate()
                return true
            end,
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.tool_button,
        },
        holded = true,
        height = 20,
        width = 20,
        icon = {
            color = 'text_04_a5',
            color_hover = 'act_02',
            color_press = 'act_04',
            color_nonactive = 'text_02',
            rect = { l = 0, t = 0, w = 20, h = 20 },
            material = GuiMaterials.visibility_icon,
        },

        background = {
            color = 'null',
            color_hover = 'null',
            color_press = 'null',
            color_nonactive = 'null',
        },
        align = GUI_ALIGN.LEFT,
        alt = "Hide / Show",
        id = 'vis_btn',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                SceneBrowser:HideEnt(self)
                return true 
            end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev)
                SceneBrowser:ShowEnt(self)
                return true 
            end,
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.alt_button,
        },
        height = 20,
        align = GUI_ALIGN.BOTH,
        left = 24,
        alt = ent_type,
    }),
})

btn.linked_ent = ent
btn.vis_btn = btn.entity:GetChildById('vis_btn'):GetInherited()
return btn
end