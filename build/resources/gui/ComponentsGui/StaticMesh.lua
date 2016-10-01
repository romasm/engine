loader.require("ComponentsGui.StaticMeshCallback")

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

    -- shadows
    GuiCheck({
        styles = {GuiStyles.props_check,},
        left = 10,
        top = 35,
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
    
    -- mesh
    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Filepath",
        left = 10,
        top = 75,
    }), 

    GuiFilefield({
        styles = {GuiStyles.common_filefield,},
        left = 80,
        top = 73,
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

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "Materials list",
        left = 105,
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
    height = 40,
    top = top_offset,
    id = 'mat_'..i_mat,

    GuiButton({
        styles = {GuiStyles.common_button,},
        text = {font = "",},
        cursor = SYSTEM_CURSORS.NONE,
        holded = true,
        width = 100,
        width_percent = true,
        height = 100,
        height_percent = true,
        id = 'mat_btn',
        border = {
            color = 'bg_01',
            color_hover = 'act_01',
            color_press = 'act_00',
            width = 2,
        },
        background = {
            color = 'null',
            color_hover = 'null',
            color_press = 'null',
        },

        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) return StaticMeshCallback.SelectMat(self) end,
            [GUI_EVENTS.BUTTON_UNPRESSED] = function(self, ev) 
                MaterialProps:SetSelected(nil) 
                return true
            end,
            material_id = i_mat,
        }
    }),

    GuiString({
        styles = {GuiStyles.string_props_01,},
        str = "# " .. i_mat,
        static = false,
        length = 32,
        left = 10,
        top = 12,
    }), 

    GuiFilefield({
        styles = {GuiStyles.common_filefield,},
        collide_through = true,
        left = 50,
        width = 225,
        top = 10,
        browse_header = "Choose material file",
        filetypes = {
            {"Material", "*.mta;*.mtb;"},
        },
        allow_none = false,
        id = 'slot_mat',

        events = {
            [GUI_EVENTS.FF_SET] = function(self, ev) return StaticMeshCallback.SetMaterial(self, ev, self.events.material_id) end,
            [GUI_EVENTS.UPDATE] = function(self, ev) return StaticMeshCallback.UpdMaterial(self, ev, self.events.material_id) end,
            material_id = i_mat,
        }
    }),
})
end