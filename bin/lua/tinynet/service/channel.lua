local class = require("tinynet/core/class")
local exception = require("tinynet/core/exception")
local table_find = table.find
local table_insert = table.insert
local table_remove = table.remove
local table_append = table.append

---@class Channel:Object
local Channel = class("Channel")

function Channel:constructor(name)
    self.name = name
    self.groups = {}
    self.members = {}
end

function Channel:group_add(group_id, member_id)
    if not self.groups[group_id] then
        self.groups[group_id] = {}
    end
    if table_find(self.groups[group_id], member_id) then
        return false
    end
    table_insert(self.groups[group_id], member_id)
    return true
end

function Channel:group_remove(group_id, member_id)
    if not self.groups[group_id] then
        return false
    end
    return table_remove( self.groups[group_id], table_find(self.groups[group_id], member_id)) ~= nil
end

function Channel:add(member_id, group_id)
    if not self:group_add(group_id, member_id ) then
        return false
    end
    self.members[member_id] = group_id
    return true
end

function Channel:remove(member_id, group_id)
    if not self:group_remove(group_id, member_id) then
        return false
    end
    self.members[member_id] = nil
    return true
end

function Channel:push_message(route, msg, method)
    local RpcStub = require("tinynet/rpc/rpc_stub")
    local status, res
    local fails = {}
    for k, v in pairs(self.groups) do
        status, res = try(RpcStub.Invoke, k, "channel_remote", method, route, msg, v)()
        if status then
            if res ~= nil then
                table_append(fails, res)
            end
        else
            log.warning("Push message failed, route:%s, group:%s, err:%s", route, k, res)
        end
    end
    if #fails > 0 then
        log.warning("Push message partial failed, route:%s, fails:[%s]", route, table.concat(fails, ','))
    end
end

function Channel:push_message_to_users(route, msg)
    return self:push_message(route, msg, "push_message_to_users")
end

function Channel:push_message_to_sessions(route, msg)
    return self:push_message(route, msg, "push_message_to_sessions")
end

function Channel:push_messages_to_sessions(msgs)
    local RpcStub = require("tinynet/rpc/rpc_stub")
    local status, res
    local fails = {}
    for k, v in pairs(self.groups) do
        status, res = try(RpcStub.Invoke, k, "channel_remote", "push_messages_to_sessions", msgs, v)()
        if status then
            if res ~= nil and #res > 0 then
                table_append(fails, res)
            end
        else
            log.warning("Push messages failed, msgs:%s, group:%s, err:%s", rjson.encode(msgs), k, res)
        end
    end
    if #fails > 0 then
        log.warning("Push messages partial failed, msgs:%s, fails:[%s]", rjson.encode(msgs), table.concat(fails, ','))
    end    
end

return Channel
