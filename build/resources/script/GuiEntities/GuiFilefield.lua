if not GuiFilefield then GuiFilefield = class(GuiEntity) end

-- public
function GuiFilefield:init(props)
    self.filetypes = {}
    self.allow_edit = true
    self.allow_none = true
    self.browse_header = ""

    self.ChildFilepath = {}
    self.ChildBrowse = {}
    self.ChildReload = {}
    self.ChildDelete = {}
    
    self._base.init(self, props)
    
    if self.allow_none then
        self.ChildDelete.align = GUI_ALIGN.RIGHT
        self.ChildDelete.valign = GUI_VALIGN.TOP
        self.ChildDelete.right = 0
        self.ChildDelete.top = 0
        self.ChildDelete.height = 100
        self.ChildDelete.height_percent = true
        if self.ChildDelete.width == nil then 
            self.ChildDelete.width = 20
        end
    end

    self.ChildReload.align = GUI_ALIGN.RIGHT
    self.ChildReload.valign = GUI_VALIGN.TOP
    self.ChildReload.right = (self.allow_none and self.ChildDelete.width or 0)
    self.ChildReload.top = 0
    self.ChildReload.height = 100
    self.ChildReload.height_percent = true
    if self.ChildReload.width == nil then 
        self.ChildReload.width = 20
    end

    self.ChildBrowse.align = GUI_ALIGN.RIGHT
    self.ChildBrowse.valign = GUI_VALIGN.TOP
    self.ChildBrowse.right = (self.allow_none and self.ChildDelete.width or 0) + self.ChildReload.width
    self.ChildBrowse.top = 0
    self.ChildBrowse.height = 100
    self.ChildBrowse.height_percent = true
    if self.ChildBrowse.width == nil then 
        self.ChildBrowse.width = 20
    end

    self.ChildFilepath.align = GUI_ALIGN.BOTH
    self.ChildFilepath.valign = GUI_VALIGN.TOP
    self.ChildFilepath.left = 0
    self.ChildFilepath.right = (self.allow_none and self.ChildDelete.width or 0) + self.ChildReload.width + self.ChildBrowse.width
    self.ChildFilepath.top = 0
    self.ChildFilepath.height = 100
    self.ChildFilepath.height_percent = true
    self.ChildFilepath.allow_none = self.allow_none
    self.ChildFilepath.show_tail = true

    self.filepath_tf = GuiTextfield(self.ChildFilepath)
    self.entity:AttachChild(self.filepath_tf.entity)

    self.browse_btn = GuiButton(self.ChildBrowse)
    self.entity:AttachChild(self.browse_btn.entity)

    self.reload_btn = GuiButton(self.ChildReload)
    self.entity:AttachChild(self.reload_btn.entity)
    
    if self.allow_none then
        self.delete_btn = GuiButton(self.ChildDelete)
        self.entity:AttachChild(self.delete_btn.entity)
    end
    
    if not self.allow_edit then self.ChildFilepath.entity:Deactivate() end

    self.filefilter = dlgFilter()
    for i, ext in ipairs(self.filetypes) do 
	    self.filefilter:Add(ext[1], ext[2])
    end
end

function GuiFilefield:ApplyProps(props)
    self._base.ApplyProps(self, props)

    if props.ChildFilepath ~= nil then self.ChildFilepath = deep_copy(props.ChildFilepath) end
    if props.ChildBrowse ~= nil then self.ChildBrowse = deep_copy(props.ChildBrowse) end
    if props.ChildReload ~= nil then self.ChildReload = deep_copy(props.ChildReload) end
    if props.ChildDelete ~= nil then self.ChildDelete = deep_copy(props.ChildDelete) end

    if props.filetypes ~= nil then self.filetypes = props.filetypes end

    if props.allow_edit ~= nil then self.allow_edit = props.allow_edit end
    if props.allow_none ~= nil then self.allow_none = props.allow_none end
    if props.browse_header ~= nil then self.browse_header = props.browse_header end
end

function GuiFilefield:callback(eventData)
    local res = eventData
    
    if not self.entity:IsActivated() or not self.entity:IsActivatedBranch() then
        if eventData.event == GUI_EVENTS.MOUSE_DOWN then
            self.entity:SetHierarchyFocusOnMe(false)
            res.event = GUI_EVENTS.DO_DENIED
        elseif not (eventData.event == GUI_EVENTS.UPDATE or eventData.event == GUI_EVENTS.SYS_MOVE or 
            eventData.event == GUI_EVENTS.SYS_RESIZE or eventData.event == GUI_EVENTS.MOUSE_WHEEL) then
            res.event = GUI_EVENTS.NULL
        end
        res.entity = self.entity
        return self._base.callback(self, res)
    end

    if eventData.event == GUI_EVENTS.BUTTON_PRESSED then 
        if eventData.entity:is_eq(self.browse_btn.entity) then
            local win = CoreGui.SysWindows.GetByEntity(self.entity)
            local path = dlgOpenFile(win:GetHWND(), self.browse_header, self.filefilter)

            if path:len() > 0 and self.filepath_tf:GetText() ~= path then
                self.filepath_tf:SetText(path)
                res.event = GUI_EVENTS.FF_SET
            else
                res.event = GUI_EVENTS.NULL
            end
            res.entity = self.entity

        elseif eventData.entity:is_eq(self.reload_btn.entity) then
            if self.filepath_tf:GetText():len() > 0 then
                res.event = GUI_EVENTS.FF_RELOAD
            else
                res.event = GUI_EVENTS.NULL
            end
            res.entity = self.entity

        elseif self.delete_btn then
            if eventData.entity:is_eq(self.delete_btn.entity) then
                if self.filepath_tf:GetText():len() > 0 then
                    self.filepath_tf:SetText("")
                    res.event = GUI_EVENTS.FF_DELETE
                else
                    res.event = GUI_EVENTS.NULL
                end
                res.entity = self.entity
           end
        end
        
    elseif eventData.event == GUI_EVENTS.TF_DEACTIVATE then 
        if eventData.entity:is_eq(self.filepath_tf.entity) then
            if self.filepath_tf:IsChanged() then
                if self.filepath_tf:GetText():len() == 0 then
                    res.event = self.allow_none and GUI_EVENTS.FF_DELETE or GUI_EVENTS.FF_SET
                else
                    res.event = GUI_EVENTS.FF_SET
                end
            else
                res.event = GUI_EVENTS.NULL
            end
            res.entity = self.entity
        end

    elseif eventData.event == GUI_EVENTS.UNFOCUS then
        if eventData.entity:is_eq(self.entity) then self.entity:SetFocus(HEntity()) end

    end

    return self._base.callback(self, res)
end

function GuiFilefield:GetPath()
    return self.filepath_tf:GetText()
end

function GuiFilefield:SetPath(path)
    self.filepath_tf:SetText(path)
end