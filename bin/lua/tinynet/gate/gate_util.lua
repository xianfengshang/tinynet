local Message = require("tinynet/message")
local RouteUtil = require("tinynet/util/route_util")
local AppUtil = require("tinynet/util/app_util")
local RouteTable = require("public/protocol/RouteTable")
local Rpc = require("tinynet/rpc/rpc")

local high_resolution_time = high_resolution_time
local app_name = env.meta.name

local exports = {}

---此模块用到的错误码
local ErrorCode = {
    ERROR_OK = 0,

    ERROR_FAIL = 1,

    ERROR_INTERNAL_SERVER_ERROR = 3, -- //Internal Server Error

    ERROR_INVALID_ROUTE = 15
}

---消息包解析
---@param ctx TaskContext
---@return boolean
exports.body_parser = function(ctx)
    if not ctx.request:ParseFromString(ctx.raw_request) then
        return false
    end
    local type_info = Message.TypeMeta[ctx.request.type]
    if type_info ~= nil and not type_info.oneway then
        ctx.response = Message.new()
        ctx.response.seq = ctx.request.seq
        ctx.response.route = ctx.request.route
        ctx.response.type = type_info.ack
    end
end

---转发消息
--- Forward client message to the other service
---@param session Session
---@param request Message
---@param routeEntry table route info
---@return boolean,table
local function forward_message(session, request, routeEntry)
    return Rpc[routeEntry.app].msg_remote.forward_message(session:toFrontendSession(), routeEntry.service,
        routeEntry.method, request.body, session:export())
end

---转发消息
--- Forward client message to the other service
---@param session Session
---@param request Message
---@param routeEntry table route info
---@return boolean,table
local function forward_raw(session, request, routeEntry)
    local msg = {
        route = request.route,
        body = request.body
    }
    return Rpc[routeEntry.app].msg_remote.forward_message(session:toFrontendSession(), routeEntry.service,
        routeEntry.method, msg, session:export())

end

--- 处理本服务器消息
---@param session any
---@param request Message
---@param routeEntry any
local function handle_message(session, request, routeEntry)
    local HandlerService = require("tinynet/service/handler_service")
    return HandlerService.InvokeHandler(routeEntry.service, routeEntry.method, request.body, session:toFrontendSession())
end

--- 处理本服务器消息
---@param session any
---@param request Message
---@param routeEntry any
local function handle_raw(session, request, routeEntry)
    local msg = {
        route = request.route,
        body = request.body
    }
    local HandlerService = require("tinynet/service/handler_service")
    return HandlerService.InvokeHandler(routeEntry.service, routeEntry.method, msg, session:toFrontendSession())
end

---消息路由
---@param ctx TaskContext
---@return boolean
exports.routes = function(ctx)
    local route = ctx.request.route
    ctx.routeEntry = RouteTable[route].route
    if not ctx.routeEntry then
        if ctx.response then
            ctx.response.body = {
                errorCode = ErrorCode.ERROR_INVALID_ROUTE
            }
        end
        return false
    end
    local dispatcher
    if ctx.routeEntry.app == app_name then
        dispatcher = RouteTable[route].forward and handle_raw or handle_message
    else
        dispatcher = RouteTable[route].forward and forward_raw or forward_message
    end
    local res = dispatcher(ctx.session, {
        route = ctx.request.route,
        body = ctx.request.body
    }, ctx.routeEntry)
    if ctx.response then
        ctx.response.body = res
    end
    return true
end

local DEFAULT_CODEC_OPTS = {
    --pretty_format = get_os_name() == "Windows" 
}

local function format_msg(msg)
    return rjson.encode(msg, DEFAULT_CODEC_OPTS) .. '\n'
end

local function format_time()
    local t1, t2 = math.modf(time())
    return string.format("%s.%03d", os.date("%Y-%m-%d %X", t1), math.floor(t2 * 1000))
end

exports.message_logger = function(ctx)

    local info = {}
    info.uid = ctx.session.uid
    info.sid = ctx.session.id
    info.addr = ctx.session.addr
    info.route = ctx.request.route
    info.request = ctx.request:PlainObject()
    info.request_byte_size = ctx.request:ByteSize()
    info.request_body_size = ctx.request:BodySize()
    if ctx.response then
        info.response = ctx.response:PlainObject()
        info.response_byte_size = ctx.response:ByteSize()
        info.response_body_size = ctx.response:BodySize()
    end
    info.time = format_time()
    local messageFile = AppUtil.GetMessageFilePath()
    log.write_file_log(messageFile, format_msg(info))
    log.flush_file_log(messageFile)
end

exports.access_logger = function(ctx)
    local diff = high_resolution_time() - ctx.create_time
    local status
    if ctx.response and ctx.response.body then
        status = ctx.response.body.errorCode or ctx.response.body.err
    end
    if status == "" then
        status = "ERROR_OK"
    end
    if status == nil then
        status = 0
    end
    log.info("%s %s %s %s %.3f ms", ctx.addr, integer_to_string(ctx.session.uid), ctx.request.route, status,
        diff * 1000)
end

---请求响应
---@param ctx TaskContext
exports.after_each = function(ctx)
    if ctx.response then
        ctx.session:send_msg(ctx.response)
    end
end

return exports
