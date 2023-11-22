local gevent = require("tinynet/core/gevent")
local class = require("tinynet/core/class")
local integer_to_string = integer_to_string

---@class SessionService:Object
local SessionService = class("SessionService")

function SessionService:constructor()
    ---@type table<string, Session>
    self.sessions = {}
    ---@type table<integer, Session>
    self.uids = {}
end

function SessionService:create(sid, gate_id, addr, server)
    local Session = require("tinynet/service/session")
    ---@type Session
    local session = Session.new(sid, gate_id, addr, server)
    self.sessions[sid] = session
    return session
end

---Get session
---@param sid string
---@return Session
function SessionService:get(sid)
    return self.sessions[sid]
end

function SessionService:get_by_uid(uid)
    return self.uids[uid]
end

function SessionService:remove(sid)
    local session = self.sessions[sid]
    if session ~= nil then
        self.sessions[sid] = nil
        if session.uid > 0 then
            self.uids[session.uid] = nil
            local frontend_session = session:toFrontendSession()
            gevent.pspawn(frontend_session.logout, frontend_session)
        end
    end
end

function SessionService:bind(sid, uid)
    local session = self.sessions[sid]
    if session == nil then
        log.warning("Session %s not found", sid)
        return false
    end
    if session.uid > 0 then
        if session.uid == uid then
            return true
        end
        log.warning("Session %s has been already bound to %s", sid, integer_to_string(uid))
        return false
    end
    session.uid = uid
    self.uids[uid] = session -- create or replace uid session mapping
    return true
end

function SessionService:unbind(sid, uid)
    local session = self.sessions[sid]
    if session == nil then
        log.warning("Session %s not found", sid)
        return false
    end
    if session.uid == 0 then
        log.warning("Session %s has not been bound uid  %d", sid, session.uid, uid)
        return false
    end
    if session.uid ~= uid then
        log.warning("Session %s has been bound to uid %d but not %d", sid, session.uid, uid)
        return false
    end
    session.uid = 0
    if self.uids[uid] and self.uids[uid].id == sid then
        self.uids[uid] = nil
    end
    return true
end

function SessionService:push(sid, key, value)
    local session = self.sessions[sid]
    if session == nil then
        log.warning("Session %s not found", sid)
        return false
    end
    session.data[key] = value
    return true
end

function SessionService:sync(sid, data)
    local session = self.sessions[sid]
    if session == nil then
        log.warning("Session %s not found", sid)
        return false
    end
    session.data = table.clone(data.data)
    if session.uid == 0 and data.uid ~= 0 then
        return self:bind(sid, data.uid)
    end
    if session.uid ~= 0 and data.uid == 0 then
        return self:unbind(sid, data.uid)
    end
    return true
end

function SessionService:kick_session(sid, reason)
    local session = self.sessions[sid]
    if session == nil then
        return
    end
    log.info(string.format("Session %s kick by server, reason:%s", sid, reason or "kick"))
    session:close()
end

function SessionService:kick_user(uid, reason)
    local session = self.uids[uid]
    if session == nil then
        return
    end
    log.info(string.format("User %s kick by server, reason:%s", integer_to_string(uid), reason or "kick"))
    session:close()
end

function SessionService:close_session(sid)
    local session = self.sessions[sid]
    if not session then
        return
    end
    session:close()
end

---@type SessionService
local exports = SessionService.new()

return exports
