local rjson = rjson
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
