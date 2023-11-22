local timer = require("tinynet/core/timer")

log.set_logtostderr(true)
local a = string.rep('a', 1024)
local function ReleaseMemory()
     --Execute tcmalloc release free memory
    local begin = high_resolution_time()
    local stat_info = tcmalloc.GetStats()
    log.info("[TCMALLOC] Before release:")
    log.info(stat_info)
    tcmalloc.ReleaseFreeMemory()
    stat_info = tcmalloc.GetStats()
    log.info("[TCMALLOC] After release:")
    log.info(stat_info)
    local elapsed = high_resolution_time() - begin
    log.info("ReleaseMemory cost %.3f ms", elapsed * 1000)
end
local function timer_callback(id)
    log.warning("id = %s", id)
    log.warning(a)
    if id == 10000 then
        --local mem = collectgarbage("count") / 1024
        --ReleaseMemory()
        log.warning("id="..id)
    end
end

--[[
for i= 1, 100000 do
    timer.start_repeat( i*2, function() timer_callback(i) end)
end
--]]
log.debug("debug log")
log.info("info log")
log.warning("warning log")
log.error("error log")
log.write_file_log("./log/test1/redis", "this is redis log")
log.write_file_log("./log/test1/mysql", "this is mysql log")
log.write_file_log("./log/test1/payment", "this is payment log")


local a = {}
a.float_value = 3.1415926
a.double_value = 3.141592653589793
a.string_value = "this is a string"
a.utf8_value = "中文字符串"
a.uint_value = 4294967295
a.int_value = -2147483648
a.int64_value = "9223372036854775807"
a.number_array = {1, rjson.null, 3, 4, 5, 6, 7, 8, 9, 10}
a.string_array = {"apple", "banana", "orange"}
a.object_array = {{id=1, name="obj1"}, {id=2, name="obj2"}, {id=3, name="obj3"}}
a.empty_table = {}
a.large_array = {}
for i = 1, 1000 do
    a.large_array[i] = {id=i, name="large"..i}
end

local b = rjson.encode(a)
log.warning("test case:")
--log.warning("rjson:%s", b)
--log.warning("rjson:%s", rjson.encode(a))
local c = bytes.new(b)
local count = 1000
local begin = high_resolution_time()
for i = 1, count do
    a.changed_value = i
    rjson.encode(a)
    rjson.decode(b)
end
local elapsed = high_resolution_time() - begin
log.warning("rjson cost:%.3f ms", elapsed * 1000)

local opts = {encode_as_bytes = true}
begin = high_resolution_time()
local tab = {}
for i = 1, count do
    a.changed_value = i
    rjson.encode(a)
    rjson.decode(b)
end
tab = nil

local timer = require("tinynet/core/timer")
timer.start_repeat(10000, function() 
    collectgarbage("collect")
    tcmalloc.ReleaseFreeMemory()
    log.warning("Release memory")
end)
local elapsed = high_resolution_time() - begin
log.warning("rjson cost:%.3f ms", elapsed * 1000)

--可选项
local opts = {}
opts.encode_max_depth = 100 --编码允许最大嵌套深度
opts.decode_max_depth = 100 --解码允许最大嵌套深度
opts.order_map_entries = true --保持key有序
opts.empty_table_as_array = true --空表视为数组
opts.pretty_format = false --以便于阅读的格式化输出
opts.encode_as_bytes = true --是否使c++ std::string

--local data = rjson.encode(a, opts)
--log.warning(data:get_data())
local data = rjson.decode(b)
print(type(package.loaders))
for k, v in pairs(package.loaded) do
    print(k)
end
