if not Properties then Properties = {} end

function Properties.reload()
    if Properties.window then
        Tools.left_side_area.entity:DetachChild(Properties.window.entity)
        Properties.window.entity:Destroy()
        Properties.window = nil
    end

	Properties.window = Gui.ToolPropertiesWindow()
	Tools.left_side_area.entity:AttachChild(Properties.window.entity)
    
	Tools.left_side_area.first_win = Properties.window.entity

    Properties.window.entity:UpdatePosSize()

	Properties.body = Properties.window:GetBody().entity
	Properties.none_msg = Properties.window.entity:GetChildById('none_msg')
	Properties.brush_tab = Properties.body:GetChildById('brush_settings')
	Properties.plane_tab = Properties.body:GetChildById('plane_settings')

	Tools.left_side_area.entity:UpdatePosSize()
end

function Properties:Init()
    print("Properties:Init") 
    	
	loader.require("ToolProperties", Properties.reload)
	self.reload()
end

function Properties:UpdateEvent()
	local ev = HEvent()
	ev.event = GUI_EVENTS.UPDATE
	self.body:SendEvent(ev)  
end

function Properties:EnableBrush()
	self.none_msg.enable = false
	self.brush_tab.enable = true
	self.plane_tab.enable = false

	self.window:SetHeader(lcl.brush_header)
	self:UpdateEvent()
end

function Properties:EnablePlane()
	self.none_msg.enable = false
	self.brush_tab.enable = false
	self.plane_tab.enable = true

	self.window:SetHeader(lcl.plane_header)
	self:UpdateEvent()
end

function Properties:Disable()
	self.none_msg.enable = true
	self.brush_tab.enable = false
	self.plane_tab.enable = false

	self.window:SetHeader(lcl.property_header)
end
