local class = require("tinynet/core/class")
local SessionService = require("tinynet/service/session_service")
local HandlerService = require("tinynet/service/handler_service")

---@class FrontendSession:SessionConcept
local FrontendSession = class("FrontendSession")

function FrontendSession:constructor(data)
    self:init(data)
end

function FrontendSession:init(data)
    self.id = data.id
    self.gate_id = data.gate_id
    self.uid = data.uid
    self.data = table.clone(data.data)
    self.addr = data.addr
end

function FrontendSession:export()
    local o = {}
    o.id = self.id
    o.gate_id = self.gate_id
    o.uid = self.uid
    o.data = table.clone(self.data)
    o.addr = self.addr
    return o
end

function FrontendSession:is_connected()
    return self.id ~= nil and self.gate_id ~= nil
end

function FrontendSession:push(key)
    return SessionService:push(self.id, key, self.data[key])
end

function FrontendSession:sync()
    return SessionService:sync(self.id, self:export())
end

function FrontendSession:bind(uid)
    SessionService:bind(self.id, uid)
    self.uid = uid
end

function FrontendSession:unbind()
    SessionService:unbind(self.id, self.uid)
    self.uid = 0
end

function FrontendSession:logout()
    return HandlerService.InvokeRemote('login_remote', 'logout', self)
end

function FrontendSession:update()
    return self:init(SessionService:get(self.id))
end

function FrontendSession:close()
    return SessionService:close_session(self.id)
end

function FrontendSession:push_message(route, msg, async)
    return HandlerService.InvokeRemote("channel_remote", "push_message_to_session", route, msg, self.id)
end

function FrontendSession:push_messages(msgs, async)
    return HandlerService.InvokeRemote("channel_remote", "push_messages_to_session", msgs, self.id)
end

return FrontendSession
