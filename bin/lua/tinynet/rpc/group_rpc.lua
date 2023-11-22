--[[
Rpc proxy 2
Example usage:
    local GroupRpc = require("tinynet/rpc/group_rpc")
    GroupRpc.manager.user_remote.test1("123")
]]
local setmetatable = setmetatable
local rawset = rawset

local RpcStub = require("tinynet/rpc/rpc_stub")
local pipeline = {}
pipeline[1] = setmetatable({}, { __index = function(t, k) rawset(pipeline[4], 'app', k)  return pipeline[2] end})
pipeline[2] = setmetatable({}, { __index = function(t, k) rawset(pipeline[4], 'service', k) return pipeline[3] end})
pipeline[3] = setmetatable({}, { __index = function(t, k) rawset(pipeline[4],  'method', k) return pipeline[4] end})
pipeline[4] = setmetatable({}, { __call = function(t, ...)
                                                return RpcStub.GroupInvoke(t.app, t.service, t.method, ...)
                                          end})

return pipeline[1]