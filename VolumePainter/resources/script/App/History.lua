if not History then History = {} end

function History:Init() -- to do, clamp history
    print("History:Init") 

    self.stack = {}
    self.current = 0
end

function History:clearRedo()
    if self.current == #self.stack then return end
    for i = #self.stack, self.current+1, -1 do
        table.remove(self.stack, i)
    end
end

function History:Push(history_event)
    self:clearRedo()
    self.stack[#self.stack+1] = deep_copy(history_event)
    self.current = #self.stack
    print("History pushed: ".. self.stack[self.current].msg)
end

function History:Clear()
    for i = #self.stack, 1, -1 do
        table.remove(self.stack, i)
    end
    self.current = 0
end

function History:Undo()
    if self.current < 1 then return end
    self.stack[self.current]:undo()
    print("Undo: ".. self.stack[self.current].msg)
    self.current = self.current - 1
end

function History:Redo()
    if self.current >= #self.stack then return end
    self.current = self.current + 1
    self.stack[self.current]:redo()
    print("Redo: ".. self.stack[self.current].msg)
end