local m = {}

--- 生成随机字符串
---@param n number
---@return string
function m.RandomString(n)
    local charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"
    local array = {}
    for _ = 1, n do
        table.insert(array, string.char(string.byte(charset, math.random(1, #charset))))
    end
    return table.concat(array, "")
end

--- 生成随机数字
---@param n number
---@return number
function m.RandomNumber(n)
    return math.floor((math.random() * 9 + 1) * math.pow(10, n - 1))
end

local randomSeed = 5
function m.RandomSeed(seed)
    randomSeed = seed
end

function m.Random()
    randomSeed = (randomSeed * 9301 + 49297) % 233280.0
    return randomSeed / 233280.0
end

function m.Shuffle(t)
    local j
    for i = #t, 1, -1 do
        j = math.random(1, i)
        t[i], t[j] = t[j], t[i]
    end
    return t
end

return m
