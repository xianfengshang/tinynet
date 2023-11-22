local lua_mysql = require("test/lua_mysql")
local lua_redis = require("test/lua_redis")
local mysql = require("tinynet/core/mysql")
local redis = require("tinynet/core/redis")
local gevent = require("tinynet/core/gevent")
local timer = require("tinynet/core/timer")

local dictionary = searchabledictionary.new()

local users = {}
local function test_lua()
    log.warning("test_lua begin")
    local begin = high_resolution_time()
    local uid = users["淑卿"]
    local cost = high_resolution_time() - begin
    log.warning("uid=%s cost=%.3fus", tostring(uid), cost * 1000 * 1000)
    users["淑卿"] = nil

    local begin = high_resolution_time()
    local uid =  users["淑卿"]
    local cost = high_resolution_time() - begin
    log.warning("uid=%s cost=%.3fus", tostring(uid), cost * 1000 * 1000)    
    log.warning("test_lua end")
end
local function test_search()
    log.warning("test_search begin")
    local begin = high_resolution_time()
    local uid = dictionary:TryGetValue("淑卿")
    local cost = high_resolution_time() - begin
    log.warning("uid=%s cost=%.3fus", tostring(uid), cost * 1000 * 1000)
    dictionary:Remove("淑卿")

    local begin = high_resolution_time()
    local uid = dictionary:TryGetValue("淑卿")
    local cost = high_resolution_time() - begin
    log.warning("uid=%s cost=%.3fus", tostring(uid), cost * 1000 * 1000)
    log.warning("test_search end")
end

local function test_mysql()
    local opts = {}
    opts.host = "127.0.0.1"
    opts.port = 3306
    opts.user = "root"
    opts.password = "root"
    opts.database = "dev_social"
    opts.encoding = "utf8mb4"
    opts.logSql = true
    opts.logConnect = true
    opts.connectionLimit = 10
    opts.debug = true
    opts.use_ssl = false
    local conn = mysql.new(opts)
    local begin = time()
    local data = conn:execute("select * from `social_user`;")
    log.warning("Rows=%s", #data.rows)
    for k, v in ipairs(data.rows) do
        if v.nickName ~= "" then
            users[v.nickName] = v.uid
            dictionary:Add(v.nickName, integer_to_string(v.uid))
        end
    end
    local cost = (time() - begin)*1000
    log.info("Execute sql cost %.3f ms", cost)
    test_lua()
    test_search()
end

gevent.pspawn(test_mysql)