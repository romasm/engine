loader.require("ComponentsGui.StaticMeshCallback")

GuiStyles.material_list_button = {
    styles = {
        GuiStyles.common_button,
    },

    collide_through = true,
    height = 25,
    width = 25,

    background = {
        color = 'act_00_a0',
        color_hover = 'act_00',
        color_press = 'act_00',
        color_nonactive = 'bg_01',
    },

    text = {
        font = '',
    },
    border = {
        width = 0,
    },

    icon = {
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02',
        rect = { l = 0, t = 0, w = 25, h = 25 },
    },
}

function Gui.StaticMeshComp()
return GuiGroup({
    styles = {
        GuiStyles.common_group,
    },
    
    id = "static_mesh",
    
    header = {
        styles = {
            GuiStyles.group_button,
        },
        align = GUI_ALIGN.BOTH,
        top = 0,
        height = 23,

        text = {
            str = "Model",
            offset = { x = 22, y = 1 },
            center = { x = false, y = false },
        },
    },

    width = 100,
    width_percent = true,
    
    height = 105,

    -- mesh
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Filepath",
        left = 10,
        top = 35,
    }), 

    GuiFilefield({
        styles = {GuiStyles.common_filefield,},
        align = GUI_ALIGN.BOTH,
        left = 70,
        right = 10,
        top = 33,
        browse_header = "Choose model file",
        filetypes = {
            {"Supported", "*.3ds;*.dae;*.dxf;*.fbx;*.obj;*.ply;*.mnogo_chego"},
        },
        allow_none = false,

        events = {
            [GUI_EVENTS.FF_SET] = function(self, ev) return StaticMeshCallback.SetMesh(self, ev) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return StaticMeshCallback.UpdMesh(self, ev) end,
        }
    }),
    
    -- shadows
    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 71,
        width = 120,
        height = 18,
        text = { str = "Cast shadows" },
        alt = "Cast shadows from model",

        events = {
            [GUI_EVENTS.CB_CHECKED] = function(self, ev) return StaticMeshCallback.SetShadows(self, ev, true) end,
            [GUI_EVENTS.CB_UNCHECKED] = function(self, ev) return StaticMeshCallback.SetShadows(self, ev, false) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return StaticMeshCallback.UpdShadows(self, ev) end,
        }
    }),
    
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Materials",
        left = 115,
        top = 113,
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
})
end

function Gui.MaterialSlot(i_mat, top_offset)
return GuiDumb({
    styles = {GuiStyles.live,},
    width = 100,
    width_percent = true,
    height = 60,
    top = top_offset,
    id = 'mat_'..i_mat,

    GuiButton({
        styles = {GuiStyles.solid_button,},
        text = {font = "",},
        cursor = SYSTEM_CURSORS.NONE,
        holded = true,
        width = 100,
        width_percent = true,
        height = 100,
        height_percent = true,
        id = 'mat_btn',

        background = {
            color = 'bg_01',
            color_hover = 'act_01',
            color_press = 'act_05',
            color_nonactive = 'bg_01',
        },

        icon = {
            rect = { l = -35, t = 0, w = GUI_PREVIEW_SIZE.X, h = GUI_PREVIEW_SIZE.Y },
            material = {
                shader = "../resources/shaders/gui/rect_color_icon_alpha",
                textures = {""}
            },
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return StaticMeshCallback.SelectMat(self) end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) 
                AssetBrowser:SetSelectedByName(nil, false) 
                return true
            end,
            material_id = i_mat,
        }
    }),

    GuiTextfield({
        styles = {GuiStyles.props_textfield,},
        collide_through = true,
        align = GUI_ALIGN.BOTH,
        left = 70,
        right = 10,
        top = 5,
        height = 20,
        allow_none = false,
        show_tail = true,
        text = {
            length = 128,
        },
        data = { d_type = GUI_TEXTFIELD.TEXT, },

        id = 'slot_mat',

        events = {
            [GUI_EVENTS.TF_DEACTIVATE] = function(self, ev) return StaticMeshCallback.SetMaterial(self, ev, self.events.material_id) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return StaticMeshCallback.UpdMaterial(self, ev, self.events.material_id) end,
            material_id = i_mat,
        }
    }),

    GuiButton({
        styles = {
            GuiStyles.material_list_button,
        },
        collide_through = true,
        holded = false,
        height = 25,
        width = 25,
        left = 70,
        top = 30,
        alt = "Assign selected material",
        icon = {
            material = GuiMaterials.assign_asset_icon,
            rect = { l = 0, t = 0, w = 25, h = 25 },
        },
        id = 'assign_btn',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                StaticMeshCallback.AssignSelectedMaterial(self, ev, self.events.material_id)
                return true 
            end,
            material_id = i_mat,
        },
    }),

    GuiButton({
        styles = {
            GuiStyles.material_list_button,
        },
        collide_through = true,
        holded = false,
        height = 25,
        width = 25,
        left = 99,
        top = 30,
        alt = "Clear material",
        icon = {
            material = GuiMaterials.clear_asset_icon,
            rect = { l = 0, t = 0, w = 25, h = 25 },
        },
        id = 'clear_btn',

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev)
                StaticMeshCallback.ClearMaterial(self, ev, self.events.material_id)
                return true 
            end,
            material_id = i_mat,
        },
    }),
})
end