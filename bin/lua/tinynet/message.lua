local class = require("tinynet/core/class")
local string_format = string.format
local rjson = rjson
local pb = pb

local DefaultEncodeOptions = 
{
    encode_as_bytes = true
}

---@class Message: Object
local Message = class("Message")

Message.CONTENT_TYPE_PB = 0

Message.CONTENT_TYPE_JSON = 1

Message.TYPE_REQUEST = 0

Message.TYPE_NOTIFY = 1

Message.TYPE_RESPONSE = 2

Message.TYPE_PUSH = 3

Message.TYPE_HANDSHAKE = 4

Message.TYPE_HANDSHAKE_RESPONSE = 5

Message.TypeMeta =
{
    [Message.TYPE_REQUEST] = { oneway = false, ack = Message.TYPE_RESPONSE},
    [Message.TYPE_NOTIFY] = { oneway = true},
    [Message.TYPE_RESPONSE] = { oneway = true},
    [Message.TYPE_PUSH] = { oneway = true},
    [Message.TYPE_HANDSHAKE] = { oneway = false, ack = Message.TYPE_HANDSHAKE_RESPONSE}
}

--- 构造函数
function Message:constructor(data)
    self.contentType = Message.CONTENT_TYPE_PB
    if data then
        self:init(data)
    else
        self:initDefault()
    end
end

function Message:init(data)
    self:ParseFromString(data)
end

function Message:initDefault()
    self.seq = 0        --消息序列号
    self.type = 0       --消息类型
    self.route = 0      --消息路由
    self.body = nil     --消息体
    self.bodySize = -1  --消息体长度
    self.byteSize = -1  --消息包长度
end

function Message:ParseFromString(str)
    if self.contentType == Message.CONTENT_TYPE_JSON then
        return self:ParseFromJson(str)
    else
        return self:ParseFromPB(str)
    end
end

--- 从字符串解析
---@param str string  二进制字符串
---@return boolean 是否成功
function Message:ParseFromPB(str)
    local Protobuf = require("tinynet/protobuf")
    local szPacket = string_format("%s.Packet", Protobuf.namespace)
    local packet = pb.decode(szPacket, str)
    if not packet then
        return false
    end
    self.seq    = packet.seq
    self.type   = packet.type
    self.route  = packet.route

    self.body = Protobuf.Decode(self.route, packet.body)
    if not self.body then
        return false
    end
    self.bodySize = #packet.body
    self.byteSize = #str
    return true
end

function Message:SerializeToString()
    if self.contentType == Message.CONTENT_TYPE_JSON then
        return self:SerializeToJson()
    else
        return self:SerializeToPB()
    end
end
--- 序列为二进制字符串
---@return string 序列化后的二进制字符串
function Message:SerializeToPB()
    local Protobuf = require("tinynet/protobuf")
    local szPacket = string_format("%s.Packet", Protobuf.namespace)
    local data = Protobuf.Encode(self.route, self.body)
    self.bodySize = #data
    local msg = {}
    msg.seq     = self.seq
    msg.type    = self.type
    msg.route   = self.route
    msg.body    = data
    local bin_data = pb.encode(szPacket, msg, DefaultEncodeOptions)
    self.byteSize = #bin_data
    return  bin_data
end

--- 调试字符串
---@return string JSON字符串
function Message:DebugString()
    return self:SerializeToJson()
end

--- 序列化为JSON
function Message:SerializeToJson()
    return rjson.encode(self:PlainObject())
end

function Message:PlainObject()
    --plain object
    local o = {}
    o.seq       = self.seq
    o.type      = self.type
    o.route     = self.route
    o.body      = self.body
    o.name      = self:GetName()
    return o
end

-- 获取消息名
function Message:GetName()
    local Protobuf = require("tinynet/protobuf")
    if self.type == Message.TYPE_REQUEST then
        return Protobuf.decodeProtos[self.route]
    elseif self.type == Message.TYPE_NOTIFY then
        return Protobuf.decodeProtos[self.route]
    elseif self.type == Message.TYPE_RESPONSE then
        return Protobuf.encodeProtos[self.route]
    elseif self.type == Message.TYPE_PUSH then
        return Protobuf.encodeProtos[self.route]
    elseif self.type == Message.TYPE_HANDSHAKE then
        return Protobuf.decodeProtos[self.route]
    elseif self.type == Message.TYPE_HANDSHAKE_RESPONSE then
        return Protobuf.encodeProtos[self.route]
    end
    return ""
end

---从json解析
function Message:ParseFromJson(str)
    local o = rjson.decode(str)
    self.seq    = o.seq
    self.type   = o.type
    self.route  = o.route
    self.body   = o.body
    return true
end

function Message:ByteSize()
    if self.byteSize == -1 then
        self:SerializeToString()
    end
    return self.byteSize
end

function Message:BodySize()
    if self.bodySize == -1 then
        self:SerializeToString()
    end 
    return self.bodySize
end

return Message
