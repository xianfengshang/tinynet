local class = require("tinynet/core/class")

---@class Session:Object
local Session = class("Session")

function Session:constructor(sid, gate_id, addr, server)
    self.id = sid
    self.addr = addr
    self.gate_id = gate_id
    self.server = server

    self.uid = 0
    self.data = {}
end

function Session:export()
    local o = {}
    o.id = self.id
    o.gate_id = self.gate_id
    o.uid = self.uid
    o.data = table.clone(self.data)
    o.addr = self.addr
    return o
end

--Send msg
function Session:send_msg(msg)
    if self.server == nil  then
      return
    end
    local tp = type(msg)
    if tp == "table" then
        local data
        if msg.SerializeToString then
            data = msg:SerializeToString()
        else
            data = rjson.encode(msg)
        end
        self.server:send_bytes(self.id, data)
    else
        self.server:send_bytes(self.id, msg)
    end
end

--- Convert to FrontendSession
---@return FrontendSession
function Session:toFrontendSession()
    local FrontendSession = require("tinynet/service/frontend_session")
    return FrontendSession.new(self)
end

function Session:close()
    if self.server ~= nil then
        self.server:close_session(self.id)
    end
end

return Session
