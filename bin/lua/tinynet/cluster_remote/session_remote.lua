local SessionService = require("tinynet/service/session_service")

local exports = {}

--- Bind session that specified by sid to the giving uid
exports.bind = function (sid, uid)
    return SessionService:bind(sid, uid)
end

--- Remove a previously-binded uid from the session specified by sid
exports.unbind = function (sid, uid)
    return SessionService:unbind(sid, uid)
end

exports.push = function (sid, key, value)
    return SessionService:push(sid, key, value)
end

exports.sync = function (sid, data)
    return SessionService:sync(sid, data)
end

exports.get_session_by_sid = function(sid)
    local session = SessionService:get(sid)
    return session:export()
end

exports.get_session_by_uid = function(uid)
    local session = SessionService:get_by_uid(uid)
    return session:export()
end

exports.kick_session = function (sid, reason)
    return SessionService:kick_session(sid, reason)
end

exports.kick_user = function (uid, reason)
    return SessionService:kick_user(uid, reason)
end

return exports
