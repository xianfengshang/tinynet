
local class = require("tinynet/core/class")

local Callback = class("Callback")

Callback.__call = function(...)
    return self.fun(self.ob, ...)
end

Callback.__eq = function(lhs, rhs)
    return lhs.obj == rhs.obj and lhs.fun == rhs.fun
end

function Callback:constructor(obj, fun)
    self.obj = obj
    self.fun = fun
end


return Callback