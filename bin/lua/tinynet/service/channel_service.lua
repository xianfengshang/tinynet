local class = require("tinynet/core/class")
local gevent = require("tinynet/core/gevent")
local RpcStub = require("tinynet/rpc/rpc_stub")
local integer_to_string = integer_to_string

---@class ChannelSerivce:Object
local ChannelService = class("ChannelService")

function ChannelService:constructor()
    self.channels = {}
end

function ChannelService:create_channel(name)
    local channel = self.channels[name]
    if channel then
        return channel
    end
    local Channel = require("tinynet/service/channel")
    channel = Channel.new(name)
    self.channels[name] = channel
    return channel
end

function ChannelService:get_channel(name)
    return self.channels[name]
end

function ChannelService:destroy_channel(name)
    self.channels[name] = nil
end

--- Push message to a set of users
---@param route string
---@param msg table
---@param uids integer[]
function ChannelService:push_message_to_users(route, msg, uids)
    local Channel = require("tinynet/service/channel")
    ---@type Channel
    local channel = Channel.new()
    for _, v in ipairs(uids) do
        channel:add(v.uid, v.gate_id)
    end
    return channel:push_message_to_users(route, msg)
end

--- Push message to a single user specified by the given uid
---@param route any
---@param msg any
---@param gate_id any
---@param uid any
function ChannelService:push_message_to_user(route, msg, gate_id, uid)
    return RpcStub.Invoke(gate_id, "channel_remote", "push_message_to_user", route, msg, uid)
end

function ChannelService:push_messages_to_user(msgs, gate_id, uid)
    return RpcStub.Invoke(gate_id, "channel_remote", "push_messages_to_user", msgs, uid)
end

function ChannelService:broadcast(route, msg)
    return RpcStub.GroupInvoke("gate", "channel_remote", "broadcast", route, msg)
end

function ChannelService:broadcast_async(route, msg)
    gevent.pspawn(ChannelService.broadcast, self, route, msg)
end

function ChannelService:broadcast_by_tag(route, msg, tag_key, tag_value)
    return RpcStub.GroupInvoke("gate", "channel_remote", "broadcast_by_tag", route, msg, tag_key, tag_value)
end

function ChannelService:broadcast_by_tag_async(route, msg, tag_key, tag_value)
    gevent.pspawn(ChannelService.broadcast_by_tag, self, route, msg, tag_key, tag_value)
end

function ChannelService:broadcast_by_tag(route, msg, tag_key, tag_value)
    return RpcStub.GroupInvoke("gate", "channel_remote", "broadcast_by_tag", route, msg, tag_key, tag_value)
end

--- Push message to a set of sessions
---@param route any
---@param msg any
---@param sids any
function ChannelService:push_message_to_sessions(route, msg, sids)
    local Channel = require("tinynet/service/channel")
    local channel = Channel.new()
    for _, v in ipairs(sids) do
        channel:add(v.id, v.gate_id)
    end
    return channel:push_message_to_sessions(route, msg)
end


--- Push message to a single session specified by the given uid
---@param route any
---@param msg any
---@param gate_id any
---@param sid any
function ChannelService:push_message_to_session(route, msg, gate_id, sid)
    return RpcStub.Invoke(gate_id, "channel_remote", "push_message_to_session", route, msg, sid)
end

function ChannelService:push_messages_to_session(msgs, gate_id, sid)
    return RpcStub.Invoke(gate_id, "channel_remote", "push_messages_to_session", msgs, sid)
end

function ChannelService:push_messages_to_sessions(msgs, sids)
    local Channel = require("tinynet/service/channel")
    ---@type Channel
    local channel = Channel.new()
    for _, v in ipairs(sids) do
        channel:add(v.id, v.gate_id)
    end
    return channel:push_messages_to_sessions(msgs)
end

function ChannelService:push_message_to_session_async(route, msg, gate_id, sid)
    gevent.pspawn(ChannelService.push_message_to_session, self, route, msg, gate_id, sid) 
end

function ChannelService:push_messages_to_session_async(msgs, gate_id, sid)
    gevent.pspawn(ChannelService.push_messages_to_session, self, msgs, gate_id, sid) 
end

function ChannelService:push_message_to_sessions_async(route, msg, sids)
    gevent.pspawn(ChannelService.push_message_to_sessions, self, route, msg, sids) 
end

function ChannelService:push_messages_to_sessions_async(msgs, sids)
    gevent.pspawn(ChannelService.push_messages_to_sessions, self, msgs, sids) 
end

---@type ChannelSerivce
local exports = ChannelService.new()

return exports