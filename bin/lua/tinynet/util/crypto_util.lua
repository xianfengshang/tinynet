local json = rjson
local md5_sum = md5_sum

local exports = {}

--@brief 根据设备ID和ip地址创建token
function exports.CreateToken(deviceId, ip)
    deviceId = deviceId or ""
    ip = ip or ""
    local today = os.date("*t", os.time())
    today.hour = 0
    today.min = 0
    today.sec = 0
    local todayBegin =  os.time(today)
    local data = string.format("%s&%s&%s", deviceId, ip, todayBegin)
    return md5_sum(data)
end

local json_codec_options = { order_map_entries = true }

--Note obj must be a plain object
function exports.hash_object(obj)
    return md5_sum(json.encode(obj, json_codec_options))
end

function exports.hash_string(str)
    return md5_sum(str)
end

return exports
