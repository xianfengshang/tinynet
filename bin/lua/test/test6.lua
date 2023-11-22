local lua_mysql = require("test/lua_mysql")
local lua_redis = require("test/lua_redis")
local mysql = require("tinynet/core/mysql")
local redis = require("tinynet/core/redis")
local gevent = require("tinynet/core/gevent")
local timer = require("tinynet/core/timer")

local function test_lua_mysql()
    log.warning("test lua mysql")
    local conn = lua_mysql.new()
    local opts = {}
    opts.database = "ice_cream_global"
    opts.user = "root"
    opts.password = "root"
    opts.host = "127.0.0.1"
    opts.port = 3306
    opts.charset = "utf8mb4"

    local code, err = conn:connect(opts)
    if code ~= 1 then
        log.error("connect to mysql server failed, err:%s", err)
        return
    end
    log.info("connect to mysql server")
    local begin = time()
    local data, err =  conn:query("select * from `user`;")
    if not data then
        log.error("query faild, err:%s", err)
        return
    end
    local cost = (time() - begin) * 1000;
    log.info("Execute sql cost %.3f ms", cost)
end


local function test_mysql(conn)
    local opts = {}
    opts.host = "127.0.0.1"
    opts.port = 3306
    opts.user = "root"
    opts.password = "root"
    opts.database = "dev_game_0"
    opts.encoding = "utf8mb4"
    opts.logSql = true
    opts.logConnect = true
    opts.connectionLimit = 10
    opts.debug = false
    opts.use_ssl = false
    local conn = mysql.new(opts)

    local tables = {"item_data_0", "build_data_0"}
    for k, v in ipairs(tables) do
        local sql = string.format("select * from `%s` where uid = 10071;", v) 
        local data = conn:execute(sql)
        log.error("data=%s", rjson.encode(data))
        --local cost = (time() - begin)*1000
    end
    log.info("Execute sql cost %.3f ms", cost)
end

local conns = {}
local function test_redis(id)
    log.warning("test redis %d", id)
    local opts = {}
    opts.host = "localhost"
    opts.port = 6379
    local conn = redis.new(opts)

    local key = "key".. (id or 1)
    local res = conn:set(key, "111")
    local begin = high_resolution_time()
    res = conn:set(key, "123")
    local diff = high_resolution_time() - begin
    --log.warning("step1 id=%d, ok=%s, res=%s, diff=%.3f ms", id, res, diff * 1000)
    assert(res == "OK")
    res = conn:get(key)
    assert(res == "123", res)
    begin = high_resolution_time()
    res = conn:del(key)
    diff = high_resolution_time() - begin
    --log.warning("step4 id=%d, ok=%s, res=%s, diff=%.3f ms", id, ok, res, diff * 1000)

    res = conn:set(key, "123")
    assert(res == "OK")
    res = conn:get(key)
    assert(res == "123")
    res = conn:del(key)
    assert(res == 1)
end

local function test_redis1()
    log.warning("test redis1")
    local opts = {}
    opts.host = "localhost"
    opts.port = 6379
    local conn = redis.new(opts)

    local begin = high_resolution_time()
    for i = 1, 1000 do
        local key1 = "Dev:RankDB:Rank:0"
        local res1, res2 = conn:zrevrange(key1, 0, -1, true)
        log.warning("res1= %s, res2=%s", rjson.encode(res1), rjson.encode(res2))
        local key = "key1"
        local res = conn:set(key, "111")
        log.warning("res=%s", rjson.encode(res))
        assert(res == "OK")
        local begin = high_resolution_time()
        res = conn:set(key, "123")
        local diff = high_resolution_time() - begin
        assert(res == "OK")
        res = conn:get(key)
        assert(res == "123", res)
        begin = high_resolution_time()
        res = conn:del(key)
        diff = high_resolution_time() - begin

        res = conn:set(key, "123")
        assert(res == "OK")
        res = conn:get(key)
        assert(res == "123")
        res = conn:del(key)
        assert(res == 1)
    end
    local elapsed = high_resolution_time() - begin
    log.warning("elapsed = %.3fms", elapsed * 1000)
end

local MAX_CLIENT = 1
local count = 0
local begin
local function test_redis2(id)
    log.warning("test redis %d", id)
    local opts = {}
    opts.host = "localhost"
    opts.port = 6379
    local conn = redis.new(opts)

    local key = "key".. (id or 1)
    for i = 1, 10000 do
        local res = conn:set(key, "111")
    end
    local res = conn:set(key, "123")
    assert(res == "OK")
    res = conn:get(key)
    assert(res == "123", res)
    res = conn:del(key)

    res = conn:set(key, "123")
    assert(res == "OK")
    res = conn:get(key)
    assert(res == "123")
    res = conn:del(key)
    assert(res == 1)

    count = count + 1
    if count == MAX_CLIENT then
        local elapsed = high_resolution_time() - begin
        log.warning("Client num:%s, elapsed %.3f ms", MAX_CLIENT, elapsed * 1000)
    end
end

local monitor = {}
local function monitor_callback(msg)
    log.warning("monitor:%s, msg:%s", i, msg)
end

local function test_redis_monitor()
    local opts = {}
    opts.host = "localhost"
    opts.port = 6379
    monitor = redis.new(opts)
    monitor:monitor(monitor_callback)
end

local subscriber

local function subscribe_callback(msg)
    log.warning(rjson.encode(msg))
end

local function test_redis_subscribe()
    local opts = {}
    opts.host = "localhost"
    opts.port = 6379
    subscriber = redis.new(opts)
    for i = 1, 1000 do
        subscriber:subscribe("tinynet"..i, subscribe_callback)
    end
end


local function test_lua_redis()
    local conn = lua_redis.new()
    local opts = {}
    local ok, err = conn:connect("127.0.0.1", 6379, opts)
    if not ok then
        log.error("Connect to redis server error:%s", err)
        return
    end
    assert(conn:set("key1", "123") == "OK")
    local value = conn:get("key1")
    assert(value == "123")
    assert(conn:del("key1") == 1)
    log.warning("test ok")
end

log.set_logtostderr(true)
log.set_minloglevel(0)
local function test_timeout(id)
    log.warning("timeout %s", id)
end
begin = high_resolution_time()
--for i = 1, 10000 do
--    gevent.pspawn(test_redis, i)
--end
--gevent.pspawn(test_redis1)
--for i = 1, MAX_CLIENT do
--    gevent.pspawn(test_redis2, i)
--end

--timer.start_repeat(10000, function() log.flush() log.warning("flush log") end)
--gevent.spawn(test_redis_subscribe)


gevent.pspawn(test_mysql)

--gevent.pspawn(test_lua_mysql)

--gevent.spawn(test_lua_redis)

--local fd = io.open("config/mysql.yaml", 'rb')

--local content = fd:read("*a")

--local config = yaml.decode(content)

--log.warning(rjson.encode(config))
