if not DevConsole then DevConsole = {} end

GuiStyles.console_str = {
    styles = {
        GuiStyles.dead,
        GuiStyles.string_autosize,
        GuiStyles.string_16,
    },
    static = true,
    color = 'text_01',
    left = 5,
}

function DevConsole.CreateWindow()
return GuiWindow({
    styles = {
        GuiStyles.integrated_window,
    },

    background = {
        color = 'bg_01',
        color_live = 'bg_01',
        color_nonactive = 'bg_01',
    },

    border = {
        color = 'bg_01',
        color_live = 'bg_01',
        color_nonactive = 'bg_01',
    },

    independent = true,
    dragable = true,

    closeable = false,
    close = {
        styles = {
            GuiStyles.close_button_alpha,
        },
    },

    left = 400,
    top = 400,
    width = 700,
    height = 700,

    cleintarea_padding = { b = 34, },

    id = "console_window",

    header_size = 0,
    header = {
        styles = {
            GuiStyles.window_header,
        },

        str = "Developer's console",
    },
    
    events = {
        [GUI_EVENTS.KILL] = function(self, ev) 
            DevConsole:SysClose()
            return true
        end,
        [GUI_EVENTS.KEY_DOWN]  = function(self, ev)
            if ev.key == KEYBOARD_CODES.KEY_UP then
                DevConsole.currentCode = DevConsole.currentCode + 1
                if DevConsole.currentCode > #DevConsole.prevCode then DevConsole.currentCode = 0 end
                if DevConsole.currentCode == 0 then
                    DevConsole.codebar:SetText("")
                else
                    DevConsole.codebar:SetText(DevConsole.prevCode[#DevConsole.prevCode + 1 - DevConsole.currentCode])
                end
                return true
            end
            return false
            end,
    },

    GuiTextfield({
        styles = {
            GuiStyles.props_textfield,
        },
        data = {
            d_type = GUI_TEXTFIELD.TEXT,
        },
        text = {
            length = 1024,
        },
        bottom = 5,
        left = 5,
        right = 5,
        height = 24,
        valign = GUI_VALIGN.BOTTOM,
        align = GUI_ALIGN.BOTH,
        border = {
            width = 0
        },
        id = 'lua_field',

        events = {
            [GUI_EVENTS.TF_EDITING]  = function(self, ev)
                if self:GetText():len() ~= 0 then 
                    self.entity:GetChildById('lua_sign').enable = false
                else
                    self.entity:GetChildById('lua_sign').enable = true
                end
                return true
                end,
            [GUI_EVENTS.TF_DEACTIVATE]  = function(self, ev)
                if self:GetText():len() ~= 0 then 
                    DevConsole:Execute( self:GetText() )
                    self:SetText("")
                end
                self:SetActive(true)
                return true
                end,
        },

        GuiString({
            styles = {
                GuiStyles.ghost,
                GuiStyles.string_autosize,
                GuiStyles.string_18,
            },
        
            id = 'lua_sign',
            str = "Type Lua code here",
            static = true,
            color = 'text_02',
            color_nonactive = 'text_02',

            enable = true,

            top = 3,
            left = 3,
        }),        
    }),

    GuiClientarea({

        GuiBody({
            width_percent = true,
            width = 100,
            height = 0,

            }),
        }),
    })
end

function DevConsole.reload()
    if DevConsole.window ~= nil then
        local left = DevConsole.window.sys_win:GetLeft()
        local top = DevConsole.window.sys_win:GetTop()

        DevConsole.window:Close()
                
        DevConsole:Init()

        DevConsole.window.sys_win:SetPos(left, top)
        DevConsole.window.entity:UpdatePosSize()
    end
end

function DevConsole:Init()    
    self.err_color = Vector4(0.9, 0.2, 0.05, 1.0)
    self.wrn_color = Vector4(0.8, 0.7, 0.05, 1.0)
    self.lua_color = CoreGui.GetColor("act_03")
    self.dbg_color = CoreGui.GetColor("act_00")

    self.stringHeight = 18
    self.stringMaxCount = 100

    self.strings = {}
    self.stringsBegin = 1
    self.stringsSize = 0

    self.prevCode = {}
    self.currentCode = 0

    self.window = self.CreateWindow()
    self.body = self.window:GetBody()
    self.codebar = self.window.entity:GetChildById('lua_field'):GetInherited()
    self.codebar:SetActive(true)

    self.window.sys_win:SetMinMaxBox(false)
    --self.window.sys_win:SetAlpha(0.8)

    self:Fill(Util.Log.Size())
end

function DevConsole:IsInit()
    return self.window ~= nil
end

function DevConsole:Close()
    if self:IsInit() then 
        self.window:Close()
        self:SysClose()
    end
end

function DevConsole:SysClose()
    self.window = nil
end

function DevConsole:Execute(code)
    self.prevCode[#self.prevCode + 1] = code

    -- TODO: deque
    if #self.prevCode > 1000 then self.prevCode = {} end

    print(code)

    local func, errorMsg = loadstring(code)
    if func ~= nil then
        func()
    else
        error(errorMsg)
    end
end

function DevConsole:Fill(count)
    count = math.min(self.stringMaxCount, count)

    local newTop = -1

    local topOffset = self.body.entity.height
    local bufSize = Util.Log.Size()
    for i = bufSize - count, bufSize - 1 do
        local stringPos = self.stringsBegin + self.stringsSize
        if stringPos > self.stringMaxCount then stringPos = stringPos - self.stringMaxCount end

        if self.stringsSize >= self.stringMaxCount then
            newTop = math.max( self.strings[self.stringsBegin].entity.top, newTop )

            self.body.entity:DetachChild(self.strings[self.stringsBegin].entity)
            self.strings[self.stringsBegin].entity:Destroy()

            self.stringsBegin = self.stringsBegin + 1
            if self.stringsBegin > self.stringMaxCount then self.stringsBegin = 1 end
        else
            self.stringsSize = self.stringsSize + 1
        end

        self.strings[stringPos] = GuiString({
            styles = {GuiStyles.console_str,},
            str = Util.Log.Text(i),
            top = topOffset,
        })
        
        local prefix = Util.Log.Prefix(i)
        if prefix:find("ERROR") ~= nil then
            self.strings[stringPos].color = self.err_color
        elseif prefix:find("WARNING") ~= nil then
            self.strings[stringPos].color = self.wrn_color
        elseif prefix:find("LUA") ~= nil then
            self.strings[stringPos].color = self.lua_color
        elseif prefix:find("DEBUG") ~= nil then
            self.strings[stringPos].color = self.dbg_color
        end
        self.strings[stringPos]:UpdateProps()

        self.body.entity:AttachChild( self.strings[stringPos].entity )

        topOffset = topOffset + self.stringHeight        
    end

    if newTop >= 0 then
        newTop = newTop + self.stringHeight
        for i = 1, #self.strings do
            self.strings[i].entity.top = self.strings[i].entity.top - newTop
        end
        topOffset = topOffset - newTop
    end

    self.body.entity.height = topOffset
    self.window:SetScrollY(topOffset)
    self.window.entity:UpdatePosSize()
end

function DevConsole:Tick(dt)
    if not DevConsole:IsInit() then return end

    local bufUpdates = Util.Log.ResetUpdates()
    if bufUpdates == 0 then return end
    
    self:Fill(bufUpdates)
end