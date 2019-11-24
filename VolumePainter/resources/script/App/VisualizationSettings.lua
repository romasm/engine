if not VisualizationSettings then VisualizationSettings = {} end

function VisualizationSettings.reload()
    if VisualizationSettings.window then
        Tools.left_side_area.entity:DetachChild(VisualizationSettings.window.entity)
        VisualizationSettings.window.entity:Destroy()
        VisualizationSettings.window = nil
    end

	VisualizationSettings.window = Gui.VisualizationSettingsWindow()
    Tools.left_side_area.entity:AttachChild(VisualizationSettings.window.entity)
    
	Tools.left_side_area.second_win = VisualizationSettings.window.entity
	
	VisualizationSettings.body = VisualizationSettings.window:GetBody().entity

	VisualizationSettings.window.entity:UpdatePosSize()
    Tools.left_side_area.entity:UpdatePosSize()
end

function VisualizationSettings:Init()
    print("VisualizationSettings:Init") 

	loader.require("VisualizationWindow", VisualizationSettings.reload)
	self.reload()

	VisualizationSettings:Deactivate()
end

function VisualizationSettings:UpdateEvent()
	local ev = HEvent()
	ev.event = GUI_EVENTS.UPDATE
	VisualizationSettings.body:SendEvent(ev)  
end

function VisualizationSettings:Activate()
	VisualizationSettings.window.entity:Activate()
	VisualizationSettings:UpdateEvent()
end

function VisualizationSettings:Deactivate()
	VisualizationSettings.window.entity:Deactivate()
end