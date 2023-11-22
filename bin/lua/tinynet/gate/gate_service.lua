--- 网关接入服务
local class = require("tinynet/core/class")
local timer = require("tinynet/core/timer")
local exception = require("tinynet/core/exception")
local SessionService = require("tinynet/service/session_service")
local TaskManager = require("tinynet/gate/task_manager")
local TaskContext = require("tinynet/gate/task_context")

local id_to_string = id_to_string

exception.GateStartFailedException = "GateStartFailedException"

-- 心跳超时
local keepAliveTimeout = 30000

local kMaxPacketSize = 16 * 1024 * 1024 --16M

-- 统计间隔
local kStatInterval = 600000

---@class GateService: Object
local GateService = class("GateService")

--- 构造函数
---@param opts any table
function GateService:constructor(opts)
    -- 服务器id
    self.app_id = opts.app_id

    -- 名字
    self.name = opts.name

    -- 中间件列表
    self.middlewares = {}

    -- 前置处理器
    self.before_hook = nil

    -- 后置处理器
    self.after_hook = nil

    -- tcp server对象
    self.server = nil

    -- 连接数统计计时器
    self.stat_timer = nil

    self.stat_info = {
        totalMessageReceived = 0
    }
end

--  使用中间件
function GateService:use(middleware)
    table.insert(self.middlewares, middleware)
end

function GateService:before(before_hook)
    self.before_hook = before_hook
end

function GateService:after(after_hook)
    self.after_hook = after_hook
end

--  服务启动
-- @param {string} address  地址
function GateService:start(address)
    self.server = tinynet.ws.server.new()
    local opts = {}
    opts.listen_url = address
    opts.keepalive_ms = keepAliveTimeout
    opts.reuseport = true
    opts.max_packet_size = kMaxPacketSize
    local err = self.server:start(opts, function(evt)
        self:dispatch_event(evt)
    end)
    if err ~= nil then
        throw(exception.GateStartFailedException, "Gate server start failed, err:%s", err)
    end
    self.stat_timer = timer.start_repeat(kStatInterval, function()
        self:stat()
    end)
end

function GateService:dispatch_event(evt)
    local sid = id_to_string(evt.guid)
    local session
    if evt.type == "onopen" then
        session = SessionService:create(sid, self.app_id, evt.addr, self.server)
        log.info("[GATE] Session(%s, %s) open.", sid, session.addr)
    elseif evt.type == "onmessage" then
        session = SessionService:get(sid)
        if not session then
            log.error("[GATE] Session(%s, %s) not found", sid, evt.addr)
            return
        end
        TaskManager:push_task(sid, TaskContext.new(self, session, evt.data))
        self.stat_info.totalMessageReceived = self.stat_info.totalMessageReceived + 1
    elseif evt.type == "onclose" then
        TaskManager:close_queue(sid)
        SessionService:remove(sid)
        log.info("[GATE] Session(%s, %s) close.", sid, evt.addr)
    end
end

--  服务结束
function GateService:stop()
    if self.stat_timer then
        timer.stop_repeat(self.stat_timer)
        self.stat_timer = nil
    end
    if self.server then
        self.server:stop()
    end
end

--  统计连接数
function GateService:stat()
    log.info("[GATE] Session stat: sessionCount:%d, totalMessageReceived:%d", self.server:get_session_size(),
        self.stat_info.totalMessageReceived)
end

--  运行中间件
function GateService:runMiddlewares(ctx)
    local result
    for _, action in pairs(self.middlewares) do
        result = action(ctx)
        if result == false then
            break
        end
    end
end

---执行请求处理流程
---@param ctx TaskContext
function GateService:run(ctx)
    if self.before_hook then
        self.before_hook(ctx)
    end
    local status, e = try(self.runMiddlewares, self, ctx)()
    if not status then
        ctx:OnError()
        log.error(e:ToString())
    end
    if self.after_hook then
        self.after_hook(ctx)
    end
end

-- 广播消息
function GateService:broadcast(msg)
    self.server:broadcast_msg(msg)
end

return GateService
