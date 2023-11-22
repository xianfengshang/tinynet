local AppUtil = require("tinynet/util/app_util")
local Message = require("tinynet/message")
local SessionService = require("tinynet/service/session_service")

local DEFAULT_CODEC_OPTS = {
    -- pretty_format = get_os_name() == "Windows"
}

local function format_msg(msg)
    return rjson.encode(msg, DEFAULT_CODEC_OPTS) .. '\n'
end

local function format_time()
    local t1, t2 = math.modf(time())
    return string.format("%s.%03d", os.date("%Y-%m-%d %X", t1), math.floor(t2 * 1000))
end

local exports = {}

-- Push message to the client specified by the given uid
exports.push_message_to_user = function(route, msg_body, uid)
    local session = SessionService:get_by_uid(uid)
    if not session then
        return "Session not found"
    end
    local msg = Message.new()
    msg.type = Message.TYPE_PUSH
    msg.route = route
    msg.body = msg_body
    session:send_msg(msg)

    if AppUtil.enable_log_message() then
        local info = {}
        info.uids = {uid}
        info.push = msg:PlainObject()
        info.time = format_time()
        local messageFile = AppUtil.GetMessageFilePath()
        log.write_file_log(messageFile, format_msg(info))
    end
end

exports.push_messages_to_user = function(msgs, uid)
    local session = SessionService:get_by_uid(uid)
    if session == nil then
        return "Session not found"
    end
    local debug_mode = AppUtil.enable_log_message()
    local debug_info
    if debug_mode then
        debug_info = {}
        debug_info.uids = {uid}
        debug_info.sids = {session.id}
        debug_info.push = {}
        debug_info.time = format_time()
    end
    for k, v in ipairs(msgs) do
        local msg = Message.new()
        msg.type = Message.TYPE_PUSH
        msg.route = v.route
        msg.body = v.body
        session:send_msg(msg)
        if debug_info then
            table.insert(debug_info.push, msg:PlainObject())
        end
    end
    if debug_mode then
        local messageFile = AppUtil.GetMessageFilePath()
        log.write_file_log(messageFile, format_msg(debug_info))
    end
end

-- Push message to clients by the given uids
exports.push_message_to_users = function(route, msg_body, uids)
    local fails = {}
    local session
    local msg = Message.new()
    msg.type = Message.TYPE_PUSH
    msg.route = route
    msg.body = msg_body
    local bin = msg:SerializeToString()
    for _, v in ipairs(uids) do
        session = SessionService:get_by_uid(v)
        if session then
            session:send_msg(bin)
        else
            table.insert(fails, v)
        end
    end
    if AppUtil.enable_log_message() then
        local info = {}
        info.uids = uids
        info.push = msg:PlainObject()
        info.time = format_time()
        local messageFile = AppUtil.GetMessageFilePath()
        log.write_file_log(messageFile, format_msg(info))
    end
    return fails
end

-- Broadcast to all clients
exports.broadcast = function(route, msg_body)
    local msg = Message.new()
    msg.type = Message.TYPE_PUSH
    msg.route = route
    msg.body = msg_body
    local bin = msg:SerializeToString()
    local app = AppUtil.get_app()
    app.gate_service:broadcast(bin)

    if AppUtil.enable_log_message() then
        local info = {}
        info.broadcast = msg:PlainObject()
        info.time = format_time()
        local messageFile = AppUtil.GetMessageFilePath()
        log.write_file_log(messageFile, format_msg(info))
    end
end

-- Broadcast to the clients specified by the tag
exports.broadcast_by_tag = function(route, msg_body, tag_key, tag_value)
    local msg = Message.new()
    msg.type = Message.TYPE_PUSH
    msg.route = route
    msg.body = msg_body
    local bin = msg:SerializeToString()
    for k, v in pairs(SessionService.sessions) do
        if v.data[tag_key] == tag_value then
            v:send_msg(bin)
        end
    end
    if AppUtil.enable_log_message() then
        local info = {}
        info.broadcast = msg:PlainObject()
        info.tag = {
            key = tag_key,
            value = tag_value
        }
        info.time = format_time()
        local messageFile = AppUtil.GetMessageFilePath()
        log.write_file_log(messageFile, format_msg(info))
    end
end

-- Push message to the client specified by the given sid
exports.push_message_to_session = function(route, msg_body, sid)
    local session = SessionService:get(sid)
    if session == nil then
        return nil, "Session not found"
    end
    local msg = Message.new()
    msg.type = Message.TYPE_PUSH
    msg.route = route
    msg.body = msg_body
    session:send_msg(msg)
    if AppUtil.enable_log_message() then
        local info = {}
        info.uids = {}
        if session.uid > 0 then
            table.insert(info.uids, session.uid)
        end
        info.sids = {sid}
        info.push = msg:PlainObject()
        info.time = format_time()
        local messageFile = AppUtil.GetMessageFilePath()
        log.write_file_log(messageFile, format_msg(info))
    end
end

exports.push_messages_to_session = function(msgs, sid)
    local session = SessionService:get(sid)
    if session == nil then
        return nil, "Session not found"
    end
    local debug_mode = AppUtil.enable_log_message()
    local debug_info
    if debug_mode then
        debug_info = {}
        debug_info.uids = {}
        if session.uid > 0 then
            table.insert(debug_info.uids, session.uid)
        end
        debug_info.sids = {sid}
        debug_info.push = {}
        debug_info.time = format_time()
    end
    for k, v in ipairs(msgs) do
        local msg = Message.new()
        msg.type = Message.TYPE_PUSH
        msg.route = v.route
        msg.body = v.body
        session:send_msg(msg)
        if debug_info then
            table.insert(debug_info.push, msg:PlainObject())
        end
    end
    if debug_mode then
        local messageFile = AppUtil.GetMessageFilePath()
        log.write_file_log(messageFile, format_msg(debug_info))
    end
end

-- Push message to clients by the given sids
exports.push_message_to_sessions = function(route, msg_body, sids)
    local fails = {}
    local session
    local msg = Message.new()
    msg.type = Message.TYPE_PUSH
    msg.route = route
    msg.body = msg_body
    local bin = msg:SerializeToString()
    for _, v in ipairs(sids) do
        session = SessionService:get(v)
        if session then
            session:send_msg(bin)
        else
            table.insert(fails, v)
        end
    end
    if AppUtil.enable_log_message() then
        local info = {}
        info.uids = {}
        info.sids = sids
        info.push = msg:PlainObject()
        for _, v in ipairs(sids) do
            session = SessionService:get(v)
            if session and session.uid > 0 then
                table.insert(info.uids, session.uid)
            end
        end
        info.time = format_time()
        local messageFile = AppUtil.GetMessageFilePath()
        log.write_file_log(messageFile, format_msg(info))
    end
    return fails
end

-- Push multi messages to clients by the given sids
exports.push_messages_to_sessions = function(msgs, sids)
    local fails = {}
    local session
    local debug_mode = AppUtil.enable_log_message()
    local debug_info
    if debug_mode then
        debug_info = {}
        debug_info.uids = {}
        for k, v in ipairs(sids) do
            session = SessionService:get(v)
            if session and session.uid > 0 then
                table.insert(debug_info.uids, session.uid)
            end
        end
        debug_info.sids = sids
        debug_info.push = {}
        debug_info.time = format_time()
    end
    for k, v in ipairs(msgs) do
        local msg = Message.new()
        msg.type = Message.TYPE_PUSH
        msg.route = v.route
        msg.body = v.body
        for k1, v1 in ipairs(sids) do
            session = SessionService:get(v1)
            if session then
                session:send_msg(msg)
            else
                fails[v1] = true
            end
        end
        if debug_info then
            table.insert(debug_info.push, msg:PlainObject())
        end
    end
    if debug_mode then
        local messageFile = AppUtil.GetMessageFilePath()
        log.write_file_log(messageFile, format_msg(debug_info))
    end
    return table.keys(fails)
end

return exports
