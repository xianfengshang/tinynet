local AppUtil = require("tinynet/util/app_util")
local WebRouter = require("tinynet/web/web_router")
local StatusCode =
{
    NOT_FOUND = 404
}

local exports = WebRouter.new()

exports.get["/debugger"] = function(ctx)
    local app = AppUtil.get_app()
    local ip = "127.0.0.1"  --默认本机ip
    local port = 8818       --默认8818端口
    local sid = app.app_id --默认调试本进程
    if type(ctx.req.query) == "table" then
        ip = ctx.req.query.ip or ip
        port = ctx.req.query.port or port
        sid = ctx.req.query.sid or sid
    end
    if AppUtil.is_publish_mode() then
        ctx.res.body = "Debug is only allow in development mode!"
        return
    end
    --发起RPC调用, 让目标进程连接调试器
    local RpcStub = require("tinynet/rpc/rpc_stub")
    RpcStub.Invoke(sid, "sys_remote", "StartDebugger", ip, port)
    ctx.res.body = ""
end

exports.get["/services"] = function(ctx)
    local app = AppUtil.get_app()
    local services = app:GetApps()
    ctx.res.body = table.concat(services, "\n")
end

return exports
