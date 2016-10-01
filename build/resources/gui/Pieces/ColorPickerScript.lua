loader.require("Pieces.ColorPicker")

if not ColorPicker then ColorPicker = {} end

local function ToInt(c, base)
    return math.floor(c * base + 0.5)
end

local function ToFloat(c, base)
    return c / base
end

function ColorPicker:Show(caller, color, allow_temperature)
    self.old_color = Vector4(ToInt(color.x, 255), ToInt(color.y, 255), ToInt(color.z, 255), 255)
    self.caller = caller
    self.allow_temperature = allow_temperature

    if not self.caller:is_a(GuiButton) then
        error("ColorPicker caller must be a GuiButton!")
        return 
    end

    self.init = false

    local root = self.caller.entity:GetRoot()

    self.window = Gui.ColorPicker()
    root:AttachChild(self.window.entity)
    root:SetHierarchyFocus(self.window.entity)
    
    if not self.allow_temperature then 
        self.window.entity.height = 239
    end

    local Grect = root:GetRectAbsolute()
    local rect = caller.entity:GetRectAbsolute()

    local win_w = self.window.entity.width
    local win_h = self.window.entity.height

    local win_left = rect.l - Grect.l
    if win_left + win_w > Grect.w then
        win_left = win_left - win_w + rect.w
    end
    self.window.entity.left = win_left

    local win_top = rect.t + rect.h + GUI_COLORPICKER.OFFSET - Grect.t
    if win_top + win_h > Grect.h then
        win_top = win_top - win_h - rect.h - GUI_COLORPICKER.OFFSET * 2
    end
    self.window.entity.top = win_top

    self.window.entity:UpdatePosSize()

    self.sv_rect = self.window.entity:GetChildById('SV_rect'):GetInherited()
    
    self.picked_rect = self.window.entity:GetChildById('picked_color'):GetInherited()
    self.h_text = self.window.entity:GetChildById('h_text'):GetInherited()
    self.s_text = self.window.entity:GetChildById('s_text'):GetInherited()
    self.v_text = self.window.entity:GetChildById('v_text'):GetInherited()
    self.r_text = self.window.entity:GetChildById('r_text'):GetInherited()
    self.g_text = self.window.entity:GetChildById('g_text'):GetInherited()
    self.b_text = self.window.entity:GetChildById('b_text'):GetInherited()
    self.t_text = self.window.entity:GetChildById('t_text'):GetInherited()
    self.pipet_btn = self.window.entity:GetChildById('pipet'):GetInherited()

    self.h_btn = self.window.entity:GetChildById('H_btn'):GetInherited()
    self.h_select_ent = self.h_btn.entity:GetChildById('H_select')
    self.h_select_dist = self.window.entity:GetChildById('H_rect').height
    self.h_selecl_size = self.h_select_ent.height / 2

    self.sv_btn = self.window.entity:GetChildById('SV_btn'):GetInherited()
    self.sv_select = self.sv_btn.entity:GetChildById('SV_select'):GetInherited()
    self.sv_select_h = self.window.entity:GetChildById('SV_rect').height
    self.sv_select_w = self.window.entity:GetChildById('SV_rect').width
    self.sv_selecl_size = self.sv_select.entity.height / 2

    self.t_btn = self.window.entity:GetChildById('T_btn'):GetInherited()
    self.t_select_ent = self.t_btn.entity:GetChildById('T_select')
    self.t_select_dist = self.window.entity:GetChildById('T_rect').width
    self.t_selecl_size = self.t_select_ent.width / 2

    if not self.allow_temperature then 
        self.t_text.entity.enable = false
        self.t_btn.entity.enable = false
        self.t_select_ent.enable = false
    end
    
    self:Reset()

    local reset_btn = self.window.entity:GetChildById('reset'):GetInherited()

    local btn_color = self:calcPipetColor()
    reset_btn.icon.color = Vector4(btn_color, btn_color, btn_color, 0)
    reset_btn.icon.color_hover = Vector4(btn_color, btn_color, btn_color, 1)
    reset_btn.icon.color_press = reset_btn.icon.color_hover
    reset_btn:UpdateProps()

    local old_rect = self.window.entity:GetChildById('old_color'):GetInherited()
    old_rect.background.color = self:GetColor()
    old_rect:UpdateProps()

    self.state_sv = false
    self.state_h = false
    self.state_t = false
    self.pipet = false
    self.pipet_time = 0

    self.init = true

    self.no_pick = true
end

function ColorPicker:GetColor()
    return Vector4(ToFloat(self.picked_color.x, 255), ToFloat(self.picked_color.y, 255), ToFloat(self.picked_color.z, 255), 1)
end

function ColorPicker:IsChanged()
    return not self.no_pick
end

function ColorPicker:SendPick()
    self.caller.background.color = self:GetColor()
    self.caller.background.color_hover = self.caller.background.color
    self.caller.background.color_press = self.caller.background.color
    self.caller:UpdateProps()

    local eventData = HEvent()
    eventData.event = GUI_EVENTS.COLOR_PICKING
    eventData.entity = self.window.entity
    self.caller:callback(eventData)

    self.no_pick = false
end

function ColorPicker:SendEnd()
    local eventData = HEvent()
    eventData.event = GUI_EVENTS.COLOR_PICKED
    eventData.entity = self.window.entity
    self.caller:callback(eventData)
end

function ColorPicker:SetH(h)
    self.hue = h
    self:updateColor(true)
end

function ColorPicker:SetS(s)
    self.saturation = s
    self:updateColor()
end

function ColorPicker:SetV(v)
    self.value = v
    self:updateColor()
end

function ColorPicker:SetR(r)
    self.picked_color.x = r
    self:updateHSV()
end

function ColorPicker:SetG(g)
    self.picked_color.y = g
    self:updateHSV()
end

function ColorPicker:SetB(b)
    self.picked_color.z = b
    self:updateHSV()
end

function ColorPicker:SetT(t)
    self.temperature = t
    self.picked_color.x, self.picked_color.y, self.picked_color.z = self.TtoRGB(t)
    self:updateHSV(true)
end

function ColorPicker:Reset()
    self.picked_color = Vector4(self.old_color.x, self.old_color.y, self.old_color.z, 1)
    self:updateHSV()
    self.no_pick = true
end

function ColorPicker:StartPipet()
    self.pipet = true
    CoreGui.Screen.StartColorRead()
end

function ColorPicker:EndPipet()
    self.pipet = false
    CoreGui.Screen.EndColorRead()
end

function ColorPicker:GetPipetColor()
    if not self.pipet then return end
    
    local current_time = Get_time()
    if current_time - self.pipet_time < COMMON_UPDATE_WAIT then return end
    self.pipet_time = current_time

    local color = CoreGui.Screen.ReadColorUnderCursor()
    self.picked_color.x = ToInt(color.x, 255)
    self.picked_color.y = ToInt(color.y, 255)
    self.picked_color.z = ToInt(color.z, 255)
    self:updateHSV()
end

function ColorPicker:onSVMove(btn, ev)
    if not self.state_sv then return end
    
    local rect = btn.entity:GetRectAbsolute()
    rect.l = rect.l + btn.border.width
    rect.t = rect.t + btn.border.width
    local double_border = btn.border.width * 2
    rect.w = rect.w - double_border
    rect.h = rect.h - double_border

    self.saturation = ToInt(math.max(0, math.min((ev.coords.x - rect.l) / rect.w, 1)), 100)
    self.value = ToInt(math.max(0, math.min(1 - (ev.coords.y - rect.t) / rect.h, 1)), 100)

    self:updateColor()
end

function ColorPicker:onHMove(btn, ev)
    if not self.state_h then return end
    
    local rect = btn.entity:GetRectAbsolute()
    rect.t = rect.t + btn.border.width
    rect.h = rect.h - btn.border.width * 2

    self.hue = ToInt(math.max(0, math.min(1 - (ev.coords.y - rect.t) / rect.h, 1)), 360)
    self:updateColor(true)
end

function ColorPicker:onTMove(btn, ev)
    if not self.state_t then return end
    
    local rect = btn.entity:GetRectAbsolute()
    rect.l = rect.l + btn.border.width
    rect.w = rect.w - btn.border.width * 2

    self.temperature = ToInt( math.max(0, math.min((ev.coords.x - rect.l) / rect.w, 1)), GUI_COLORPICKER.DIFF ) + GUI_COLORPICKER.MIN

    self.picked_color.x, self.picked_color.y, self.picked_color.z = self.TtoRGB(self.temperature)
    self:updateHSV(true)
end

function ColorPicker:updateColor(updateSVrect)
    if updateSVrect then
        local hr,hg,hb = self.HSVtoRGB(self.hue, 100, 100)
        self.sv_rect.background.color = Vector4(ToFloat(hr,255),ToFloat(hg,255),ToFloat(hb,255),1)
        self.sv_rect:UpdateProps()
    end

    local r,g,b = self.HSVtoRGB(self.hue, self.saturation, self.value)
    self.picked_color.x = r
    self.picked_color.y = g
    self.picked_color.z = b

    if self.allow_temperature then self.temperature = self.RGBtoT(self.picked_color.x, self.picked_color.y, self.picked_color.z) end

    self.picked_rect.background.color = self:GetColor()
    self.picked_rect:UpdateProps()

    local btn_color = self:calcPipetColor()
    self.pipet_btn.icon.color = Vector4(btn_color, btn_color, btn_color, 0)
    self.pipet_btn.icon.color_hover = Vector4(btn_color, btn_color, btn_color, 1)
    self.pipet_btn.icon.color_press = self.pipet_btn.icon.color_hover
    self.pipet_btn:UpdateProps()

    self:updateTextfields(r, g, b, self.hue, self.saturation, self.value, self.temperature)
    self:updateSelectors(btn_color)

    self:SendPick()
end

function ColorPicker:updateHSV(skipT)
    self.hue, self.saturation, self.value = self.RGBtoHSV(self.picked_color.x, self.picked_color.y, self.picked_color.z)
    if not skipT and self.allow_temperature then 
        self.temperature = self.RGBtoT(self.picked_color.x, self.picked_color.y, self.picked_color.z)
    end

    local r, g, b = ColorPicker.HSVtoRGB(self.hue, 100, 100)
    self.sv_rect.background.color = Vector4(ToFloat(r,255),ToFloat(g,255),ToFloat(b,255),1)
    self.sv_rect:UpdateProps()

    self.picked_rect.background.color = self:GetColor()
    self.picked_rect:UpdateProps()
    
    local btn_color = self:calcPipetColor()
    self.pipet_btn.icon.color = Vector4(btn_color, btn_color, btn_color, 0)
    self.pipet_btn.icon.color_hover = Vector4(btn_color, btn_color, btn_color, 1)
    self.pipet_btn.icon.color_press = self.pipet_btn.icon.color_hover
    self.pipet_btn:UpdateProps()

    self:updateTextfields(self.picked_color.x, self.picked_color.y, self.picked_color.z, 
        self.hue, self.saturation, self.value, self.temperature) 
    self:updateSelectors(btn_color)
    
    if self.init then self:SendPick() end
end

function ColorPicker:updateSelectors(color)
    if self.allow_temperature then 
        self.t_select_ent.left = self.t_btn.border.width + math.max(0, math.min( ToFloat(self.temperature - GUI_COLORPICKER.MIN, GUI_COLORPICKER.DIFF), 1)) * self.t_select_dist - self.t_selecl_size
        self.t_select_ent:UpdatePos()
    end

    self.h_select_ent.top = self.h_btn.border.width + (1 - ToFloat(self.hue, 360)) * self.h_select_dist - self.h_selecl_size
    self.h_select_ent:UpdatePos()

    self.sv_select.entity.top = self.sv_btn.border.width + (1 - ToFloat(self.value, 100)) * self.sv_select_h - self.sv_selecl_size
    self.sv_select.entity.left = self.sv_btn.border.width + ToFloat(self.saturation, 100) * self.sv_select_w - self.sv_selecl_size
    self.sv_select.entity:UpdatePos()
    self.sv_select.border.color = Vector4(color, color, color, 1)
    self.sv_select:UpdateProps()
end

function ColorPicker:updateTextfields(r,g,b,h,s,v,t)
    self.r_text:SetNum(r)
    self.g_text:SetNum(g)
    self.b_text:SetNum(b)
    local hh = h
    if hh > 359 then hh = 0 end 
    self.h_text:SetNum(hh)
    self.s_text:SetNum(s)
    self.v_text:SetNum(v)
    if self.allow_temperature then self.t_text:SetNum(t) end
end

function ColorPicker:calcPipetColor()
    local inv_value = (1 - ToFloat(self.value, 100)) * 2 + ToFloat(self.saturation, 100)
    if inv_value > 1 then return 1
    else return 0.0 end
end

function ColorPicker.HSVtoRGB(h, s, v)
    local h_scale = h / 60
    
    local hi = math.floor(h_scale)

    local v_min = (100 - s) * v / 100
    local a = (v - v_min) * (h_scale - hi)

    local v_inc = v_min + a
    local v_dec = v - a

    v_v = v / 100 * 255
    v_inc = v_inc / 100 * 255
    v_dec = v_dec / 100 * 255
    v_min = v_min / 100 * 255

    if hi == 0 or hi == 6 then return v_v, v_inc, v_min
    elseif hi == 1 then return v_dec, v_v, v_min
    elseif hi == 2 then return v_min, v_v, v_inc
    elseif hi == 3 then return v_min, v_dec, v_v
    elseif hi == 4 then return v_inc, v_min, v_v
    elseif hi == 5 then return v_v, v_min, v_dec end
    return 0,0,0
end

function ColorPicker.RGBtoHSV(r, g, b)
    local rr = ToFloat(r, 255)
    local gg = ToFloat(g, 255)
    local bb = ToFloat(b, 255)

    local rgb_max = math.max(rr, math.max(gg, bb))
    local rgb_min = math.min(rr, math.min(gg, bb))

    local h = 0
    if rgb_max == rgb_min then h = 0
    else
        local diff = rgb_max - rgb_min
        if rgb_max == rr then
            h = ((gg - bb) / diff) * 60
            if gg < bb then h = h + 360 end
        elseif rgb_max == gg then
            h = (bb - rr) / diff * 60 + 120
        else
            h = (rr - gg) / diff * 60 + 240
        end
    end
    
    local s = 0
    if rgb_max ~= 0 then s = 1 - rgb_min / rgb_max end
    
    return h, ToInt(s, 100), ToInt(rgb_max, 100)
end

function ColorPicker.TtoRGB(Temperature)
    local t = Temperature / 100
    local r, g, b = 0

    if t <= 66 then
        r = 255

        g = t
        g = 99.4708025861 * math.log(g) - 161.1195681661
		g = math.max(0, math.min(g, 255))

        if t <= 19 then
			b = 0
		else
			b = t - 10
			b = 138.5177312231 * math.log(b) - 305.0447927307
			b = math.max(0, math.min(b, 255))
        end
    else
        r = t - 60
        r = 329.698727466 * math.pow(r, -0.1332047592)
        r = math.max(0, math.min(r, 255))

        g = t - 60;
		g = 288.1221695283 * math.pow(g, -0.0755148492)
		g = math.max(0, math.min(g, 255))

        b = 255
    end

    return math.floor(r + 0.5), math.floor(g + 0.5), math.floor(b + 0.5)
end

function ColorPicker.RGBtoT(r, g, b)
    --local max = math.max(r, math.max(g, b))

    local rr = r
    local gg = g
    local bb = b

    if rr == 255 then
        if bb == 255 then return 6600 end
        gg = math.exp((gg + 161.1195681661) / 99.4708025861)
        if bb == 0 then
            return math.floor(gg * 100 + 0.5)
        else
            bb = math.exp((bb + 305.0447927307) / 138.5177312231) + 10
            return math.floor((bb + gg) * 50 + 0.5)
        end
    else
        rr = math.pow((rr / 329.698727466), -7.5072392759) + 60
        gg = math.pow((gg / 288.1221695283), -13.2424286163) + 60
        return math.floor((rr + gg) * 50 + 0.5)
    end
end