local class = require("tinynet/core/class")
local exception = require("tinynet/core/exception")

exception.ProcessStartFailedException = "ProcessStartFailedException"

---@class Process
local Process = class("Process")

--See See the definition of enum uv_process_flags

Process.FLAGS_NEW_CONSOLE = 1

Process.FLAGS_NO_WINDOW = 2

Process.FLAGS_DETACHED_PROCESS = 4

Process.FLAGS_UNICODE_ENVIRONMENT = 8

function Process:constructor()
    self.process = tinynet.process.new()
    assert(self.process, "Cant not create Process")
    self.process:on_event(function(evt) self:on_event(evt) end)
    --events
    self.onexit = nil
end

function Process:spawn(options)
    local err = self.process:spawn(options)
    if err ~= nil then
        throw(exception.ProcessStartFailedException, "Process start failed, err:%s", err)
    end
end

function Process:kill(signum)
    return self.process:kill(signum)
end

function Process:close()
    return self.process:close()
end

function Process:get_pid()
   return self.process:get_pid() 
end

function Process:on_event(evt)
    local handler = self[evt.type]
    if handler then
        handler(evt)
    end
end

return Process
