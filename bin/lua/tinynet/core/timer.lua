local ctimer = tinynet.timer
local unpack = unpack or table.unpack
local select = select

local timer = {}

function timer.start_timeout(timeout, callback, ...)
    return timer.start_timer(timeout, 0, callback, ...)
end

--- Start a repeat timer
---@param interval number Execution interval in milliseconds
---@param callback function Execution callback
---@param ... any User data
---@return  number timerId
function timer.start_repeat(interval, callback, ...)
    return timer.start_timer(interval, interval, callback, ...)
end

function timer.start_timer(timeout, interval, callback, ...)
    if select("#", ...) == 0 then
        return ctimer.start(timeout, interval, callback)
    end
    local params = {...}
    return ctimer.start(timeout, interval, function() callback(unpack(params)) end)
end

function timer.stop_timeout(guid)
    return timer.stop_timer(guid)
end

function timer.stop_repeat(guid)
    return timer.stop_timer(guid)
end

function timer.stop_timer(guid)
    ctimer.stop(guid)
end

return timer
