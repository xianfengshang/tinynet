local Application = require("tinynet/application")
local class = require("tinynet/core/class")
local log = log
local env = env

---@class test.app: Application
local App = class("App", Application)

function App:constructor(...)
    App.super.constructor(self, ...)
end

function App:InitModules()
    local test_cases =
    {
        --"test/test1",
        --"test/test2",
        --"test/test3",
        --"test/test4",
        --"test/test5",
        --"test/test6",
        --"test/test8",
        --"test/test11",
        --"test/test12",
        --"test/test16",
        --"test/test17",
        --"test/test18",
        --"test/test19",
        --"test/test20",
        --"test/test21"
        --"test/test22"
        --"test/test23"
        --"test/test24"
        --"test/test25",
        --"test/test26",
        -- "test/test28"
        --"test/test29",
        -- "test/test30",
        -- "test/test31",
        --"test/test32",
        --"test/test33",
        --"test/test34"
        "test/test35"
    }
    for k, v in pairs(test_cases) do
        require(v)
    end
end

function App:Start()
    self:InitEnv()
    self:InitConfig()
    self:InitModules()
end

local exports = App.new()

OnApplicationQuit = function() exports:OnApplicationQuit() end
OnApplicationReload = function() exports:OnApplicationReload() end

return exports
