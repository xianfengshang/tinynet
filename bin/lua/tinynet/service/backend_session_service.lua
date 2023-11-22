
local class = require("tinynet/core/class")
local RpcStub = require("tinynet/rpc/rpc_stub")

---@class BackendSessionService:Object
local BackendSessionService = class("BackendSessionService")

function BackendSessionService:constructor()
end

function BackendSessionService:create(opts)
    local BackendSession = require("tinynet/service/backend_session")
    return BackendSession.new(opts)
end

function BackendSessionService:bind(gate_id, sid, uid)
    return RpcStub.Invoke(gate_id, "session_remote", "bind", sid, uid)
end

function BackendSessionService:unbind(gate_id, sid, uid)
    return RpcStub.Invoke(gate_id, "session_remote", "unbind", sid, uid)
end

function BackendSessionService:push(gate_id, sid, key, value)
    return RpcStub.Invoke(gate_id, "session_remote", "push", sid, key, value)
end

function BackendSessionService:sync(gate_id, sid, data)
    return RpcStub.Invoke(gate_id, "session_remote", "sync", sid, data)
end

function BackendSessionService:get_session_by_sid(gate_id, sid)
    return RpcStub.Invoke(gate_id, "session_remote", "get_session_by_sid", sid)
end

function BackendSessionService:get_session_by_uid(gate_id, uid)
    return RpcStub.Invoke(gate_id, "session_remote", "get_session_by_uid", uid)
end

function BackendSessionService:kick_session(gate_id, sid)
    return RpcStub.Invoke(gate_id, "session_remote", "kick_session", sid)
end

function BackendSessionService:kick_user(gate_id, uid)
    return RpcStub.Invoke(gate_id, "session_remote", "kick_user", uid)
end

---@type BackendSessionService
local exports = BackendSessionService.new()

return exports
