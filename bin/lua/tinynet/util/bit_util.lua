local bor = bit.bor
local band = bit.band
local lshift = bit.lshift
local rshit = bit.rshift
local bnot = bit.bnot

local exports = {}

--- 置位
---@param flag integer
---@param mask integer 比特位
---@return boolean
function exports.set(flag, mask)
    return bor(flag or 0, lshift(1, mask))
end

--- 清零
---@param flag integer
---@param mask integer
function exports.clear(flag, mask)
    return band(flag, bnot(lshift(1, mask)))
end

--- 测试比特位真假
---@param flag any
---@param mask any
---@return boolean
function exports.test(flag, mask)
    return rshit(band(flag, lshift(1, mask)), mask) == 1
end

return exports