
local timer = require("tinynet/core/timer")
local gevent = require("tinynet/core/gevent")
local http = require("tinynet/core/http")
local redis = require("tinynet/core/redis")

local function task(i)
    gevent.sleep(10000)
    log.info("task done "..i)
end

local function main(id)
	-- body
	local tasks = {}
    for i = 1, 10 do
        table.insert(tasks, gevent.spawn(task, i))
    end
    gevent.joinall(tasks)
    log.info("main done")
end

local function onTimer()
    log.info("-------------------------")
end

--for i = 1, 100000 do
--    timer.start_timer(1000, -1, 0, onTimer)
--end
--gevent.spawn(main, 1)

--for i = 1, 1000 do
--    require_module("./modules/test"..i)
--end
--require_module("module_init")

local function init(jwt)
        local data = 
    {
        seq = 1,
        jwt = jwt,
        data = 
        {
            channel = "facebook"
        }
    }
    local config = 
    {
        url = "GameData/GetLoginGameData",
        baseUrl = "http://localhost:8360",
        method = "POST",
        headers = 
        {
            ["Accept-Language"] = "zh",
            ["Content-Type"] = "application/json"
        },
        data = rjson.encode(data)    
    } 
    local response = http.client.request(config)
    if (response.code ~= 0) then
        log.info(string.format("init code =%s, msg = %s",response.code, response.msg))
    else
        if response.status ~= 200 then
            log.info(string.format("init status = %s",response.status))
        else
            local data = rjson.decode(response.data)
            if data.errno ~= 0 then
                log.info(string.format("init errno=%s, errmsg=%s", data.errrno, data.errmsg))
            else
                log.info(response.data)
            end
        end
    end 
end
local function login(account)
    local data = 
    {
        seq = 1,
        jwt = "",
        data = 
        {
            account = account, 
            token = ""
        }
    }
    local config = 
    {
        url = "account/login",
        baseUrl = "http://localhost:8360",
        method = "POST",
        headers = 
        {
            ["Accept-Language"] = "zh",
            ["Content-Type"] = "application/json"
        },
        data = rjson.encode(data)    
    } 
    local response = http.client.request(config)
    if (response.code ~= 0) then
        log.info(string.format("login code =%s, msg = %s",response.code, response.msg))
    else
        if response.status ~= 200 then
            log.info(string.format("status = %s",response.status))
        else
            local data = rjson.decode(response.data)
            if data.errno ~= 0 then
                log.info(string.format("login errno=%s, errmsg=%s", data.errrno, data.errmsg))
            else
                init(data.data.jwt)
            end
        end
    end 
end
local function testHttp(account)
    local beginTime = time()
    local data = 
    {
        seq = 1,
        jwt = "",
        data = 
        {
            account = account 
        }
    }
    local config = 
    {
        url = "account/auth",
        baseUrl = "http://localhost:8360",
        method = "POST",
        headers = 
        {
            ["Accept-Language"] = "zh",
            ["Content-Type"] = "application/json"
        },
        data = rjson.encode(data)    
    }
    local response = http.client.request(config)
    local deltaTime = time() - beginTime
    if (response.code ~= 0) then
        log.info(string.format("auth code =%s, msg = %s",response.code, response.msg))
    else
        if response.status ~= 200 then
            log.info(string.format("auth status = %s",response.status))
        else
            local data = rjson.decode(response.data)
            if data.errno ~= 0 then
                log.info(string.format("auth errno=%s, errmsg=%s", data.errrno, data.errmsg))
            else
                -- login(account)
                log.info(string.format("auth account=%s, token=%s", data.data.account, data.data.token))
            end
        end
    end
end
--for i = 1, 1000 do
--    gevent.spawn(testHttp, "test"..tostring(i))
--end

--for i = 1, 1000 do
--    gevent.spawn(testTimer, "test"..tostring(i))
--end


--local redis = redis.new("172.16.0.116", 6379)
local function test_redis()
    local redis = redis.new("127.0.0.1", 6379)
    assert(redis)
    for j = 1, 1000000 do
        local begin = time()
        for i = 1, 1000 do
            redis:set("a", i)
            redis:get("a")
            redis:del("a")
        end
        local diff = time() - begin
        log.info(string.format("Cost: %s", diff))
    end
end
--for i = 1, 1 do
--    gevent.spawn(test_redis)
--end
