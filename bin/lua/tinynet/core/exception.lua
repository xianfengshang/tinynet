---@diagnostic disable: lowercase-global
local class = require("tinynet/core/class")
local json = rjson
local unpack = unpack or table.unpack

---@class Exception:Object
local Exception = class("Exception")

Exception.__tostring = function(self)
    return self:ToString()
end

function Exception:constructor()
    self.name = ""
    self.message = ""
    self.stacktrace = ""
    self.file = ""
    self.line = ""
end

function Exception:ToString()
    local str
    if not self.file then
        str = string.format("%s: %s\n%s", self.name, self.message, self.stacktrace)
    else
        str = string.format("%s:%s: %s: %s\n%s", self.file, self.line, self.name, self.message, self.stacktrace)
    end
    return str
end

function Exception:ParseFromString(info)
    local obj, err1 = pcall(json.decode, info)
    if err1 == nil then
        self.name = obj.name
        self.message = obj.message
        if obj.stacktrace then
            self.file = obj.file
            self.line = obj.line
            self.stacktrace = obj.stacktrace
        end
    else
        self.name = "ErrorException"
        self.message = info
    end
end

function Exception:catcher()
    return function(err)
        local file, line, info = string.match(err, "([^%{%}]*):(%d+):%s?(.*)")
        if info ~= nil then
            self:ParseFromString(info)
        else
            if string.match(err, "^{.*}$") then
                self:ParseFromString(err)
            else
                self.name = "ErrorException"
                self.message = err
            end
        end
        if self.stacktrace == "" then
            self.file, self.line = file, line
            self.stacktrace = debug.traceback("", 2)
        end
    end
end

---@class ExecutionContext: Object
local ExecutionContext = class("ExecutionContext")

local function traceback(err)
    log.error(debug.traceback(err, 2))
end

---Start execution
---@param self ExecutionContext
---@return any
ExecutionContext.__call = function(self)
    local result = {xpcall(self.callable_task, self.exception_info:catcher(), unpack(self.argument_list))}
    if result[1] == false then
        result[2] = self.exception_info
        for k, v in ipairs(self.exception_handlers) do
            if v.name == self.exception_info.name then
                xpcall(v.handler, traceback, self.exception_info)
            end
        end
        if self.generic_handler ~= nil then
            xpcall(self.generic_handler, traceback, self.exception_info)
        end
    end
    if self.final_task ~= nil then --- pass the results to the final task
        self.final_task(unpack(result))
    else
        return unpack(result)
    end
end

---constructor
---@param func function
---@param ... any args
function ExecutionContext:constructor(func, ...)
    self.exception_info = Exception.new() ---@type Exception
    self.exception_handlers = {}
    self.generic_handler = nil
    self.callable_task = func
    self.argument_list = {...}
    self.final_task = nil

    ---Install a exception handler
    ---@overload fun(name: string, handler: function): ExecutionContext
    ---@overload fun(handler: function): ExecutionContext
    ---@param ctx? ExecutionContext Will be ingored
    ---@param name? string exception name
    ---@param handler function exception handler
    ---@return ExecutionContext
    self.catch = function(ctx, name, handler)
        if type(ctx) ~= "table" then
            name, handler = ctx, name
        end
        if type(name) == "function" then
            handler = name
            name = nil
        end
        if name == nil then
            self.generic_handler = handler
        else
            table.insert(self.exception_handlers, {
                name = name,
                handler = handler
            })
        end
        return self
    end

    ---Finally block
    ---@overload fun(handler: function): ExecutionContext
    ---@param ctx? ExecutionContext Will be ingored
    ---@param handler function
    ---@return ExecutionContext
    self.finally = function(ctx, handler)
        if type(ctx) == "function" then
            handler = ctx
        end
        self.final_task = handler
        return self
    end
end

--- using try(function() end).catch('ErrorException', function(e) end).finally(funcion() end)()
---@param callable any
---@param ... any 
---@return ExecutionContext
function try(callable, ...)
    return ExecutionContext.new(callable, ...)
end

---Throw a exception
---@overload fun(e:Exception)
---@param name string|Exception exception name or a Exception object
---@param msgFormat string more exception message
---@param ... any msg args
---@return any
function throw(name, msgFormat, ...)
    local msg = {}
    if type(name) == "table" then
        local e = name
        msg.name = e.name
        msg.message = e.message
        msg.stacktrace = e.stacktrace
        msg.file = e.file
        msg.line = e.line
    else
        msg.name = name
        if msgFormat then
            msg.message = string.format(msgFormat, ...)
        else
            msg.message = ""
        end
    end
    return error(json.encode(msg), 2)
end

local exports = {}

exports.ErrorException = "ErrorException"
exports.DatabaseException = "DatabaseException"
exports.RedisException = "RedisException"

return exports
