local class = require("tinynet/core/class")
local date_util = require("tinynet/util/date_util")

---@class TimedLock: Object
local TimedLock = class("TimedLock")

function TimedLock:constructor(timeout)
    self.timeout = timeout
    self.lock_time = 0
end

function TimedLock:lock()
    self.lock_time = date_util.time() + self.timeout
end

function TimedLock:try_lock()
    if self:is_lock() then
        return false
    end
    self:lock()
    return true
end

function TimedLock:is_lock()
    return self.lock_time > 0 and self.lock_time > date_util.time()
end

function TimedLock:unlock()
    self.lock_time = 0
end

return TimedLock