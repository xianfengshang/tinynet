---Find a value in the array, return the array index if the value exists, otherwise return nil
---@param tb table
---@param value any
---@return number|nil
function table.find(tb, value)
    for i = 1, #tb do
        if tb[i] == value then
            return i
        end
    end
end

---Find a value in the array with predication, return the array index if the predication return true, otherwise return nil
---@param tb table
---@param pred fun(a:any)
function table.find_if(tb, pred)
    for i = 1, #tb do
        if pred(tb[i]) then
            return i
        end
    end
end

---Copy a table's elements to another table
---@param dst table target table
---@param src table source table
function table.copy(dst, src)
    for k, v in pairs(src) do
        dst[k] = v
    end
end

---Clone the given table by shallow copy
---@param tb table
---@return table
function table.clone(tb)
    local result = {}
    table.copy(result, tb)
    return result
end

---Merge a hash table to another
---@param dst any
---@param src any
function table.merge(dst, src)
    for k, v in pairs(src) do
        dst[k] = v
    end
end

---Append a array to another
---@param dst table
---@param src table
function table.append(dst, src)
    for k, v in ipairs(src) do
        table.insert(dst, v)
    end
end

---Return all then table keys as array
---@param tb any
---@return table
function table.keys(tb)
    local results = {}
    for k, v in pairs(tb) do
        table.insert(results, k)
    end
    return results
end

--- Return all the table values as array
---@param tb any
---@return table
function table.values(tb)
    local results = {}
    for k, v in pairs(tb) do
        table.insert(results, v)
    end
    return results
end
--- Returns the sum of the total elements number in table given table
---@param tb table
---@return integer
function table.nkeys(tb)
    local n = 0
    for k, v in pairs(tb) do
        n = n + 1
    end
    return n
end

--- Returns whether the given table is empty or not
---@param tb any
---@return boolean
function table.is_empty(tb)
    return next(tb) == nil
end

--- Check whether the given table is array or not.
---@return boolean
function table.is_array(tb)
    return #tb > 0 and table.nkeys(tb) == #tb
end

--- Returns the intersection set of two sets
---@param a any[]
---@param b any[]
---@return any[]
function table.set_intersection(a, b)
    local b_set = {}
    for k, v in ipairs(b) do
        b_set[v] = true
    end
    local results = {}
    for k, v in ipairs(a) do
        if b_set[v] then
            table.insert(results, v)
        end
    end
    return results
end

local function bisect_cmp(a, b)
    return a < b
end

---@param tb any
---@param pred any
function table.bisect(tb, entry, pred)
    pred = pred or bisect_cmp
    local mid, left, right = nil, 1, #tb
    while left <= right do
        mid = left + math.floor((right - left) / 2)
        if pred(tb[mid], entry) then
            left = mid + 1
        else
            right = mid - 1
        end
    end
    return left
end

function table.slice(tb, first, last)
    local len = #tb
    local ret = {}
    if first > len then
        return ret
    end
    if last > #tb then
        last = #tb
    end
    for i = first, last do
        table.insert(ret, tb[i])
    end
    return ret
end

local function table_foreach(tb, fn)
    for k, v in pairs(tb) do
        fn(k, v)
    end
end

local function table_foreachi(tb, fn)
    for k, v in ipairs(tb) do
        fn(k, v)
    end
end

table.foreach = table.foreach or table_foreach
table.foreachi = table.foreachi or table_foreachi