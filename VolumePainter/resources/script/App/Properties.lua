if not Properties then Properties = {} end

loader.require("ToolProperties")

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

    local body = Properties.window:GetBody().entity
    Properties.body = body:GetInherited()
    Properties.none_msg = Properties.window.entity:GetChildById('none_msg')
    --Properties:Update()

    Tools.right_side_area.entity:UpdatePosSize()
end

function Properties:Init()
    print("Properties:Init") 
    
    self.reload()
end

function Properties:Tick(dt)
    
end
