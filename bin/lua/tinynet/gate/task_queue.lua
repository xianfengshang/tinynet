local class = require("tinynet/core/class")
local gevent = require("tinynet/core/gevent")
local timer = require("tinynet/core/timer")
local exception = require("tinynet/core/exception")
local table_insert = table.insert

local STATE_IDLE = 1

local STATE_BUSY = 2

local STATE_CLOSE = 3

local DEFAULT_TIMEOUT = 30000

local TaskQueue = class("TaskQueue")

function TaskQueue:constructor(id, timeout)
    self.id = id
    self.queue = {}
    self.state = STATE_IDLE
    self.timeout = timeout or DEFAULT_TIMEOUT
    self.timerId = nil
end

function TaskQueue:push(task)
    if self.state == STATE_CLOSE then
        return
    end
    table_insert(self.queue, task)
    if self.state == STATE_IDLE then
        self.state = STATE_BUSY
        gevent.spawn( self.process, self)
    end
end

function TaskQueue:close()
    self.state = STATE_CLOSE
end

function TaskQueue:process_safe_mode()
    while true do
        local task = table.remove(self.queue, 1)
        if not task then
            break
        end
        try(task.run, task).catch(function (e)
            log.error(e:ToString())
        end)()
    end
    self.state = STATE_IDLE
end

function TaskQueue:process_timeout_mode()
    while true do
        if self.timerId then
            break
        end
        local task = table.remove(self.queue, 1)
        if not task then
            break
        end
        self.timerId = timer.start_timeout(task.timeout or self.timeout, function() self.state = STATE_IDLE end)
        task:run()
        timer.stop_timeout(self.timerId)
        self.timerId = nil
        if self.state == STATE_IDLE then
            self.state = STATE_BUSY
        end
    end
    self.state = STATE_IDLE
end

function TaskQueue:process()
    return self:process_safe_mode()
end

return TaskQueue
