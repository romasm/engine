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

    VisualizationSettings.window.entity:UpdatePosSize()
    Tools.left_side_area.entity:UpdatePosSize()
end

function VisualizationSettings:Init()
    print("VisualizationSettings:Init") 

	loader.require("VisualizationWindow", VisualizationSettings.reload)
    self.reload()

end