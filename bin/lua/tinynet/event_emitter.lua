local class = require("tinynet/core/class")
local gevent = require("tinynet/core/gevent")

---@class EventEmitter:Object
local EventEmitter = class("EventEmitter")

function EventEmitter:constructor()
    self.callbacks = {}
    self.listeners = {}
end

function EventEmitter:RegisterCallback(id, callback)
    self.callbacks[id] = callback
end

function EventEmitter:InvokeCallback(id, msg)
    local callback = self.callbacks[id]
    if not callback then
        return
    end
    self.callbacks[id] = nil
    callback(msg)
end

function EventEmitter:AddListener(evt, listener)
    local listeners = self.listeners[evt]
    if not listeners then
        listeners = {}
        self.listeners[evt] = listeners
    end
    table.insert(listeners, listener)
end

function EventEmitter:RemoveListener(evt, listener)
    local listeners = self.listeners[evt]
    if not listeners then
        return
    end
    table.remove(listeners, table.find(listeners, listener))
end

function EventEmitter:InvokeListener(evt, msg)
    local listeners = self.listeners[evt]
    if not listeners then
        return
    end
    for _, listener in pairs(listeners) do
        listener(msg)
    end
end

function EventEmitter:RemoveAllListener(evt)
    self.listeners[evt] = nil
end

return EventEmitter
