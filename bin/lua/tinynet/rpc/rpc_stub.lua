local class = require("tinynet/core/class")
local cluster = require("tinynet/core/cluster")
local Event = require("tinynet/core/event")
local exception = require("tinynet/core/exception")
local Codec = require("tinynet/cluster/cluster_codec")
local json = rjson
local unpack = unpack or table.unpack

exception.RpcTimeoutException = "RpcTimeoutException"
exception.RpcNotReturnedException = "RpcNotReturnedException"
exception.RpcRouterNotFoundException = "RpcRouterNotFoundException"
exception.RpcRemoteNotFoundException = "RpcRemoteNotFoundException"
exception.RpcRemoteInvalidException = "RpcRemoteInvalidException"

-- rpc请求超时时间
local kRpcTimeout = 30000

---@class RpcStub:Object
local RpcStub = class("RpcStub")

function RpcStub:constructor(opts)
    self.app_id = opts.app_id
    self.routes = opts.routes or {}
    self.services = opts.services or {}
    self.rpc_timeout = opts.rpc_timeout or kRpcTimeout
    self.rpc_context = {}
    self.rpc_seq = 0
end

--- 是否是本地服务
---@param sid string
---@return boolean
function RpcStub:IsLocalService(sid)
    for k, v in ipairs(self.services) do
        if v == sid then
            return true
        end
    end
    return false
end

--- 远程过程调用
---@param msg table
function RpcStub:DoInvoke(msg)
    if not msg.remote or msg.remote == "" then
        return throw(exception.RpcRemoteInvalidException, "Rpc failed, err: Remote name should not empty")
    end
    if self:IsLocalService(msg.remote) then
        local HandlerService = require("tinynet/service/handler_service")
        return HandlerService.InvokeRemote(msg.service, msg.method, unpack(msg.args))
    end
    self.rpc_seq = self.rpc_seq + 1
    msg.seq = self.rpc_seq
    msg.type = "request"
    local event = Event.new()
    self.rpc_context[msg.seq] = event
    cluster.send_message(msg.remote, Codec.encode(msg), function(err)
        if err ~= nil then
            self:HandleError(msg.seq, err)
        end
    end)

    local res, err = event:wait(self.rpc_timeout)
    self.rpc_context[msg.seq] = nil
    if not event:is_set() then -- Rpc timeout, maybe remote server is down
        return throw(exception.RpcTimeoutException, "Rpc failed, err:%s, msg:%s", err, json.encode(msg))
    end
    if res == nil or res.returns == nil then -- Remote server was not returned
        return throw(exception.RpcNotReturnedException, "Rpc failed, err:Remote server was not returned, msg:%s",
            json.encode(msg))
    end
    return unpack(res.returns) -- Remote server was returned
end

function RpcStub:Invoke(remote_id, service, method, ...)
    local msg = {}
    msg.args = {...}
    msg.service = service
    msg.method = method
    msg.remote = remote_id
    msg.source = self.app_id
    return self:DoInvoke(msg)
end

function RpcStub:TypeInvoke(remote_type, service, method, session, ...)
    local msg = {}
    msg.args = {...}
    local router = self.routes[remote_type]
    if router == nil then
        return throw(exception.RpcRouterNotFoundException, "Router not found")
    end
    local remote_id = router(session, msg)
    if remote_id == nil then
        return throw(exception.RpcRemoteNotFoundException, "Remote Not Found")
    end
    msg.service = service
    msg.method = method
    msg.remote = remote_id
    msg.source = self.app_id
    return self:DoInvoke(msg)
end

function RpcStub:GroupInvoke(remote_type, service, method, ...)
    local AppUtil = require("tinynet/util/app_util")
    local app = AppUtil.get_app()
    local servers
    if remote_type == "*" then
        servers = app:GetApps()
    else
        servers = app:GetAppsByName(remote_type)
    end
    for k, v in ipairs(servers) do
        self:Invoke(v, service, method, ...)
    end
end

-- RPC响应
function RpcStub:EndInvoke(msg)
    local event = self.rpc_context[msg.seq]
    if event ~= nil then
        event:set(msg)
    end
end

-- 错误处理
function RpcStub:HandleError(seq, err)
    local event = self.rpc_context[seq]
    if event ~= nil then
        event:set(nil, err)
    end
end

local exports = {}

function exports.Init(opts)
    ---@type RpcStub
    exports.stub = RpcStub.new(opts)
end

--- 指定服务id进行远程调用 
---@param remote_id any
---@param service any
---@param method any
function exports.Invoke(remote_id, service, method, ...)
    return exports.stub:Invoke(remote_id, service, method, ...)
end

--- 对session对应服务类型的服务实例进行远程调用
---@param remote_type any
---@param service any
---@param method any
---@param session any
function exports.TypeInvoke(remote_type, service, method, session, ...)
    return exports.stub:TypeInvoke(remote_type, service, method, session, ...)
end

---按服务类型进行远程调用
---@param remote_type any
---@param service string
---@param method string
---@param ... any
function exports.GroupInvoke(remote_type, service, method, ...)
    return exports.stub:GroupInvoke(remote_type, service, method, ...)
end

--- 处理RPC应答
---@param msg any
function exports.EndInvoke(msg)
    return exports.stub:EndInvoke(msg)
end

return exports
