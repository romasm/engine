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

require "class"
require "script_loader"

Gui = {}
Main = {}

loader.require("common_math")

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


-- world entities
loader.require("Entities.BaseEntity", Core.UpdateLuaFuncs)

loader.require("Entities.LocalLight", Core.UpdateLuaFuncs)
loader.require("Entities.GlobalLight", Core.UpdateLuaFuncs)
loader.require("Entities.StaticModel", Core.UpdateLuaFuncs)

--test
loader.require("Entities.TestEnt", Core.UpdateLuaFuncs)


-- app logic
loader.require("App.const_Defines")
loader.require("App.MainWindow")
loader.require("App.History")
loader.require("App.Hotkeys")

loader.require("App.SceneMgr")
loader.require("App.Viewport")
loader.require("App.Tools")

loader.require("App.Properties")
loader.require("App.MaterialProps")

loader.require("App.Profiler")



function Main:Start()	
    print("Main:Start")  

    MainWindow:Init()
    History:Init()
    Hotkeys:Init()
    SceneMgr:Init()

    self.reload_time = 0
end

function Main:onTick(dt) 
    -- remove in consumer
	self.reload_time = self.reload_time + dt

    if self.reload_time > 2000 then
        loader.check_modif()
        self.reload_time = 0
    end

    Profiler:Tick(dt)
    --------

    MainWindow:Tick(dt)
end
