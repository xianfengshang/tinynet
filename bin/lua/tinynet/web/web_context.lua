local class = require("tinynet/core/class")
local StatusCode =
{
    OK = 200
}

---@class WebContext:Object
local WebContext = class("WebContext")

function WebContext:constructor(req, app)
    self.req = req
    self.res = {}
    self.res.guid = req.guid
    self.res.statusCode = StatusCode.OK
    self.res.statusMessage = ""
    self.res.headers = {}
    self.res.body = ""
    self.app = app
    self.state = {}
    self.createTime = high_resolution_time()
end

function WebContext:finish()
    self.app:sendResponse(self)
end

return WebContext