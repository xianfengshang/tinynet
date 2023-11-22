local class = require("tinynet/core/class")
local BackendSessionService = require("tinynet/service/backend_session_service")
local ChannelService = require("tinynet/service/channel_service")

---@class BackendSession:SessionConcept
local BackendSession = class("BackendSession")

function BackendSession:constructor(data)
    self:init(data)
end

function BackendSession:init(data)
    self.id = data.id
    self.gate_id = data.gate_id
    self.uid = data.uid
    self.data = table.clone(data.data)
    self.addr = data.addr
    self.player = nil
end

function BackendSession:export()
    local o = {}
    o.id = self.id
    o.gate_id = self.gate_id
    o.uid = self.uid
    o.data = table.clone(self.data)
    o.addr = self.addr
    return o
end

function BackendSession:is_connected()
    return self.id ~= nil and self.gate_id ~= nil
end

function BackendSession:push(key)
    if self:is_connected() then
        return BackendSessionService:push(self.gate_id, self.id, key, self.data[key])
    end
end

function BackendSession:sync()
    if self:is_connected() then
        return BackendSessionService:sync(self.gate_id, self.id, self:export())
    end
end

function BackendSession:bind(uid)
    self.uid = uid
    if self:is_connected() then
        BackendSessionService:bind(self.gate_id, self.id, uid)
    end
end

function BackendSession:unbind()
    if self:is_connected() then
        BackendSessionService:unbind(self.gate_id, self.id, self.uid)
    end
    self.uid = 0
end

function BackendSession:update()
    local res = BackendSessionService:get_session_by_sid(self.gate_id, self.id)
    self:init(res)
end


function BackendSession:push_message(route, msg, async)
    if not self:is_connected() then
        return false
    end
    if async then
        return ChannelService:push_message_to_session_async(route, msg, self.gate_id, self.id)
    else
        return ChannelService:push_message_to_session(route, msg, self.gate_id, self.id)
    end
end

function BackendSession:push_messages(msgs, async)
    if not self:is_connected() then
        return false
    end
    if async then
        return ChannelService:push_messages_to_session_async(msgs, self.gate_id, self.id)
    else
        return ChannelService:push_messages_to_session(msgs, self.gate_id, self.id)
    end
end

return BackendSession
