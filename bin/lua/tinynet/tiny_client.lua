local class = require("tinynet/core/class")
local Message = require("tinynet/message")
local WebSocket = require("tinynet/core/web_socket")
local EventEmitter = require("tinynet/event_emitter")
local gevent = require("tinynet/core/gevent")

---@class TinyClient:Object
local TinyClient = class("TinyClient")

function  TinyClient:constructor(url)
    --服务器地址
    self.server_url = url
    --请求ID
    self.req_id = 0
    --推送ID
    self.push_id = 0
    --WebSocket连接
    self.websocket = nil

    ---@type EventEmitter
    self.event_emitter = EventEmitter.new()

    self.send_queue = {}

    self.recv_queue = {}

    --连接成功回调
    self.connected = nil
    --断开连接回调
    self.disconnected = nil
end

function TinyClient:Open()
    if self.websocket then
        self.websocket:close()
    end
    local opts = {}
    opts.url = self.server_url
    opts.keepalive_timeout = 30000
    opts.ping_interval = 5000
    self.websocket = WebSocket.new(opts)
    self.websocket.onopen = function (evt) self:WebSocket_Open(evt) end
    self.websocket.onclose = function (evt) self:WebSocket_Close(evt) end
    self.websocket.onmessage = function (evt) self:WebSocket_Message(evt) end
    self.websocket.onerror = function (evt) self:WebSocket_Error(evt) end
    self.websocket:open()
end

---发送请求
function TinyClient:Request(route, msg, callback)
    self.req_id = self.req_id + 1
    local req = Message.new()
    req.seq = self.req_id
    req.type = Message.TYPE_REQUEST
    req.route = route
    req.body = msg
    self.event_emitter:RegisterCallback(req.seq, callback)
    table.insert(self.send_queue, req)
    self:Send(req)
    return req.seq
end

---发送通知
function TinyClient:Notify(route, msg)
    local req = Message.new()
    req.seq = 0
    req.type = Message.TYPE_NOTIFY
    req.route = route
    req.body = msg
    self:Send(req)
    return req.seq
end

function TinyClient:On(route, callback)
    self.event_emitter:AddListener(route, callback)
end

--- WebSocket连接打开事件
function TinyClient:WebSocket_Open(evt)
    if self.connected then
        self.connected()
    end
end

---WebSocket连接关闭事件
function TinyClient:WebSocket_Close(evt)
    if self.disconnected then
        self.disconnected()
    end
end

---WebSocket连接错误事件
function TinyClient:WebSocket_Error(evt)
end

---WebSocket接收消息事件
function TinyClient:WebSocket_Message(evt)
    local data = evt.data
    if not data then
        return
    end
    local msg = Message.new(evt.data)
    if msg.type == Message.TYPE_REQUEST then
        log.error("Client should never receive request message!")
    elseif msg.type == Message.TYPE_RESPONSE then
        self:HandleResponse(msg)
    elseif msg.type == Message.TYPE_PUSH then
        self:HandlePush(msg)
    elseif msg.type == Message.TYPE_NOTIFY then
        self:HandleNotify(msg)
    end
end

function TinyClient:HandleResponse(msg)
    local req = table.remove( self.send_queue, 1)
    if req and req.seq ~= msg.seq then
        log.error("Response message sequence error!")
    end
    gevent.pspawn(self.event_emitter.InvokeCallback, self.event_emitter, msg.seq, msg.body)
end

function TinyClient:HandlePush(msg)
    self.push_id = msg.seq
    gevent.pspawn(self.event_emitter.InvokeListener, self.event_emitter, msg.route, msg)
end

function TinyClient:HandleNotify(msg)
    gevent.pspawn(self.event_emitter.InvokeListener, self.event_emitter, msg.route, msg)
end

---发送消息接口
function TinyClient:Send(msg)
    if self.websocket and self.websocket:is_connected() then
        local data = msg:SerializeToString()
        self.websocket:send_bytes(data)
    end
end

return TinyClient
