-- entity type
if not GuiCell then GuiCell = class(GuiEntity) end

function GuiCell:init()
    local props = {}
    self._base.init(self, props)

    self.childs = {}
    self.orient = TABLE_ORIENT.LEFT

    self.divide = TABLE_DIV.BOTH
end

function GuiCell:Add(guiEnt, orient)
    if #self.childs >= 2 then return end

    if not guiEnt.entity.width_percent and not guiEnt.entity.height_percent then return end

    if #self.childs == 0 then
        if orient ~= TABLE_ORIENT.BOTTOM then
            guiEnt.entity.valign = GUI_VALIGN.TOP
        else
            guiEnt.entity.valign = GUI_VALIGN.BOTTOM
        end
        if orient ~= TABLE_ORIENT.RIGHT then
            guiEnt.entity.align = GUI_ALIGN.LEFT
        else
            guiEnt.entity.align = GUI_ALIGN.RIGHT
        end
        guiEnt.entity.left = 0
        guiEnt.entity.right = 0
        guiEnt.entity.top = 0
        guiEnt.entity.bottom = 0

        if guiEnt.entity.width_percent then
            guiEnt.entity.width = 100
            if not guiEnt.entity.height_percent then
                self.divide = TABLE_DIV.HORZ
            end
        end

        if guiEnt.entity.height_percent then
            guiEnt.entity.height = 100
            if not guiEnt.entity.width_percent then
                self.divide = TABLE_DIV.VERT
            end
        end

        local entParent = guiEnt.entity:GetParent()
        if not entParent:is_null() then
            entParent:DetachChild(guiEnt.entity)
        end
        self.entity:AttachChild(guiEnt.entity)

        self.childs[#self.childs+1] = guiEnt
    else
        
        -----------------------
        -- TODO
    end
end

-- table
if not WinTable then WinTable = {} end

function WinTable:Init()
    
end