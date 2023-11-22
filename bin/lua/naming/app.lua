local Application = require("tinynet/application")
local class = require("tinynet/core/class")

---@class naming.App:Application
local App = class("App", Application)

function App:constructor(...)
    App.super.constructor(self, ...)
end

function App:InitBaseModules()
end

function App:Start()
    self:InitEnv()
    self:InitNaming()
end

function App:OnApplicationReload()
end

local exports = App.new()

OnApplicationQuit = function() exports:OnApplicationQuit() end
OnApplicationReload = function() exports:OnApplicationReload() end

return exports
