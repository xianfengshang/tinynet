
local timer = require("tinynet/core/timer")
local gevent = require("tinynet/core/gevent")
local http = require("tinynet/core/http")

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

local function onTimer(a)
    log.info("on timer: %s", a)
end

for i = 1, 100000 do
    timer.start_timer(i, i, function() onTimer(i) end)
end
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
local function testHttp()
    local beginTime = time()
    local data = 
    {
        seq = 1,
        token = "",
        data = 
        {
            channel = "" 
        }
    }
    local config = 
    {
        url = "serverinfo",
        baseUrl = "https://icecream-va-qa.shinezone.com",
        method = "POST",
        headers = 
        {
            ["Accept-Language"] = "zh",
            ["Content-Type"] = "application/json"
        },
        data = rjson.encode(data),
        verbose = true
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
--for i = 1, 1 do
--    gevent.spawn(testHttp)
--end

--for i = 1, 1000 do
--    gevent.spawn(testTimer, "test"..tostring(i))
--end

--gevent.spawn(test_tdo)

log.info(string.format("server name is %s", env.meta.labels.id))

log.info(hash_hmac("sha1", "data 1234567", "key aaaa"))

--@brief 将参数转换为字符串
local function param_to_string(data)
    local data_type = type(data)
    --普通数据类型
    if data_type == "number" then
        return tostring(data)
    elseif data_type == "string" then
        return data
    elseif data_type == "boolean" then
        return tostring(data)
    end
    --数组
    if data_type == "table" and #data > 0 then
        local param_list = {}
        table.sort(data)
        for k, v in ipairs(data) do
            param_list[k] = param_to_string(v)
        end
        return table.concat( param_list, "&")
    end
    --对象
    if data_type == "table" then
        local param_list = {}
        local keys = {}
        for k, v in pairs(data) do
            table.insert(keys, k)
        end
        table.sort( keys )
        for k, v in pairs(keys) do
            param_list[k] = string.format("%s=%s", v, param_to_string(data[v]))
        end
        return table.concat( param_list, "&")
    end
end

local function send_gm_message(msg)
    msg.timestamp = time()
    msg.sign = string.lower( hash_hmac("sha1", param_to_string(msg), "0af4734ad1c5539709214323659cca08") )
    local config = 
    {
        url = "gmtool",
        baseUrl = "http://localhost:13180",
        method = "POST",
        headers = 
        {
            ["Accept-Language"] = "zh",
            ["Content-Type"] = "application/json"
        },
        data = rjson.encode(msg),
        verbose = true        
    }
    local response = http.client.request(config)
    if (response.code ~= 0) then
        log.info(string.format("code =%s, msg = %s",response.code, response.msg))
    else
        if response.status ~= 200 then
            log.info(string.format("status = %s",response.status))
        else
            local data = rjson.decode(response.data)
            log.info(response.data)
        end
    end
end

local function test_sdk_payment()
    local msg = {}
    msg.action = "payment_deliver"
    msg.data = {}
    msg.data.orderId = id_to_string(new_uniqueid()) 
    msg.data.orderStatus = 3
    msg.data.orderSource = "GooglePlay"
    msg.data.guid = "qa001"
    msg.data.gameRoleId = "10023"
    msg.data.outerOrderId = id_to_string(new_uniqueid())
    msg.data.productId = "icecreamsr.diamonds.lv1"
    msg.data.productNum = 1
    msg.data.productPrice = 2.99
    msg.data.paymentMethod = 1
    msg.data.realCurrency = "USD"
    msg.data.realAmount = 299
    msg.data.extra = "3"
    msg.data.createTime = time()
    msg.data.paymentTime = time()
    msg.data.isRepairOrder = 1
    msg.data.gameServerCode = "1"
    send_gm_message(msg)
end
--gevent.spawn(test_sdk_payment)
