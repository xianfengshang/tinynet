--require global functions
require("tinynet/util/init")
require("tinynet/core/exception")
require("UnityEngine/init")
local app_util  = require("tinynet/util/app_util")
local app = app_util.get_app()
try(function() app:Start() end).catch(function (e)
    log.error("Server %s start failed!\n%s", app.app_id, e)
end)()