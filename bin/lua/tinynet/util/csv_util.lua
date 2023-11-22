local ipairs = ipairs
local table_insert = table.insert
local table_concat = table.concat
local string_char = string.char
local string_byte = string.byte
local string_sub = string.sub

local function parse_csv_line(line)
    local c
    local s1, s2 = 1, 1
    local b = false
    local row = {}
    if string_char(string_byte(line, #line)) == '\r' then
        line = string_sub(line, 1, #line - 1)
    end
    for i = 1, #line do
        c = string_char(string_byte(line, i))
        if c == '\"' then
            b = not b
        elseif c == ',' then
            if not b then
                s2 = i - 1
                table_insert(row, string_sub(line, s1, s2) or "")
                s1 = i + 1
            end
        end
    end
    table_insert(row, string_sub(line, s1, #line))
    return row
end

local function merge_csv_line()
end

local function is_new_line(line)
    local quote_num = 0
    local c
    for i = 1, #line do
        c = string_char(string_byte(line, i))
        if c == '\"' then
            quote_num = quote_num + 1
        end
    end
    return quote_num % 2 == 0
end

local function remove_utf8_bom(line)
    if not line then
        return
    end
    if #line < 3 then
        return line
    end
    if string_byte( line, 1) == 239 and string_byte( line ,  2) == 187 and string_byte( line, 3 ) == 191 then
        return string_sub( line, 4, #line)
    end
    return line
end

local function io_lines (path)
    local content = fs.readfile(path)
    local lines = string.split(content, "\r*\n")
    local i = 0
    local n = #lines
    return function ()      -- iterator function
        i = i + 1
        if i <= n then
            return lines[i]
        end
    end
end

--- require_csv Import csv formatted file from disk
-- @param path An existing csv file path
function require_csv(path)
    local rows = {}
    local row
    local columns
    local obj
    local lineno = 1
    local temp = {}
    for line in io_lines(path) do
        table_insert(temp, line)
        line = table_concat(temp, "")
        if is_new_line(line) then
            if lineno == 1 then
                -- Comment
            elseif lineno == 2 then
                columns = parse_csv_line(line)
            elseif lineno == 3 then
                -- Type description
            elseif lineno == 4 then
                -- Not used
            else
                row = parse_csv_line(line)
                obj = {}
                if columns then
                    for k, v in ipairs(columns) do
                        obj[v] = row[k]
                    end
                end
                table_insert(rows, obj)
            end
            temp = {}
            lineno = lineno + 1
        end
    end
    return rows
end
