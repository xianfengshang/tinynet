local cluster = require("tinynet/core/cluster")
local unpack = unpack or table.unpack
local json = rjson
local HandlerService = require("tinynet/service/handler_service")
local RpcStub = require("tinynet/rpc/rpc_stub")
local Codec = require("tinynet/cluster/cluster_codec")

local exports = {}

local TYPE_REQUEST = "request"

local TYPE_RESPONSE = "response"

--- Parse cluster message body
exports.body_parser = function(ctx)
    ctx.body = Codec.decode(ctx.raw_body)
    if not ctx.body then
        return false
    end
end

-- Dispatch request message to handlers
exports.routes = function(ctx)
    if ctx.body.type == TYPE_REQUEST then
        --- Handle RPC request
        ctx.body.returns = {HandlerService.InvokeRemote(ctx.body.service, ctx.body.method, unpack(ctx.body.args))}
    else
        --- Handle RPC response
        RpcStub.EndInvoke(ctx.body)
        return false
    end
end

exports.body_writer = function(ctx)
end

exports.logger = function(ctx)
end

--- Respond will be executed after each request has been processed
exports.respond = function(ctx)
    if not ctx.body then
        return
    end
    if ctx.body.type == TYPE_REQUEST then
        local res = {}
        res.type = TYPE_RESPONSE
        res.seq = ctx.body.seq
        res.trace = ctx.body.trace
        res.returns = ctx.body.returns
        cluster.send_message(ctx.body.source, Codec.encode(res))
    end
    if ctx.body.trace then
        log.debug("Cluster message:%s", json.encode(ctx.body))
    end
end

exports.at_panic = function(ctx, e)
    local msg_text
    if ctx.body then
        msg_text = rjson.encode(ctx.body)
    else
        msg_text = bin2hex(ctx.raw_body)
    end
    log.error("Cluster msg error, msg:%s, err:%s", msg_text, tostring(e))
    return exports.respond(ctx)
end

return exports
