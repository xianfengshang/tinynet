local exception = require("tinynet/core/exception")
local AppUtil = require("tinynet/util/app_util")

local string_match = string.match
local math_random = math.random
local string_format = string.format
local RouteUtil = {}

exception.NoServiceAvailable = "NoServiceAvailable"

local ROUTE_PREFIX = "route_"

---Parse route info from url
---@param route string route url
---@return table route info
function RouteUtil.parse_route(route)
    if type(route) ~= "string" then
        return
    end
    local app_name, service, method = string_match(route, "([%w_]+)%.([%w_]+)%.([%w_]+)")
    if not app_name then
        return
    end
    local RouteEntry = {}
    RouteEntry.app = app_name
    RouteEntry.service = service
    RouteEntry.method = method
    return RouteEntry
end

function RouteUtil.common_route(app_name, session, msg)
    local app = AppUtil.get_app()
    local servers = app:GetAppsByName(app_name)
    if #servers == 0 then
        return throw(exception.NoServiceAvailable, "No %s service available", app_name)
    end
    if session == nil or session.uid == 0 then
        return servers[math_random(1, #servers)]
    end
    local key = ROUTE_PREFIX .. app_name
    local app_id = session.data[key]
    if app_id ~= nil and table.find(servers, app_id) then
        return app_id
    end
    app_id = servers[session.uid % #servers + 1]
    session.data[key] = app_id
    session:push(key)
    return app_id
end

return RouteUtil
