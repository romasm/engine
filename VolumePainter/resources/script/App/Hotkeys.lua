if not Hotkeys then Hotkeys = {} end

function Hotkeys:Init()
    print("Hotkeys:Init")  
    -- todo: key map support
end

function Hotkeys:Process(eventData, root)
    local resEvent = eventData
    if eventData.key == KEYBOARD_CODES.KEY_C and CoreGui.Keys.Ctrl() then 
        local fake_event = HEvent()
        fake_event.event = GUI_EVENTS.HK_COPY
        root.entity:SendEventOnFocus(fake_event)
        resEvent.event = GUI_EVENTS.NULL
    elseif eventData.key == KEYBOARD_CODES.KEY_X and CoreGui.Keys.Ctrl() then 
        local fake_event = HEvent()
        fake_event.event = GUI_EVENTS.HK_CUT
        root.entity:SendEventOnFocus(fake_event)
        resEvent.event = GUI_EVENTS.NULL
    elseif eventData.key == KEYBOARD_CODES.KEY_V and CoreGui.Keys.Ctrl() then 
        local fake_event = HEvent()
        fake_event.event = GUI_EVENTS.HK_PASTE
        root.entity:SendEventOnFocus(fake_event)
        resEvent.event = GUI_EVENTS.NULL
    elseif eventData.key == KEYBOARD_CODES.KEY_N and CoreGui.Keys.Ctrl() then 
        local fake_event = HEvent()
        fake_event.event = GUI_EVENTS.HK_NEW
        root.entity:SendEventOnFocus(fake_event)
        resEvent.event = GUI_EVENTS.NULL
    elseif eventData.key == KEYBOARD_CODES.KEY_F2 then 
        local fake_event = HEvent()
        fake_event.event = GUI_EVENTS.HK_RENAME
        root.entity:SendEventOnFocus(fake_event)
        resEvent.event = GUI_EVENTS.NULL
    elseif eventData.key == KEYBOARD_CODES.KEY_DELETE then 
        local fake_event = HEvent()
        fake_event.event = GUI_EVENTS.HK_DELETE
        root.entity:SendEventOnFocus(fake_event)
        resEvent.event = GUI_EVENTS.NULL

    elseif eventData.key == KEYBOARD_CODES.KEY_Z and CoreGui.Keys.Ctrl() then 
		--History:Undo()
		VolumeWorld:Undo()
		resEvent.event = GUI_EVENTS.NULL
    elseif eventData.key == KEYBOARD_CODES.KEY_Y and CoreGui.Keys.Ctrl() then 
		--History:Redo()
		VolumeWorld:Redo()
		resEvent.event = GUI_EVENTS.NULL

	elseif eventData.key == KEYBOARD_CODES.KEY_1 then 
		Tools:SetToolMode(TOOL_MODE.BRUSH)
		resEvent.event = GUI_EVENTS.NULL
	elseif eventData.key == KEYBOARD_CODES.KEY_2 then
		Tools:SetToolMode (TOOL_MODE.PLANE)
		resEvent.event = GUI_EVENTS.NULL

	elseif eventData.key == KEYBOARD_CODES.KEY_W then 
		Tools:SetTransform(TRANSFORM_MODE.MOVE)
        resEvent.event = GUI_EVENTS.NULL
    elseif eventData.key == KEYBOARD_CODES.KEY_E then 
		Tools:SetTransform(TRANSFORM_MODE.ROT)
        resEvent.event = GUI_EVENTS.NULL
    elseif eventData.key == KEYBOARD_CODES.KEY_R then 
		Tools:SetTransform(TRANSFORM_MODE.SCALE)
        resEvent.event = GUI_EVENTS.NULL
        
    elseif eventData.key == KEYBOARD_CODES.KEY_F and CoreGui.Keys.Alt() then 
        Viewport:ToggleFullscreen()
        resEvent.event = GUI_EVENTS.NULL

    elseif eventData.key == KEYBOARD_CODES.KEY_G and CoreGui.Keys.Ctrl() then 
        Viewport:ToggleGamemode()
        resEvent.event = GUI_EVENTS.NULL

    elseif eventData.key == KEYBOARD_CODES.KEY_G then 
        Viewport:SwitchHud()
        resEvent.event = GUI_EVENTS.NULL

    elseif eventData.key == KEYBOARD_CODES.KEY_L then 
        Viewport:ToggleWorldLive()
        resEvent.event = GUI_EVENTS.NULL

    elseif eventData.key == KEYBOARD_CODES.KEY_Z then 
        Viewport:MoveCameraToSelection()
        resEvent.event = GUI_EVENTS.NULL

    elseif eventData.key == KEYBOARD_CODES.KEY_F and CoreGui.Keys.Ctrl() then 
        Viewport:ToggleFullscreen()
        resEvent.event = GUI_EVENTS.NULL
        
    elseif eventData.key == KEYBOARD_CODES.KEY_GR then 
        if DevConsole:IsInit() then
            DevConsole:Close()
        else
            DevConsole:Init()
        end
        resEvent.event = GUI_EVENTS.NULL

    end

    return resEvent
end