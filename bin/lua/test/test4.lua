
local timer = require("tinynet/core/timer")
local gevent = require("tinynet/core/gevent")
local http = require("tinynet/core/http")
local redis = require("tinynet/core/redis")
local redis_cluster = require("tinynet/core/redis_cluster")
local mysql = require("tinynet/core/mysql")

local function test_redis()
    local cluster = redis_cluster.new({{host = "127.0.0.1", port = 7000}})
    local key = "key1"
    cluster:set(key, 1)
    local result = cluster:get(key)
    log.info("%s=%s", key, result)
    cluster:del(key)
end
gevent.spawn(test_redis)

local opts = {}
opts.host = "127.0.0.1"
opts.port = 3306
opts.user = "root"
opts.password = "root"
opts.database = "ice_cream_global"
opts.encoding = "utf8mb4"
opts.logSql = true
opts.logConnect = true
opts.connectionLimit = 10
local client = mysql.new(opts)
local function test_mysql()
    local beginTime = time()
    local msg = client:execute("select * from `user` limit 1;")
    local deltaTime = time() - beginTime
    local content = string.format("Cost=%.3f, Results=%s", deltaTime * 1000, #msg.rows)
    log.info(content)
end
gevent.spawn(test_mysql)
