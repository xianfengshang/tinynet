local exception = require "tinynet/core/exception"
exception.HttpRequestErrorException = "HttpRequestErrorException"

local CURLE_FAIL = 100

local http_client = tinynet.http.client
local http =
{
    client = {}
}

local function default_callback(...)
end

function http.client.request(config, callback)
    local yieldable
    if callback == nil then
        local co, b = coroutine.running()
        if co ~= nil and not b then
            yieldable = true
            callback = function(...)
                coroutine.resume(co, ...)
            end
        else
            callback = default_callback
        end
    end
    local guid, err = http_client.request(config, callback)
    if yieldable then
        if err ~= nil then
            return throw(exception.HttpRequestErrorException, err)
        else
            return coroutine.yield()
        end
    else
        if err ~= nil then
            return callback({code = CURLE_FAIL, msg = err})
        end
    end
end

function http.client.get(url, params, callback)
    local config = {}
    config.method = "GET"
    config.url = url
    config.params = params
    return http.client.request(config, callback)
end

function http.client.post(url, data, callback)
    local config = {}
    config.method = "POST"
    config.url = url
    if type(data) == "table" then
        config.data = rjson.encode(data)
        config.headers = {}
        config.headers["Content-Type"] = "application/json"
    else
        config.data = data
    end
    return http.client.request(config, callback)
end

return http
