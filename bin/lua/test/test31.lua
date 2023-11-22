
local temp= {
    --key = "aaa"
} 

local eee = {
    key = "aaa"
}
local mt = {
    -- __index = function(t, k)
    --     local tab = rawget(t, "__metadata")
    --     if tab then
    --         return tab[k]
    --     end
    -- end,
    __index = eee,
    -- __newindex = function(t, k, v)
        -- if t[k] ~= nil then
        --     error("Readonly table")
        --     return
        -- end

        -- if rawget(t, "__metadata") == nil then
        --     rawset(t, "__metadata", {})
        -- end
        -- local tab = rawget(t, "__metadata")
        -- if tab[k] then
        --     error("Readonly table")
        -- end
        -- tab[k] = v
    -- end
    __newindex = function(t, k, v)
        log.warning("----->>> ")
    end
}

setmetatable(temp, mt)

log.warning("temp.key:%s ms", temp.key)

temp.key = "bbb"
log.warning("temp.key:%s ms", temp.key)

temp.key = "ccc"
log.warning("temp.key2:%s ms", temp.key)





log.warning("rawget key:%s ms", rawget(temp, "key"))