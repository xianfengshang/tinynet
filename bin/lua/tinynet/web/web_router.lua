local class = require("tinynet/core/class")

local table_find = table.find
if table_find == nil then
    table_find = function(tb, value)
        for i = 1, #tb do
            if tb[i] == value then
                return i
            end
        end
    end
end

local StatusCode =
{
    METHOD_NOT_ALLOWED = 405
}

---@class WebRouter:Object
local WebRouter = class("WebRouter")

local FULL_MEHTODS = {"HEAD", "OPTIONS", "GET", "PUT", "PATCH", "POST", "DELETE"}
local full_methods = {"head", "options", "get", "put", "patch", "post", "delete"}

function WebRouter:constructor(opts)
    self.opts = opts or {}
    self.methods = self.opts.methods or FULL_MEHTODS
    self.stack = {}
    self.all = {}
    for _, method in ipairs(full_methods) do
        self[method] = {} 
    end
end

function WebRouter:use(...)
    local args = {...}
    if #args == 0 then
        return
    end
    local middleware = {}
    if type(args[1]) == "string" then
        middleware.path = args[1]
        middleware.handler = args[2]
    else
        middleware.path = "(.*)"
        middleware.handler = args[1]
    end
    table.insert(self.stack, middleware)
end


function WebRouter:prefix(prefix)
    self.opts.prefix = prefix
end

function WebRouter:match_handler(route, path)
    for k, v in pairs(route) do
        if string.match(path, k) == path then
            return v
        end
    end
end

function WebRouter:match(path, method)
    local handlers = {}
    for k, v in pairs(self.stack) do
        if path ~= "" and string.match(path, v.path) == path then
            table.insert(handlers, v.handler)
        end
    end

    if table_find(self.methods, method) ~= nil then
        local handler = self:match_handler(self.all, path)
        if handler ~= nil then
            table.insert(handlers, handler)
        end
    end

    local route = self[string.lower(method)]
    if route ~= nil then
        local handler = self:match_handler(route, path)
        if handler ~= nil then
            table.insert(handlers, handler)
        end
    end
    return handlers
end

local function run_handlers(handlers, ctx)
    local ret
    for _, handler in ipairs(handlers) do
        ret = handler(ctx)
        if ret == false then
            return false
        elseif ret == nil then
            return
        end
    end
    return true
end

--- 获取路由处理函数
---@return function
function WebRouter:routes()
    local dispatcher = function(ctx)
        local path = ctx.req.path
        if self.opts.prefix then
            local pattern, count = "^" .. self.opts.prefix, 0
            path, count = string.gsub(path, pattern, "")
            if count == 0 then
                return true
            end
        end
        local handlers = self:match(path, ctx.req.method)
        if #handlers == 0 then
            return true
        end
        local old_path = ctx.req.path
        ctx.req.path = path 
        local ret = run_handlers(handlers, ctx)
        ctx.req.path = old_path
        return ret
    end
    return dispatcher
end

return WebRouter
