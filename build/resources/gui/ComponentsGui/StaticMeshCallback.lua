if not StaticMeshCallback then StaticMeshCallback = {} end

function StaticMeshCallback.SetShadows(self, ev, cast)
    local history = {
        s_oldval = {},
        s_newval = false,
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.staticMesh:SetShadow(ent, self.s_oldval[i])
                end
                Properties:UpdateData(false, COMPONENTS.STATIC)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.staticMesh:SetShadow(ent, self.s_newval)
                end
                Properties:UpdateData(false, COMPONENTS.STATIC)
            end,
        msg = "Mesh cast shadow"
    }

    history.s_newval = cast
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.staticMesh:GetShadow(ent)
        Viewport.lua_world.world.staticMesh:SetShadow(ent, cast)
    end

    History:Push(history)
    return true
end

function StaticMeshCallback.UpdShadows(self, ev)
    local val = false
    for i, ent in ipairs(Viewport.selection_set) do
        local cast = Viewport.lua_world.world.staticMesh:GetShadow(ent)
        if i > 1 and val ~= cast then
            self:SetCheck(nil)
            return true
        else val = cast end
    end
    self:SetCheck(val)
    return true 
end

function StaticMeshCallback.SetMesh(self, ev)
    local path = self:GetPath()
    if path:len() == 0 then 
        self.events[GUI_EVENTS.UPDATE](self, ev)
        return true 
    end

    local history = {
        s_oldval = {},
        s_newval = "",
        
        s_oldmats = {},
        s_newmats = {},
        
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    Viewport.lua_world.world.staticMesh:SetMesh(ent, self.s_oldval[i])
                    local mat_count = Viewport.lua_world.world.staticMesh:GetMaterialsCount(ent)
                    for j = 0, mat_count - 1 do 
                         Viewport.lua_world.world.staticMesh:SetMaterial(ent, j, self.s_oldmats[i][j])
                    end
                    Viewport.lua_world.world.lineGeometry:SetFromVis(ent)
                end
                StaticMeshCallback.ListMaterials()
                Properties:UpdateData(false, COMPONENTS.STATIC)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    Viewport.lua_world.world.staticMesh:SetMesh(ent, self.s_newval)
                    local mat_count = Viewport.lua_world.world.staticMesh:GetMaterialsCount(ent)
                    for j = 0, mat_count - 1 do 
                         Viewport.lua_world.world.staticMesh:SetMaterial(ent, j, self.s_newmats[j])
                    end
                    Viewport.lua_world.world.lineGeometry:SetFromVis(ent)
                end
                StaticMeshCallback.ListMaterials()
                Properties:UpdateData(false, COMPONENTS.STATIC)
            end,
        msg = "Mesh file"
    }

    history.s_newval = path

    local mat_save = false
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.staticMesh:GetMesh(ent)
        local mat_count = Viewport.lua_world.world.staticMesh:GetMaterialsCount(ent)
        history.s_oldmats[i] = {}
        for j = 0, mat_count - 1 do 
            history.s_oldmats[i][j] = Viewport.lua_world.world.staticMesh:GetMaterial(ent, j)
        end
        
        Viewport.lua_world.world.staticMesh:SetMesh(ent, history.s_newval)
        Viewport.lua_world.world.lineGeometry:SetFromVis(ent)

        if not mat_save then
            local new_mat_count = Viewport.lua_world.world.staticMesh:GetMaterialsCount(ent)
            for j = 0, new_mat_count - 1 do 
                history.s_newmats[j] = Viewport.lua_world.world.staticMesh:GetMaterial(ent, j)
            end
        end
    end

    StaticMeshCallback.ListMaterials(self.entity:GetParent():GetInherited())
    History:Push(history)
    return true
end

function StaticMeshCallback.UpdMesh(self, ev)
    local val = ""
    for i, ent in ipairs(Viewport.selection_set) do
        local mesh = Viewport.lua_world.world.staticMesh:GetMesh(ent)
        if i > 1 and val ~= mesh then
            self:SetPath("")
            return true
        else val = mesh end
    end
    self:SetPath(val)
    StaticMeshCallback.ListMaterials(self.entity:GetParent():GetInherited())
    return true 
end

function StaticMeshCallback.ListMaterials(group)
    if not group then
        group = Properties.body.entity:GetChildById('static_mesh'):GetInherited()
    end

    local mat_count = 0
    for i, ent in ipairs(Viewport.selection_set) do
        local mesh_mat_count = Viewport.lua_world.world.staticMesh:GetMaterialsCount(ent)
        if mat_count == 0 then
            mat_count = mesh_mat_count
        else
            if mat_count ~= mesh_mat_count then
                if group.material_slots == nil then return end

                for j = #group.material_slots, 1, -1 do
                    group.entity:DetachChild(group.material_slots[j].entity)
                    group.material_slots[j].entity:Destroy()
                    table.remove(group.material_slots, j)
                end
                group:UpdateH(105)
                group.entity:GetParent():GetInherited().window.entity:UpdateSize()
                return
            end
        end
    end

    if group.material_slots == nil then group.material_slots = {} end

    for j = #group.material_slots, 1, -1 do
        group.entity:DetachChild(group.material_slots[j].entity)
        group.material_slots[j].entity:Destroy()
        table.remove(group.material_slots, j)
    end

    local hieght = 0
    for i = 0, mat_count - 1 do
        local arr_i = i + 1
        group.material_slots[arr_i] = Gui.MaterialSlot(i, hieght + 135)
        group.entity:AttachChild(group.material_slots[arr_i].entity)
        hieght = hieght + 45

        local mat_field = group.material_slots[arr_i].entity:GetChildById('slot_mat'):GetInherited()
        mat_field.events[GUI_EVENTS.UPDATE](mat_field, nil)
    end
    group:UpdateH(140 + hieght)
    group.entity:GetParent():GetInherited().window.entity:UpdateSize()
end

function StaticMeshCallback.SetMaterial(self, ev, mat_i)
    local path = self:GetPath()
    if path:len() == 0 then 
        self.events[GUI_EVENTS.UPDATE](self, ev, mat_i)
        return true 
    end

    path = path:gsub(".mta", ".mtb")
    self:SetPath(path)

    local history = {
        s_oldval = {},
        s_newval = "",
        
        s_mat_i = mat_i,
        
        undo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    if i > #self.s_oldval then return end
                    local mat_count = Viewport.lua_world.world.staticMesh:GetMaterialsCount(ent)
                    if self.s_mat_i < mat_count then
                        Viewport.lua_world.world.staticMesh:SetMaterial(ent, self.s_mat_i, self.s_oldval[i])
                    end
                end
                StaticMeshCallback.ListMaterials()
                Properties:UpdateData(false, COMPONENTS.STATIC)
            end,
        redo = function(self) 
                for i, ent in ipairs(Viewport.selection_set) do
                    local mat_count = Viewport.lua_world.world.staticMesh:GetMaterialsCount(ent)
                    if self.s_mat_i < mat_count then
                        Viewport.lua_world.world.staticMesh:SetMaterial(ent, self.s_mat_i, self.s_newval)
                    end
                end
                StaticMeshCallback.ListMaterials()
                Properties:UpdateData(false, COMPONENTS.STATIC)
            end,
        msg = "Material " .. mat_i
    }

    history.s_newval = path
    
    for i, ent in ipairs(Viewport.selection_set) do
        history.s_oldval[i] = Viewport.lua_world.world.staticMesh:GetMaterial(ent, mat_i)
        Viewport.lua_world.world.staticMesh:SetMaterial(ent, mat_i, history.s_newval)
    end

    History:Push(history)
    return true
end

function StaticMeshCallback.UpdMaterial(self, ev, mat_i)
    local val = ""
    for i, ent in ipairs(Viewport.selection_set) do
        local mat = Viewport.lua_world.world.staticMesh:GetMaterial(ent, mat_i)
        if i > 1 and val ~= mat then
            self:SetPath("")
            return true
        else val = mat end
    end
    self:SetPath(val)
    
    local current_mat = MaterialProps:GetSelected()
    if current_mat == nil then return true end

    if current_mat:GetName() == Viewport.lua_world.world.staticMesh:GetMaterialObject(Viewport.selection_set[1], mat_i):GetName() then
        local mat_btn = self.entity:GetParent():GetChildById('mat_btn'):GetInherited()
        StaticMeshCallback.UnselectAll(mat_btn, self.entity:GetParent())
        mat_btn:SetPressed(true)
    end
    return true 
end

function StaticMeshCallback.UnselectAll(self, mat_gui)
    local group = mat_gui:GetParent():GetInherited()
    for i, ent in ipairs(group.material_slots) do
        local mat_btn = ent.entity:GetChildById('mat_btn'):GetInherited()
        if not mat_btn.entity:is_eq(self.entity) then 
            mat_btn:SetPressed(false)
        end
    end
end

function StaticMeshCallback.SelectMat(self)
    local mat_gui = self.entity:GetParent()
    StaticMeshCallback.UnselectAll(self, mat_gui)

    if mat_gui:GetChildById('slot_mat'):GetInherited():GetPath():len() == 0 then
        MaterialProps:SetSelected(nil) 
        return true 
    end

    local material = Viewport.lua_world.world.staticMesh:GetMaterialObject(Viewport.selection_set[1], self.events.material_id)
    MaterialProps:SetSelected(material) 
    return true
end