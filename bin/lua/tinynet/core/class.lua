---Class definition for lua
---@param classname string class name
---@param super Object|nil base class
---@return Object
local function class(classname, super)
    local cls
    if super then
        assert(type(super) == "table" and type(super.constructor) == "function", "super must be a valid class")
        cls = setmetatable({super = super}, { __index = super})
    else
        cls = {constructor = function(...) end}
    end

    cls.__index = cls
    cls.__name = classname
    cls.__tostring = function(o)
        return classname
    end
    
    --- Create new instance of the class
    ---@return Object
    cls.new = function(...)
        local instance = setmetatable({}, cls)
        instance.class = cls
        instance:constructor(...)
        return instance
    end
    return cls
end

return class
