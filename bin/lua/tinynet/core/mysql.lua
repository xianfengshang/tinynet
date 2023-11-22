local class = require("tinynet/core/class")
local exception = require("tinynet/core/exception")
local time = time
local string_format = string.format
local json = rjson
local tointeger = string_to_integer


local MYSQL_TYPE_DECIMAL    = 0     --DECIMAL or NUMERIC field
local MYSQL_TYPE_TINY       = 1     --TINYINT field
local MYSQL_TYPE_SHORT      = 2     --SMALLINT field
local MYSQL_TYPE_LONG       = 3     --INTEGER field
local MYSQL_TYPE_FLOAT      = 4     --FLOAT field
local MYSQL_TYPE_DOUBLE     = 5     --DOUBLE or REAL field
local MYSQL_TYPE_NULL       = 6     --NULL-type field
local MYSQL_TYPE_TIMESTAMP  = 7     --TIMESTAMP field
local MYSQL_TYPE_LONGLONG   = 8     --BIGINT field
local MYSQL_TYPE_INT24      = 9     --MEDIUMINT field
local MYSQL_TYPE_DATE       = 10    --DATE field
local MYSQL_TYPE_TIME       = 11    --TIME field
local MYSQL_TYPE_DATETIME   = 12    --DATETIME field
local MYSQL_TYPE_YEAR       = 13    --YEAR field
local MYSQL_TYPE_NEWDATE    = 14
local MYSQL_TYPE_VARCHAR    = 15
local MYSQL_TYPE_BIT        = 16    --BIT field
local MYSQL_TYPE_TIMESTAMP2 = 17
local MYSQL_TYPE_DATETIME2  = 18
local MYSQL_TYPE_TIME2      = 19
local MYSQL_TYPE_TYPED_ARRAY= 20
local MYSQL_TYPE_INVALID    = 243
local MYSQL_TYPE_BOOL       = 244 
local MYSQL_TYPE_JSON       = 245   --JSON field
local MYSQL_TYPE_NEWDECIMAL = 246   --Precision math DECIMAL or NUMERIC
local MYSQL_TYPE_ENUM       = 247   --ENUM field
local MYSQL_TYPE_SET        = 248   --SET field
local MYSQL_TYPE_TINY_BLOB  = 249 
local MYSQL_TYPE_MEDIUM_BLOB= 250 
local MYSQL_TYPE_LONG_BLOB  = 251 
local MYSQL_TYPE_BLOB       = 252   --BLOB or TEXT field (use max_length to determine the maximum length)
local MYSQL_TYPE_VAR_STRING = 253   --VARCHAR or VARBINARY field
local MYSQL_TYPE_STRING     = 254   --CHAR or BINARY field
local MYSQL_TYPE_GEOMETRY   = 255   --Spatial field

local type_covertion = {}
type_covertion[MYSQL_TYPE_DECIMAL] = function(value)
    return tonumber(value)
end
type_covertion[MYSQL_TYPE_TINY] = function(value)
    return tointeger(value)
end
type_covertion[MYSQL_TYPE_SHORT] = function(value)
    return tointeger(value)
end
type_covertion[MYSQL_TYPE_LONG] = function(value)
    return tointeger(value)
end
type_covertion[MYSQL_TYPE_INT24] = function(value)
    return tointeger(value)
end
type_covertion[MYSQL_TYPE_LONGLONG] = function(value)
    return tointeger(value)
end
type_covertion[MYSQL_TYPE_DECIMAL] = function(value)
    return tonumber(value)
end
type_covertion[MYSQL_TYPE_NEWDECIMAL] = function(value)
    return tonumber(value)
end
type_covertion[MYSQL_TYPE_FLOAT] = function(value)
    return tonumber(value)
end
type_covertion[MYSQL_TYPE_DOUBLE] = function(value)
    return tonumber(value)
end
type_covertion[MYSQL_TYPE_BIT] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_TIMESTAMP] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_DATE] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_TIME] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_DATETIME] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_YEAR] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_NEWDATE] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_VARCHAR] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_BOOL] = function(value)
    return to_boolean(value) 
end
type_covertion[MYSQL_TYPE_STRING] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_VAR_STRING] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_BLOB] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_SET] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_TINY_BLOB] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_MEDIUM_BLOB] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_LONG_BLOB] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_ENUM] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_GEOMETRY] = function(value)
    return value
end
type_covertion[MYSQL_TYPE_NULL] = function(value)
    return nil
end
type_covertion[MYSQL_TYPE_JSON] = function(value)
    local ok, res = pcall(rjson.decode, value)
    if ok then
        return res
    end
    return value
end

---@class Mysql:Object
local Mysql = class("Mysql")

function Mysql:constructor(options)
    self.options = {}
    self.options.logSql = to_boolean(options.logSql)
    self.options.host = options.host
    self.options.database = options.database
    self.options.logSqlFile = options.logSqlFile or "./log/mysql/mysql"
    self.driver = tinynet.mysql.new(options)
    assert(self.driver, "Cant not create mysql client")
end

local function parse_row(fields, row)
    local obj = {}
    for i, field in pairs(fields) do
        obj[field.name] = (type_covertion[field.type] or tostring)(row[i])
    end
    return obj
end

local function parse_results(res)
    local results = {}
    results.insertId = res.insertId
    results.affectedRows = res.affectedRows
    results.rows = {}
    local obj
    for _, row in pairs(res.rows) do
        obj = parse_row(res.fields, row)
        table.insert(results.rows, obj)
    end
    return results
end

local function wrap_callback(begin, opts, sql, callback)
    return function(res, err)
        if err == nil then
            res = parse_results(res)
        end
        if opts.logSql then
            local now = time()
            local log_info = {}
            log_info.database = opts.database
            log_info.host = opts.host
            log_info.sql = sql
            log_info.status = err or "OK"
            log_info.res = res
            log_info.RT = string_format("%.3f ms", (now - begin) * 1000)
            log_info.time = now
            log.write_file_log(opts.logSqlFile, string_format("%s\n", json.encode(log_info)))
        end
        return callback(res, err)
    end
end

local function default_callback(...)
end

function Mysql:execute(sql, callback)
    local yieldable
    if callback == nil then
        local co, b = coroutine.running()
        if co ~= nil and not b then
            yieldable = true
            callback = function(...)
                coroutine.resume(co, ...)
            end
        else
            callback = default_callback
        end
    end
    local err = self.driver:execute(sql, wrap_callback(time(), self.options, sql, callback))
    if yieldable then
        if err ~= nil then
            return throw(exception.DatabaseException, err)
        else
            local res, err1 = coroutine.yield()
            if err1 ~= nil then
                return throw(exception.DatabaseException, err1)
            else
                return res
            end
        end
    else
        if err ~= nil then
            return callback(nil, err)
        end
    end
end

return Mysql
