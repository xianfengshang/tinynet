local class = require("tinynet/core/class")
local exception = require("tinynet/core/exception")
local time = time
local table_insert = table.insert
local string_format = string.format
local json = rjson 

---@class Redis:Object
local Redis = class("Redis")

function Redis:constructor(options)
    if type(options) ~= "table" then
        error(string_format("bad argument #1 to 'Redis:constructor' (table expected, got %s)", type(options)), 3)
    end
    self.options = {}
    self.options.host = options.host or "127.0.0.1"
    self.options.port = options.port or 6379
    self.options.path = options.path
    self.options.password = options.password
    self.options.logCommand = to_boolean(options.logCommand)
    self.options.logFile = options.logFile or "./log/redis/redis"
    self.driver = tinynet.redis.new(self.options)
    assert(self.driver, "Cant not create redis client")
end

local function callback_wrapper(begin, opts, cmd, callback)
    return function(res, err)
        if opts.logCommand then
            local now = time()
            local cmd_info = {}
            cmd_info.host = opts.host
            cmd_info.cmd = cmd
            cmd_info.status = err or "OK"
            cmd_info.res = res
            cmd_info.RT = string_format("%.3f ms", (now - begin) * 1000)
            cmd_info.time = now
            log.write_file_log(opts.logFile, string_format("%s\n", json.encode(cmd_info)))
        end
        return callback(res, err)
    end
end

local function default_callback(...)
end

function Redis:command(cmd, callback)
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
    local err = self.driver:command(cmd, callback_wrapper(time(), self.options, cmd, callback))
    if yieldable then
        if err ~= nil then
            return throw(exception.RedisException, err)
        else
            local res, err1 = coroutine.yield()
            if err1 ~= nil then
                return throw(exception.RedisException, err1)
            else
                return res
            end
        end
    else
        if err ~= nil then
            return callback(nil, err)
        end
    end
end

local function parse_value(str)
    if tonumber(str) then
        return tonumber(str)
    end
    local it = string.gmatch(str, '([%w,_]+):([^\n]+)')
    if not it then
        return str
    end
    local value = {}
    for k, v in it do
        value[k] = parse_value(v)
    end
    return value
end

local function parse_info(str)
    local info = {}
    for k, v in string.gmatch(str, '([%w,_]+):([^\n]+)') do
        info[k] = parse_value(v)
    end
    return info
end

local function info_wrapper(callback)
    return function(res, err)
        if err == nil then
            res = parse_info(res)
        end
        return callback(res, err)
    end
end

function Redis:info(callback)
    local cmd = 'info'
    if callback then
        return self:command(cmd, info_wrapper(callback))
    end
    local res = self:command(cmd)
    return parse_info(res)
end

function Redis:get(key, callback)
    local cmd = {"GET", key}
    return self:command(cmd, callback)
end

function Redis:set(key, value, timeout, exist, callback)
    local cmd = {}
    table_insert(cmd, "SET")
    table_insert(cmd, key)
    table_insert(cmd, value)
    if type(timeout) == "number" then
        table_insert(cmd, "PX")
        table_insert(cmd, timeout)
    end
    if exist == true then
        table_insert(cmd, "XX")
    elseif exist == false then
        table_insert(cmd, "NX")
    end
    return self:command(cmd, callback)
end

function Redis:del(key, callback)
    local cmd = {"DEL", key}
    return self:command(cmd, callback)
end

function Redis:hset(key, field, value, callback)
    local cmd = {"HSET", key, field, value}
    return self:command(cmd, callback)
end

function Redis:hdel(key, field, callback)
    local cmd = {"HDEL", key, field}
    return self:command(cmd, callback)
end

function Redis:hmset(key, value, callback)
    local cmd = {"HMSET", key}
    for k, v in pairs(value) do
        table_insert(cmd, k)
        table_insert(cmd, v)
    end
    return self:command(cmd, callback)
end

local function parse_hash(data)
    if type(data) ~= "table" then
        return nil
    end
    local result = {}
    for i = 1, #data, 2 do
        result[data[i]] = data[i+1]
    end
    return result
end

local function hash_wrapper(callback)
    return function(res, err)
        if err ~= nil then
            res = parse_hash(res)
        end
        return callback(res, err)
    end
end

function Redis:hgetall(key, callback)
    local cmd = {"HGETALL", key}
    if callback ~= nil then
        return self:command(cmd, hash_wrapper(callback))
    end
    local res = self:command(cmd, callback)
    return parse_hash(res)
end

function Redis:incr(key, callback)
    local cmd = {"INCR", key}
    return self:command(cmd, callback)
end

function Redis:rpush(key, value, callback)
    local cmd = {"RPUSH", key, value}
    return self:command(cmd, callback)
end

function Redis:lrange(key, start, stop, callback)
    local cmd = {"LRANGE", key, start, stop}
    return self:command(cmd, callback)
end

function Redis:pexpire(key, duration, callback)
    local cmd = {"PEXPIRE", key, duration}
    return self:command(cmd, callback)
end

function Redis:scan(cursor, pattern, count, callback)
    local cmd = {}
    table_insert(cmd, "SCAN")
    table_insert(cmd, cursor)
    if pattern then
        table_insert(cmd, "MATCH")
        table_insert(cmd, pattern)
    end
    if count then
        table_insert(cmd, "COUNT")
        table_insert(cmd, count)
    end
    return self:command(cmd, callback)
end

function Redis:zadd(key, score, member, callback)
    local cmd = {}
    table_insert(cmd, "ZADD")
    table_insert(cmd, key)
    table_insert(cmd, score)
    table_insert(cmd, member)
    return self:command(cmd, callback)
end

function Redis:zincrby(key, score, member, callback)
    local cmd = {}
    table_insert(cmd, "ZINCRBY")
    table_insert(cmd, key)
    table_insert(cmd, score)
    table_insert(cmd, member)
    return self:command(cmd, callback)
end

function Redis:zrem(key, member, callback)
    local cmd = {}
    table_insert(cmd, "ZREM")
    table_insert(cmd, key)
    table_insert(cmd, member)
    return self:command(cmd, callback)
end

function Redis:zremrangebyrank(key, start, stop, callback)
    local cmd = {"ZREMRANGEBYSCORE", key, start, stop}
    return self:command(cmd, callback)
end

function Redis:zremrangebyrank(key, start, stop, callback)
    local cmd = {"ZREMRANGEBYRANK", key, start, stop}
    return self:command(cmd, callback)
end

function Redis:zcard(key, callback)
    local cmd = {}
    table_insert(cmd, "ZCARD")
    table_insert(cmd, key)
    return self:command(cmd, callback)
end

function Redis:zrevrank(key, member, callback)
    local cmd = {}
    table_insert(cmd, "ZREVRANK")
    table_insert(cmd, key)
    table_insert(cmd, member)
    return self:command(cmd, callback)
end

function Redis:zscore(key, member, callback)
    local cmd = {}
    table_insert(cmd, "ZSCORE")
    table_insert(cmd, key)
    table_insert(cmd, member)
    return self:command(cmd, callback)
end

local function parse_zset_withscores(data)
    if type(data) ~= "table" then
        log.warning("data=%s", rjson.encode(data))
        return nil
    end
    local members = {}
    local scores = {}
    for i = 1, #data, 2 do
        table_insert(members, data[i])
        table_insert(scores, data[i+1])
    end
    return members, scores
end

local function zset_withscores_wrapper(callback)
    return function(res, err)
        if err ~= nil then
            local result = {}
            result.members, result.scores = parse_zset_withscores(res)
            res = result
        end
        return callback(res, err)
    end
end

function Redis:zrange(key, start, stop, withscores, callback)
    local cmd = {}
    table_insert(cmd, "ZRANGE")
    table_insert(cmd, key)
    table_insert(cmd, start)
    table_insert(cmd, stop)
    if withscores then
        table_insert(cmd, "WITHSCORES")
    end
    if callback then
        if withscores then
           return self:command(cmd, zset_withscores_wrapper(callback))
        else
            return self:command(cmd, callback)
        end
    end
    local res = self:command(cmd, callback)
    if not withscores then
        return res
    end
    return parse_zset_withscores(res)
end

function Redis:zrevrange(key, start, stop, withscores, callback)
    local cmd = {}
    table_insert(cmd, "ZREVRANGE")
    table_insert(cmd, key)
    table_insert(cmd, start)
    table_insert(cmd, stop)
    if withscores then
        table_insert(cmd, "WITHSCORES")
    end
    if callback then
        if withscores then
           return self:command(cmd, zset_withscores_wrapper(callback))
        else
            return self:command(cmd, callback)
        end
    end
    local res = self:command(cmd, callback)
    if not withscores then
        return res
    end
    return parse_zset_withscores(res)
end

function Redis:zcount(key, min, max, callback)
    local cmd = {} 
    table_insert(cmd, "ZCOUNT")
    table_insert(cmd, key)
    table_insert(cmd, min or "-inf")
    table_insert(cmd, max or "+inf")
    return self:command(cmd, callback)
end

function Redis:hsetnx(key, field, value, callback)
    local cmd = {}
    table_insert(cmd, "HSETNX")
    table_insert(cmd, key)
    table_insert(cmd, field)
    table_insert(cmd, value)
    return self:command(cmd, callback)
end

function Redis:hget(key, field, callback)
    local cmd = {}
    table_insert(cmd, "HGET")
    table_insert(cmd, key)
    table_insert(cmd, field)
    return self:command(cmd, callback)
end

function Redis:exists(key, callback)
    local cmd = {}
    table_insert(cmd, "EXISTS")
    table_insert(cmd, key)
    return self:command(cmd, callback)
end

function Redis:monitor(callback)
    local err = self.driver:monitor(callback)
    if err ~= nil then
        throw(exception.RedisException, err)
    end
end

function Redis:subscribe(name, callback)
    local err = self.driver:subscribe(name, callback)
    if err ~= nil then
        throw(exception.RedisException, err)
    end
end

function Redis:unsubscribe(name)
    local err = self.driver:unsubscribe(name)
    if err ~= nil then
        throw(exception.RedisException, err)
    end
end

function Redis:psubscribe(pattern, callback)
    local err = self.driver:psubscribe(pattern, callback)
    if err ~= nil then
        throw(exception.RedisException, err)
    end
end

function Redis:punsubscribe(pattern)
    local err = self.driver:punsubscribe(pattern)
    if err ~= nil then
        throw(exception.RedisException, err)
    end
end

return Redis
