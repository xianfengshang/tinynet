--- Cluster service
local class = require("tinynet/core/class")
local gevent = require("tinynet/core/gevent")
local cluster = require("tinynet/core/cluster")
local exception = require("tinynet/core/exception")

local ErrorCode = {
    ERROR_OK = 0,

    ERROR_INTERNAL_SERVER_ERROR = 3 -- //Internal Server Error
}

---@class ClusterService: Object
local ClusterService = class("ClusterService")

function ClusterService:constructor()
    -- Service name
    self.name = ""

    -- Middleware list
    self.middlewares = {}

    -- Eearly filter function executed before middleware functions
    self.before_fun = nil

    -- Filter function executed after all middleware functions
    self.after_fun = nil

    -- Error handler
    self.error_fun = nil
end

--- ClusterService.use Install middleware function
--- @param middleware fun(ctx:any)
function ClusterService:use(middleware)
    table.insert(self.middlewares, middleware)
end

---- ClusterService.before Install early filter that executed  before middleware functions
--- @param before_fun fun(ctx:any) filter function 
function ClusterService:before(before_fun)
    self.before_fun = before_fun
end

--- ClusterService.after Install filter that executed  after all middleware functions
--- @param after_fun fun(ctx:any) filter function
function ClusterService:after(after_fun)
    self.after_fun = after_fun
end

function ClusterService:panic(error_fun)
    self.error_fun = error_fun
end

--- ClusterService.start Start the cluster service
--- @param id string node id
--- @param opts any Options for cluster service
function ClusterService:start(id, opts)
    cluster.start(id, opts, function(data)
        local ctx = {
            raw_body = data
        }
        gevent.xpspawn(self.run, function(e)
            if self.error_fun then
                self.error_fun(ctx, e)
            else
                log.error("Unhandled Exception:%s", tostring(e))
            end
        end, self, ctx)
    end)
end

--- ClusterService.stop Stop the cluster service
function ClusterService:stop()
    cluster.stop()
end

--- ClusterService.runMiddlewares Execute the middleware functions
--- @param ctx any Request context
function ClusterService:runMiddlewares(ctx)
    for _, mid in ipairs(self.middlewares) do
        if mid(ctx) == false then
            break
        end
    end
end

--- ClusterService.run Process the request
--- @param ctx any context
function ClusterService:run(ctx)
    if self.before_fun then
        self.before_fun(ctx)
    end
    self:runMiddlewares(ctx)
    if self.after_fun then
        self.after_fun(ctx)
    end
end

return ClusterService
