local BackendSessionService = require("tinynet/service/backend_session_service")
local HandlerService = require("tinynet/service/handler_service")

local exports = {}

local ErrorCode = {
    ERROR_OK = 0,

    ERROR_INVALID_ROUTE = 15
}

--- Dispatch client request that fowarded by frontend server(gate)
exports.forward_message = function(service, method, msg, session)
    return HandlerService.InvokeHandler(service, method, msg, BackendSessionService:create(session))
end

return exports
