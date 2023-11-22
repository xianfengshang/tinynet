local DateUtil = require("tinynet/util/date_util")
local debug = debug
local table_insert = table.insert
local exists = fs.exists
local string_char = string.char
local string_byte = string.byte
local string_match = string.match
local string_format = string.format
local string_gsub = string.gsub
local string_lower = string.lower
local tonumber = tonumber
local json = rjson

local StatusCode =
{
    NO_CONTENT = 204,

    NOT_FOUND = 404
}

local DEFAULT_CODEC_OPTIONS = {
    empty_table_as_array = true
}


local exports = {}

--URL decode
local function url_decode(s)
    return string_gsub(s, '%%(%x%x)', function(h) return string_char(tonumber(h, 16)) end)
end

--key/value pair decoder
local function kv_decode(s)
    local results = {}
    local key_value_pairs = string.split(s, '&')
    local key_value_pair
    local key, value
    local array_fields = {}
    for k, v in ipairs(key_value_pairs) do
        key_value_pair = string.split(v, '=')
        if #key_value_pair >= 2 then
            key = key_value_pair[1]
            value = key_value_pair[2]
            if results[key] then
                if not array_fields[key] then
                    array_fields[key] = {results[key]}
                end
                table_insert(array_fields[key], value)
            else
                results[key] = value
            end
        end
    end
    for k, v in pairs(array_fields) do
        results[k] = v
    end
    return results
end

local function parse_accept_encoding(str)
    if not str then
        return {}
    end
    return string.split(str, "%s*,%s*")
end

--处理跨域问题
exports.cors_handler = function(ctx)
    ctx.res.headers["Server"] = "tinynet/core/1.0.0"
    ctx.res.headers["Allow"] = "POST,OPTIONS,GET,HEAD,TRACE"
    ctx.res.headers["Date"] = DateUtil.nowdate()
    if ctx.req.headers["origin"] then
        --跨域请求
        ctx.res.headers["Access-Control-Allow-Origin"] = "*"
        ctx.res.headers["Access-Control-Allow-Methods"] = "POST,GET,OPTIONS,PUT,DELETE"
        ctx.res.headers["Access-Control-Allow-Headers"] = "X-Requested-With,Content-Type,Authorization"
    end
    local method = string_lower( ctx.req.method )
    --handle preflight request
    if method == "options" then
        ctx.res.headers["Connection"] = "Keep-Alive"
        ctx.res.statusCode = StatusCode.NO_CONTENT
        log.info("%s %s %s 0 ms", method, ctx.req.url, ctx.res.statusCode)
        return false
    end
end

--解析get参数
exports.query_parser = function(ctx)
    --复制一份path
    ctx.req.raw_path = ctx.req.path

    --复制一份原始参数
    ctx.req.raw_query = ctx.req.query
    if ctx.req.query == "" then
        return
    end
    ctx.req.query = kv_decode(ctx.req.query)
end

exports.body_parser =  function(ctx)
    --复制一份原始body
    ctx.req.raw_body = ctx.req.body
    local contentType = ctx.req.headers["content-type"]
    if not contentType then
        return
    end
    if string.startswith(contentType, "application/json") then
        local ok, body = pcall(json.decode, ctx.req.body)
        if ok then
            ctx.req.body = body
            return
        end
        body = url_decode(ctx.req.body)
        ctx.req.body = json.decode(body)
        return
    end
    if string.startswith(contentType, "application/x%-www%-form%-urlencoded") then
        ctx.req.body = kv_decode(ctx.req.body)
        return
    end
    return true
end

exports.body_writer = function(ctx)
    local type_string = type(ctx.res.body)
    if type_string == "table" then
        ctx.res.headers["Content-Type"] = "application/json;charset=utf-8"
        ctx.res.body = json.encode(ctx.res.body, DEFAULT_CODEC_OPTIONS)
    elseif type_string == "string" then
        if not ctx.res.headers["Content-Type"] then
            ctx.res.headers["Content-Type"] = "text/plain"
        end
    elseif type_string == "nil" then
        ctx.res.headers["Content-Type"] = "application/json"
        ctx.res.body = "null"
    else
        ctx.res.headers["Content-Type"] = "text/plain"
        ctx.res.body = tostring(ctx.res.body)
    end
    return true
end

exports.logger = function (ctx)
    local diff = high_resolution_time() - ctx.createTime
    log.info("%s %s %s %.3f ms", ctx.req.method, ctx.req.url, ctx.res.statusCode, diff * 1000)
    return true
end

local mime = require_json("config/mime.json")

exports.send_file = function (ctx, path)
    path = path or ctx.req.path
    local ch = string_char(string_byte(path, 1))
    if ch == '/' or ch == '\\' then
        path = string_format(".%s", path)
    end
    if not exists(path) then
        ctx.res.statusCode = StatusCode.NOT_FOUND
        return
    end
    local ext = string_match(path, ".+(%.%w+)$") or ""
    ctx.res.headers["Content-Type"] = mime[ext] or "text/plain"
    local data = fs.readfile(path)
    if not data then
        ctx.res.statusCode = StatusCode.NOT_FOUND
        return
    end
    local accept_encoding = parse_accept_encoding(ctx.req.headers["accept-encoding"])
    if not table.find(accept_encoding, "gzip") then
        ctx.res.body = data
        return
    end
    local compressed_data, err = zlib.deflate(data)
    if compressed_data and #compressed_data < #data then
        ctx.res.headers["Content-Encoding"] = "gzip"
        ctx.res.body = compressed_data
    else
        ctx.res.body = data
    end
end

return exports
