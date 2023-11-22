--require global functions
require("tinynet/util/init")
require("tinynet/core/exception")
local app_util  = require("tinynet/util/app_util")
local app = app_util.get_app()
try(function() app:Start() end).catch(function (e)
    log.fatal("Server %s start failed!\n%s", app.app_id, e)
end)()