--字符串分割
function string.split(str, sep)
    local fields = {}
    local pattern = '[^'.. (sep or " ") ..']+'
    string.gsub(str, pattern, function (c) table.insert(fields, c) end)
    return fields
end

--剔除首尾空格
function string.trim(str)
    return string.match(str, "^%s*(.-)%s*$")
end

--分割为数字数组
function string.numeric_split(str, sep)
    local result = string.split(str, sep)
    local numeric_array = {}
    for _, v in pairs(result) do
        table.insert(numeric_array, tonumber(v))
    end
    return numeric_array
end

--是否以字符串开头
function string.startswith(str, pattern)
    return string.match(str, string.format("^%s", pattern)) ~= nil
end

--是否以某字符串结尾
function string.endswith(str, pattern)
    return string.match(str, string.format("%s$", pattern)) ~= nil
end

--是否为nil 或者空字符串
function string.IsNullOrEmpty(str)
    return str == nil or str == ""
end

--查询第一个出现的字符
function string.find_first_of(str, tag)
    for i = 1, #str do
        local value = string.byte(str, i)
        for j = 1, #tag do
            if value == string.byte(tag, j) then
                return i
            end
        end
    end
end

--查询最后一个出现的字符
function string.find_last_of(str, tag)
    for i = #str, 1, -1 do
        local value = string.byte(str, i)
        for j = 1, #tag do
            if value == string.byte(tag, j) then
                return i
            end
        end
    end
end

local escape_pattern = "[\\%z\n\r'\"\026]"

local escape_keys = 
{
    ['\\'] = '\\\\',
    ['\0'] = '\\0',
    ['\n'] = '\\n',
    ['\r'] = '\\r',
    ['\''] = '\\\'',
    ['"'] = '\\"',
    ['\026'] = '\\Z'
}

--处理mysql转义字符
function mysql_escape_string(s)
    return string.gsub(s, escape_pattern, escape_keys);
end

function url_encode(s)
    s = string.gsub(s, "([^%w%.%- ])", function(c) return string.format("%%%02X", string.byte(c)) end)
    return string.gsub(s, " ", "+")
end

function url_decode(s)
    s = string.gsub(s, '%%(%x%x)', function(h) return string.char(tonumber(h, 16)) end)
    return s
end
