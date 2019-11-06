GuiStyles.dialog_button = {
    styles = {GuiStyles.solid_button,},
    background = {
        color = 'bg_01',
        color_hover = 'act_00',
        color_press = 'act_00',
        color_nonactive = 'bg_01',
    },
    text = {
        font = "../resources/fonts/opensans_normal_25px",
        color = 'text_01',
        color_hover = 'act_03',
        color_press = 'act_03',
        color_nonactive = 'text_02',
    },
    width = 100,
    height = 30,
    align = GUI_ALIGN.CENTER,
    valign = GUI_VALIGN.MIDDLE,
}

function Gui.DialogOk(root, dialogStr, overwriteSize, okFunc)
local dialog = GuiRect({
    styles = {GuiStyles.live,},
    width_percent = true,
    height_percent = true,
    width = 100,
    height = 100,
    focus_mode = GUI_FOCUS_MODE.ONTOP,
    background = {color = Vector4(0.0,0.0,0.0,0.5),},
    id = 'dialog_bg',

    GuiRect({
        styles = {GuiStyles.ghost,},
        width = math.max(400, overwriteSize ~= nil and overwriteSize or 0),
        height = 180,
        background = {color = Vector4(0.0,0.0,0.0,0.7),},
        align = GUI_ALIGN.CENTER,
        valign = GUI_VALIGN.MIDDLE,


    }),

    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_25,
        },

        str = dialogStr,

        static = true,
        align = GUI_ALIGN.CENTER,
        valign = GUI_VALIGN.MIDDLE,
        top = -35,

        color = 'text_01',
    }),
    
    GuiButton({
        styles = {GuiStyles.dialog_button,},

        left = 0,
        top = 35,
        id = 'ok_btn',
        text = {
            str = "OK",
        },
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                local dialogEnt = self.entity:GetParent()
                local root = dialogEnt:GetParent()
                root:DetachChild(dialogEnt)
                dialogEnt:Destroy()

                if okFunc ~= nil then okFunc() end
                return true
            end,
        },
    }),
})

    root:AttachChild(dialog.entity)
    dialog.entity:UpdatePosSize()
    dialog.entity:SetHierarchyFocusOnMe(true)
end

function Gui.DialogOkCancel(root, dialogStr, overwriteSize, okFunc, cancelFunc)
local dialog = GuiRect({
    styles = {GuiStyles.live,},
    width_percent = true,
    height_percent = true,
    width = 100,
    height = 100,
    focus_mode = GUI_FOCUS_MODE.ONTOP,
    background = {color = Vector4(0.0,0.0,0.0,0.5),},
    id = 'dialog_bg',

    GuiRect({
        styles = {GuiStyles.ghost,},
        width = math.max(400, overwriteSize ~= nil and overwriteSize or 0),
        height = 180,
        background = {color = Vector4(0.0,0.0,0.0,0.7),},
        align = GUI_ALIGN.CENTER,
        valign = GUI_VALIGN.MIDDLE,


    }),

    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_25,
        },

        str = dialogStr,

        static = true,
        align = GUI_ALIGN.CENTER,
        valign = GUI_VALIGN.MIDDLE,
        top = -35,

        color = 'text_01',
    }),
    
    GuiButton({
        styles = {GuiStyles.dialog_button,},

        left = -100,
        top = 35,
        id = 'ok_btn',
        text = {
            str = "OK",
        },
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                local dialogEnt = self.entity:GetParent()
                local root = dialogEnt:GetParent()
                root:DetachChild(dialogEnt)
                dialogEnt:Destroy()

                if okFunc ~= nil then okFunc() end
                return true
            end,
        },
    }),
    
    GuiButton({
        styles = {GuiStyles.dialog_button,},

        left = 100,
        top = 35,
        id = 'cancel_btn',
        text = {
            str = "Cancel",
        },
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                local dialogEnt = self.entity:GetParent()
                local root = dialogEnt:GetParent()
                root:DetachChild(dialogEnt)
                dialogEnt:Destroy()
                
                if cancelFunc ~= nil then cancelFunc() end
                return true
            end,
        },
    }),
})

    root:AttachChild(dialog.entity)
    dialog.entity:UpdatePosSize()
    dialog.entity:SetHierarchyFocusOnMe(true)
end

function Gui.DialogYesNoCancel(root, dialogStr, overwriteSize, yesFunc, noFunc, cancelFunc)
local dialog = GuiRect({
    styles = {GuiStyles.live,},
    width_percent = true,
    height_percent = true,
    width = 100,
    height = 100,
    focus_mode = GUI_FOCUS_MODE.ONTOP,
    background = {color = Vector4(0.0,0.0,0.0,0.5),},
    id = 'dialog_bg',

    GuiRect({
        styles = {GuiStyles.ghost,},
        width = math.max(440, overwriteSize ~= nil and overwriteSize or 0),
        height = 180,
        background = {color = Vector4(0.0,0.0,0.0,0.7),},
        align = GUI_ALIGN.CENTER,
        valign = GUI_VALIGN.MIDDLE,


    }),

    GuiString({
        styles = {
            GuiStyles.ghost,
            GuiStyles.string_autosize,
            GuiStyles.string_25,
        },

        str = dialogStr,

        static = true,
        align = GUI_ALIGN.CENTER,
        valign = GUI_VALIGN.MIDDLE,
        top = -35,

        color = 'text_01',
    }),
    
    GuiButton({
        styles = {GuiStyles.dialog_button,},

        left = -120,
        top = 35,
        id = 'yes_btn',
        text = {
            str = "Yes",
        },
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                local dialogEnt = self.entity:GetParent()
                local root = dialogEnt:GetParent()
                root:DetachChild(dialogEnt)
                dialogEnt:Destroy()

                if yesFunc ~= nil then yesFunc() end
                return true
            end,
        },
    }),
    
    GuiButton({
        styles = {GuiStyles.dialog_button,},

        left = 0,
        top = 35,
        id = 'no_btn',
        text = {
            str = "No",
        },
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                local dialogEnt = self.entity:GetParent()
                local root = dialogEnt:GetParent()
                root:DetachChild(dialogEnt)
                dialogEnt:Destroy()
                
                if noFunc ~= nil then noFunc() end
                return true
            end,
        },
    }),
    
    GuiButton({
        styles = {GuiStyles.dialog_button,},

        left = 120,
        top = 35,
        id = 'cancel_btn',
        text = {
            str = "Cancel",
        },
        events = {
            [GUI_EVENTS.BUTTON_PRESSED] = function(self, ev) 
                self.entity:SetHierarchyFocusOnMe(false)
                local dialogEnt = self.entity:GetParent()
                local root = dialogEnt:GetParent()
                root:DetachChild(dialogEnt)
                dialogEnt:Destroy()
                
                if cancelFunc ~= nil then cancelFunc() end
                return true
            end,
        },
    }),
})

    root:AttachChild(dialog.entity)
    dialog.entity:UpdatePosSize()
    dialog.entity:SetHierarchyFocusOnMe(true)
end