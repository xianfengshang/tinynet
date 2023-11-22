local class = require("tinynet/core/class")
local ctimer = tinynet.timer

---@class tinynet.Event:Object
local Event = class("Event")

local EVENT_STATE_INIT = 0

local EVENT_STATE_WAIT = 1

local EVENT_STATE_SET = 2

function Event:constructor()
    self.state = EVENT_STATE_INIT
    self.wait_id = nil
    self.wait_co = nil
end

function Event:set(...)
    if self.state ~= EVENT_STATE_WAIT then
        return
    end
    --1+ change state
    self.state = EVENT_STATE_SET

    --2+ stop timer immediately
    local id = self.wait_id
    self.wait_id = nil
    ctimer.stop(id)

    --3+ wake up from waiting
    local co = self.wait_co
    self.wait_co = nil
    coroutine.resume(co, ...)
end

function Event:is_set()
    return self.state == EVENT_STATE_SET
end

function Event:reset()
    local state = self.state
    self.state = EVENT_STATE_INIT
    if state == EVENT_STATE_WAIT then
        local id = self.wait_id
        self.wait_id = nil
        ctimer.stop(id)

        local co = self.wait_co
        self.wait_co = nil
        coroutine.resume(co, nil, "Interrupted")
    end
end

function Event:wait(timeout)
    --assert(self.state ~= EVENT_STATE_WAIT, "Event has already been waiting by other thread!")
    self.state = EVENT_STATE_WAIT
    local co, b = coroutine.running()
    --assert(co and not b, "Event can not be waiting in main thread!")
    self.wait_co = co
    self.wait_id = ctimer.start(timeout, 0, function() coroutine.resume(co, nil, "Timeout") end)
    return coroutine.yield()
end

function Event:is_wait()
    return self.state == EVENT_STATE_WAIT
end

return Event
