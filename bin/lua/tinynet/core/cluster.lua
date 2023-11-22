local exception = require("tinynet/core/exception")
local c_cluster = tinynet.cluster

exception.ClusterStartFailedException = "ClusterStartFailedException"
exception.ClusterSendFailedException = "ClusterSendFailedException"
exception.ClusterGetKeyException = "ClusterGetKeyException"
exception.ClusterPutKeyException = "ClusterPutKeyException"
exception.ClusterDeleteKeyException = "ClusterDeleteKeyException"
exception.ClusterListKeysException = "ClusterListKeysException"

local cluster = {}

function cluster.start(id, opts, receive_msg_callback)
    local err = c_cluster.start(id, opts, receive_msg_callback)
    if err ~= nil then
        throw(exception.ClusterStartFailedException, "Cluster start failed, err:%s", err)
    end
end

function cluster.stop()
    return c_cluster.stop()
end

local function default_send_callback(err)
end

function cluster.send_message(name, body, callback)
    local yieldable
    if callback == nil then
        local co, b = coroutine.running()
        if co ~= nil and not b then
            yieldable = true
            callback = function (err)
               coroutine.resume(co, err)
            end
        else
            callback = default_send_callback
        end
    end
    c_cluster.send_message(name, body, callback)
    if yieldable then
        local err = coroutine.yield()
        if err ~= nil then
            throw(exception.ClusterSendFailedException, "Cluster send message failed, err:%s", err)
        end
    end
end

local function default_get_callback(res, err)
end

function cluster.get(key, callback)
    local yieldable
    if callback == nil then
        local co, b = coroutine.running()
        if co ~= nil and not b then
            yieldable = true
            callback = function (res, err)
               coroutine.resume(co, res, err)
            end
        else
            callback = default_get_callback
        end
    end
    local err = c_cluster.get(key, callback)
    if yieldable then
        if err ~= nil then
            return throw(exception.ClusterGetKeyException, "Cluster get key failed, key:%s, err:%s", key, err)
        end
        local res, err1 = coroutine.yield()
        if err1 ~= nil then
            return throw(exception.ClusterGetKeyException, "Cluster get key failed, key:%s, err:%s", key, err1)
        end
        return res
    else
        if err ~= nil then
            callback(nil, err)
        end
    end
end

local function default_put_callback(err)
end

function cluster.put(key, value, timeout, callback)
    local yieldable
    if callback == nil then
        local co, b = coroutine.running()
        if co ~= nil and not b then
            yieldable = true
            callback = function (err)
               coroutine.resume(co, err)
            end
        else
            callback = default_put_callback
        end
    end
    local err = c_cluster.put(key, value, timeout, callback)
    if yieldable then
        if err ~= nil then
            return throw(exception.ClusterPutKeyException, "Cluster put key failed, key:%s, value:%s, err:%s", key, value, err)
        end
        err = coroutine.yield()
        if err ~= nil then
            return throw(exception.ClusterPutKeyException, "Cluster put key failed, key:%s, value:%s, err:%s", key, value, err)
        end
    else
        if err ~= nil then
            callback(err)
        end
    end
end

local function default_delete_callback(err)
end

function cluster.delete(key, callback)
    local yieldable
    if callback == nil then
        local co, b = coroutine.running()
        if co ~= nil and not b then
            yieldable = true
            callback = function (err)
               coroutine.resume(co, err)
            end
        else
            callback = default_delete_callback
        end
    end
    local err = c_cluster.delete(key, callback)
    if yieldable then
        if err ~= nil then
            return throw(exception.ClusterDeleteKeyException, "Cluster delete key failed, key:%s, err:%s", key, err)
        end
        err = coroutine.yield()
        if err ~= nil then
            return throw(exception.ClusterDeleteKeyException, "Cluster delete key failed, key:%s, err:%s", key, err)
        end
    else
        if err ~= nil then
            callback(err)
        end
    end
end



local function default_keys_callback(res, err)
end

function cluster.keys(key, callback)
    local yieldable
    if callback == nil then
        local co, b = coroutine.running()
        if co ~= nil and not b then
            yieldable = true
            callback = function (res, err)
               coroutine.resume(co, res, err)
            end
        else
            callback = default_get_callback
        end
    end
    local err = c_cluster.keys(key, callback)
    if yieldable then
        if err ~= nil then
            return throw(exception.ClusterListKeysException, "Cluster list keys failed, key:%s, err:%s", key, err)
        end
        local res, err1 = coroutine.yield()
        if err1 ~= nil then
            return throw(exception.ClusterListKeysException, "Cluster list keys failed, key:%s, err:%s", key, err1)
        end
        return res
    else
        if err ~= nil then
            callback(nil, err)
        end
    end
end

return cluster
