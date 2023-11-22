local exception = require("tinynet/core/exception")
local ctimer = tinynet.timer
local unpack = unpack or table.unpack

-- Enable or disable coroutine pool
local coroutine_pool_enabled = false

-- Max coroutine pool size
local coroutine_pool_size = 1024

local gevent = {
    coroutine_pool = {}
}

local function coroutine_should_reclaim()
    return #gevent.coroutine_pool < coroutine_pool_size
end

local function coroutine_spawn()
    local co = coroutine.create(function(f)
        f(coroutine.yield())
        while coroutine_should_reclaim() do
            table.insert(gevent.coroutine_pool, coroutine.running())
            f = coroutine.yield()
            f(coroutine.yield())
        end
    end)
    return co
end

local function coroutine_create(f)
    local co = table.remove(gevent.coroutine_pool)
    if not co then
        co = coroutine_spawn()
    end
    coroutine.resume(co, f)
    return co
end

local co_resume = coroutine.resume
local co_yield = coroutine.yield
local co_create
if coroutine_pool_enabled then
    co_create = coroutine_create
else
    co_create = coroutine.create
end

local next_tick_patch = function(callback)
    ctimer.start(0, 0, callback)
end

local next_tick = next_tick or next_tick_patch

function gevent.spawn(f, ...)
    local co = co_create(f)
    local params = {...}
    next_tick(function()
        co_resume(co, unpack(params))
    end)
    return co
end

function gevent.spawnlater(delay, f, ...)
    local co = co_create(f)
    local params = {...}
    ctimer.start(delay, 0, function()
        co_resume(co, unpack(params))
    end)
    return co
end

function gevent.pspawn(f, ...)
    local co = co_create(function(...)
        try(f, ...).catch(function(e)
            log.error(tostring(e))
        end)()
    end)
    local params = {...}
    next_tick(function()
        co_resume(co, unpack(params))
    end)
    return co
end

function gevent.pspawnlater(delay, f, ...)
    local co = co_create(function(...)
        try(f, ...).catch(function(e)
            log.error(tostring(e))
        end)()
    end)
    local params = {...}
    ctimer.start(delay, 0, function()
        co_resume(co, unpack(params))
    end)
    return co
end

function gevent.sleep(milliseconds)
    local co, b = coroutine.running()
    if not co or b then
        return sleep(milliseconds / 1000)
    end
    ctimer.start(milliseconds, 0, function()
        co_resume(co)
    end)
    return co_yield()
end

function gevent.xpspawn(f, errfun, ...)
    local co = co_create(function(...)
        try(f, ...).catch(errfun)()
    end)
    local params = {...}
    next_tick(function()
        co_resume(co, unpack(params))
    end)
    return co
end

return gevent
