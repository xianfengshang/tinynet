local exception = require "tinynet/core/exception"
--Protobuf协议管理
local pb = pb
local string_find = string.find
local string_format = string.format

exception.ProtobufCodecException = "ProtobufCodecException"

local DefaultEncodeOptions = 
{
    encode_as_bytes = true
}

local Protobuf = {}

--@breif 初始化
function Protobuf.Init(opts)
    --清空防止重入
    pb.clear()
    --目录映射
    local typ  = type(opts.protoPath)
    if typ == "table" then
        for k, v in pairs(opts.protoPath) do
            pb.mapping(k, v)
        end
    else
        pb.mapping("", opts.protoPath)
    end

    --加载协议文件
    for _, proto_file in ipairs(opts.protoFiles) do
        pb.import(proto_file)
    end

    Protobuf.decodeProtos = opts.decodeProtos or {}
    Protobuf.encodeProtos = opts.encodeProtos or {}
    Protobuf.namespace = opts.namespace
end

---Encode a table to protobuf binary
---@param route integer
---@param msg table
---@return string
function Protobuf.Encode(route, msg)
    local msg_name = Protobuf.encodeProtos[route]
    if not msg_name then
        return throw(exception.ProtobufCodecException, string.format("Protobuf.Encode Can not found route %s, msg:%s", route, rjson.encode(msg)))
    end
    if not msg then
        return throw(exception.ProtobufCodecException, string.format("Protobuf.Encode Invalid msg body, table expected, route:%s", route))
    end
    if Protobuf.namespace and string_find(msg_name , Protobuf.namespace) ~= 1  then
        msg_name = string_format("%s.%s", Protobuf.namespace, msg_name)
    end
    return pb.encode(msg_name, msg, DefaultEncodeOptions)
end

function Protobuf.Decode(route, data)
    local msg_name = Protobuf.decodeProtos[route]
    if not msg_name then
        return throw(exception.ProtobufCodecException, string.format("Protobuf.Decode Can not found route %s", route))
    end
    if Protobuf.namespace and string_find(msg_name , Protobuf.namespace) ~= 1  then
        msg_name = string_format("%s.%s", Protobuf.namespace, msg_name)
    end
    return pb.decode(msg_name, data)
end

return  Protobuf
