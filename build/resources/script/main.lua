script_env = {
    path = './../resources/script/',
    guipath = './../resources/gui/',
    src_ext = '.lua',
    cpl_ext = '.luac',
}
package.path = script_env.path..'?'..script_env.cpl_ext..';'..
    script_env.guipath..'?'..script_env.cpl_ext..';'..
    script_env.path..'?'..script_env.src_ext..';'..
    script_env.guipath..'?'..script_env.src_ext..';'

require "const_pathes"

Gui = {}
Main = {}

loader.require("common")

-- base entities
loader.require("GuiEntities.const_GuiDefines")
loader.require("GuiEntities.GuiStyles")

loader.require("GuiEntities.GuiEntity", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiRoot", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiButton", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiCheck", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiCheckGroup", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiRect", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiString", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiTextfield", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiSlider", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiDataSlider", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiGroup", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiMenu", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiCombo", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiWindow", CoreGui.UpdateLuaFuncs)
loader.require("GuiEntities.GuiFilefield", CoreGui.UpdateLuaFuncs)

-- gui pieces
loader.require("Pieces.ColorPickerScript")
loader.require("Pieces.Texture") -- todo
loader.require("Pieces.DialogScreen")


-- world entities
loader.require("Entities.const_EntityDefines")
loader.require("Entities.BaseEntity", Core.UpdateLuaFuncs)

loader.require("Entities.Mesh", Core.UpdateLuaFuncs)
loader.require("Entities.LocalLight", Core.UpdateLuaFuncs)
loader.require("Entities.GlobalLight", Core.UpdateLuaFuncs)
loader.require("Entities.StaticModel", Core.UpdateLuaFuncs)
loader.require("Entities.PhysicsModel", Core.UpdateLuaFuncs)
loader.require("Entities.Camera", Core.UpdateLuaFuncs)
loader.require("Entities.SkinnedMesh", Core.UpdateLuaFuncs)
loader.require("Entities.EnvProb", Core.UpdateLuaFuncs)

--test
loader.require("Entities.TestEnt", Core.UpdateLuaFuncs)
loader.require("Entities.TestPlayer", Core.UpdateLuaFuncs)


-- app logic
loader.require("App.const_Defines")
loader.require("App.MainWindow")
loader.require("App.History")
loader.require("App.Hotkeys")

loader.require("App.SceneMgr")
loader.require("App.Viewport")
loader.require("App.Tools")
loader.require("App.Importer")

loader.require("App.Properties")
loader.require("App.MaterialProps")
loader.require("App.AssetBrowser")
loader.require("App.SceneBrowser")

loader.require("App.Profiler")
loader.require("App.DevConsole")

loader.require("resource_preloader")


function Main:Start()	
    print("Main:Start")  
    
    Main:LoadLocalization()

    PreloadList:PreloadResources()
    
    Importer:Init()
    MainWindow:Init()
    History:Init()
    Hotkeys:Init()
    SceneMgr:Init()
end

function Main:onTick(dt) 
    -------- remove in consumer
    loader.check_modif()
    Profiler:Tick(dt)
    DevConsole:Tick(dt)
    --------

    MainWindow:Tick(dt)

    return true
end

function Main:LoadLocalization()
    local path = Config.GetString('localization')
    loader.require("Localization."..path)
end