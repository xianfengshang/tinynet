local class = require("tinynet/core/class")
local PathUtils = require("tinynet/util/path_util")
local exception = require("tinynet/core/exception")
local unpack = unpack or table.unpack

exception.HandlerNotFoundException = "HandlerNotFoundException"
exception.MethodNotFoundException = "MethodNotFoundException"

---@class HandlerService:Object
local HandlerService = class("HandlerService")

function HandlerService:constructor(opts)
    opts = opts or {}
    self.handlers = {}
    local name, m
    for _, v in pairs(opts) do
        m = require(v)
        name = PathUtils.basename(v)
        assert(type(m) == "table", string.format("Handler module %s must return a table", v))
        self.handlers[name] = m
    end
end

function HandlerService:Invoke(service, method, ...)
    local handlers = self.handlers[service]
    if not handlers then
        return throw(exception.HandlerNotFoundException, "Handler %s Not Found", service)
    end
    local handler = handlers[method]
    if not handler then
        return throw(exception.MethodNotFoundException, "Method %s Not Found", method)
    end
    return handler(...)
end

---@class FilterService:Object
local FilterService = class("FilterService")

function FilterService:constructor(opts)
    opts = opts or {}
    table.sort(opts)
    self.before_filters = {}
    self.after_filters = {}
    local m
    for _, v in pairs(opts) do
        m = require(v)
        assert(type(m) == "table", string.format("Filter module %s must return a table", v))
        if type(m.before_each) == "function" then
            table.insert(self.before_filters, m.before_each)
        end
        if type(m.after_each) == "function" then
            table.insert(self.after_filters, m.after_each)
        end
    end
end

function FilterService:before_each(service, method, msg, session)
    local res
    for _, filter in ipairs(self.before_filters) do
        res = filter(service, method, msg, session)
        if res ~= nil then
            return res
        end
    end
end

function FilterService:after_each(service, method, msg, session)
    for _, filter in ipairs(self.after_filters) do
        filter(service, method, msg, session)
    end
end

local exports = {}

--- exports.Init Initialize  handler service module
-- @param opts  input options
function exports.Init(opts)
    exports.handler = HandlerService.new(opts.handler)
    exports.remote = HandlerService.new(opts.remote)
    exports.filter = FilterService.new(opts.filter)
end

--- exports.InvokeHandler Invoke client handler by module name and method name
--- @param service string client handler module name
--- @param method  string client handler method name
--- @param msg     table client requet message
--- @param session any client session
--- @return table
function exports.InvokeHandler(service, method, msg, session)
    local res = exports.filter:before_each(service, method, msg, session)
    if res == nil then
        res = exports.handler:Invoke(service, method, msg, session)
        exports.filter:after_each(service, method, msg, session)
    end
    return res
end

--- exports.InvokeRemote Invoke remote handler by module name and method name
--- @param service string remote handler module name
--- @param method  string remote handler method name
--- @param ...     any arguments forward to the handler
function exports.InvokeRemote(service, method, ...)
    return exports.remote:Invoke(service, method, ...)
end

return exports
