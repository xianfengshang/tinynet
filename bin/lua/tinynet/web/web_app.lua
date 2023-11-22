--@brief 网站类应用
local class = require("tinynet/core/class")
local gevent = require("tinynet/core/gevent")
local exception = require("tinynet/core/exception")
local WebContext = require("tinynet/web/web_context")
local tinynet = tinynet

exception.HttpServerStartFailedException = "HttpServerStartFailedException"

---@class WebApp:Object
local WebApp = class("WebApp")

local StatusCode =
{
    OK = 200,

    INTERNAL_SERVER_ERROR = 500, --//Internal Server Error
}

--@brief  构造函数
function WebApp:constructor()
    --名字
    self.name = ""

    --中间件列表
    self.middlewares = {}

    --http server对象
    self.server = nil
end

--@brief 使用中间件
function WebApp:use(middleware)
    table.insert(self.middlewares, middleware)
end

--@brief 服务启动
--@param {string} address  地址
function WebApp:start(address)
    self.server = tinynet.http.server.new()
    local opts = {}
    opts.listen_url = address
    opts.reuseport = true
    local dispather = function(req)
        gevent.pspawn(self.run, self, WebContext.new(req, self))
    end
    local err = self.server:start(opts, dispather)
    if err ~= nil then
        throw(exception.HttpServerStartFailedException, "Http server start failed, err:%s", err)
    end
end

function WebApp:stop()
    if self.server then
        self.server:stop()
    end
end

--@brief 运行中间件
function WebApp:runMiddlewares(ctx)
    local result
    for _, action in pairs(self.middlewares) do
        result = action(ctx)
        if result == false then
            break
        end
    end
end

--@brief 执行请求处理流程
function WebApp:run(ctx)
    if type(jit) == "table" then
        local status, e = try(self.runMiddlewares, self, ctx)()
        if not status then
            if ctx.res.statusCode == StatusCode.OK then
                ctx.res.statusCode = StatusCode.INTERNAL_SERVER_ERROR
            end
            local info = e:ToString()
            ctx.body = info
            log.error(info)
        end
    else
        self:runMiddlewares(ctx)
    end
    ctx:finish()
end

--@brief 发送响应消息
function WebApp:sendResponse(ctx)
    return self.server:send_response(ctx.res)
end

return WebApp
