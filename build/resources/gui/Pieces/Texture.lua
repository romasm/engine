function Gui.Texture(props)
local res = GuiDumb({
    styles = {
        GuiStyles.live,
        props,
    },
    height = 100,
    id = props.id,

    events = props.events,

    GuiRect({
        styles = {GuiStyles.ghost,},
        width = 100,
        height = 100,
        border = {
            color = 'act_05',
            color_nonactive = 'text_02',
            width = 1
        },
        background = {
            color = 'bg_05',
            color_nonactive = 'bg_03'
        },

        GuiString({
            styles = {
                GuiStyles.ghost,
                GuiStyles.string_autosize,
                GuiStyles.string_16,
            },
            str = "No texture",
            static = true,
            top = 18,
            align = GUI_ALIGN.CENTER,
            valign = GUI_VALIGN.MIDDLE,
            color = 'text_02',
        }),
        GuiString({
            styles = {
                GuiStyles.ghost,
                GuiStyles.string_autosize,
                GuiStyles.string_16,
            },
            enable = props.str ~= nil,
            str = props.str == nil and "" or props.str,
            static = true,
            top = -18,
            align = GUI_ALIGN.CENTER,
            valign = GUI_VALIGN.MIDDLE,
            color = 'text_02',
        }),
    }),

    GuiRect({
        styles = {GuiStyles.ghost,},
        enable = false,
        width = 98,
        height = 98,
        left = 1,
        top = 1,
        material = props.material == nil and GuiMaterials.texture or props.material,
        id = 'texture_rect'
    }),

    GuiFilefield({
        styles = {GuiStyles.common_filefield,},
        left = 110,
        right = 0,
        top = 0,
        align = GUI_ALIGN.BOTH,
        browse_header = (props.str == nil and "" or props.str) .." texture open",
        filetypes = {
            {"All supported", "*.bmp;*.dds;*.gif;*.jpg;*.tga;*.tif;*.png;"},
            {"BMP", "*.bmp;"},
            {"DDS", "*.dds;"},
            {"GIF", "*.gif;"},
            {"JPEG", "*.jpg;"},
            {"TGA", "*.tga;"},
            {"TIFF", "*.tif;"},
            {"PNG", "*.png;"},
        },
        id = "texture_field",

        events = {
            [GUI_EVENTS.FF_SET] = function(self, ev)
                    local tr = self.entity:GetParent():GetChildById('texture_rect'):GetInherited()
                    tr.entity.enable = false
                    
                    if tr.rect_mat:SetTextureNameByID(self:GetPath(), 0, SHADERS.PS) == true then 
                        tr.entity.enable = true
                    end

                    ev.event = GUI_EVENTS.TEXTURE_SET
                    return false 
                end,
            [GUI_EVENTS.FF_RELOAD] = function(self, ev)
                    local tr = self.entity:GetParent():GetChildById('texture_rect'):GetInherited()
                    -- reloading
                    ev.event = GUI_EVENTS.TEXTURE_RELOAD
                    return false 
                end,
            [GUI_EVENTS.FF_DELETE] = function(self, ev)
                    local tr = self.entity:GetParent():GetChildById('texture_rect'):GetInherited()
                    tr.rect_mat:ClearTextures()
                    tr.entity.enable = false
                    ev.event = GUI_EVENTS.TEXTURE_DELETE
                    return false 
                end,
        }
    }),
})

res.GetTexture = function(self)
        local ff = self.entity:GetChildById("texture_field"):GetInherited()
        return ff:GetPath()
    end
res.SetTexture = function(self, texture)
        local ff = self.entity:GetChildById("texture_field"):GetInherited()
        ff:SetPath(texture)
        
        local tr = self.entity:GetChildById('texture_rect'):GetInherited()
        tr.entity.enable = false
        if texture == nil then 
            tr.rect_mat:SetTextureNameByID("", 0, SHADERS.PS)
        else
            tr.entity.enable = tr.rect_mat:SetTextureNameByID(texture, 0, SHADERS.PS)
        end
    end
    
return res
end