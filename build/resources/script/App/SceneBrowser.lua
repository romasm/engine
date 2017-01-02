if not SceneBrowser then SceneBrowser = {} end

function SceneBrowser.reload()
    if SceneBrowser.window then
        Tools.right_side_area.entity:DetachChild(SceneBrowser.window.entity)
        SceneBrowser.window.entity:Destroy()
        SceneBrowser.window = nil
    end

    SceneBrowser.window = Gui.SceneBrowserWindow()
    Tools.right_side_area.entity:AttachChild(SceneBrowser.window.entity)
    
    Tools.right_side_area.second_win = SceneBrowser.window.entity
    
    SceneBrowser.body = SceneBrowser.window:GetBody().entity
    SceneBrowser.none_msg = SceneBrowser.window:GetClient().entity:GetChildById('none_msg')
    SceneBrowser.selected_counter = SceneBrowser.window.entity:GetChildById('selected_count')

    SceneBrowser.window.entity:UpdatePosSize()

    Tools.right_side_area.entity:UpdatePosSize()
end

function SceneBrowser:Init()
    print("SceneBrowser:Init") 
        
    loader.require("SceneBrowser", SceneBrowser.reload)
    self.reload()
    
    self:Deactivate()
end

function SceneBrowser:Deactivate()
    self.window.entity:Deactivate()
    self.none_msg.enable = true
end

function SceneBrowser:Activate()
    self.window.entity:Activate()
    self.none_msg.enable = false
end