if not Hotkeys then Hotkeys = {} end

function Hotkeys:Init()
    print("Hotkeys:Init")  
    -- todo: key map support
end

function Hotkeys:Process(eventData)
    if eventData.key == KEYBOARD_CODES.KEY_Z and CoreGui.Keys.Ctrl() then 
        History:Undo()
        return
    end
    if eventData.key == KEYBOARD_CODES.KEY_Y and CoreGui.Keys.Ctrl() then 
        History:Redo()
        return
    end

    if eventData.key == KEYBOARD_CODES.KEY_Q then 
        Viewport:SetTransform(TRANSFORM_MODE.NONE, true)
        return 
    end
	if eventData.key == KEYBOARD_CODES.KEY_W then 
        Viewport:SetTransform(TRANSFORM_MODE.MOVE, true)
        return 
    end
	if eventData.key == KEYBOARD_CODES.KEY_E then 
        Viewport:SetTransform(TRANSFORM_MODE.ROT, true)
        return 
    end
	if eventData.key == KEYBOARD_CODES.KEY_R then 
        Viewport:SetTransform(TRANSFORM_MODE.SCALE, true)
        return 
    end
	if eventData.key == KEYBOARD_CODES.KEY_SPACE then 
        Viewport:SwitchTransform()
        return 
    end
	if eventData.key == KEYBOARD_CODES.KEY_G then 
        Viewport:SwitchHud()
        return 
    end

end