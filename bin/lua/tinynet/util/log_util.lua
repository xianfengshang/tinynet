local os_name = get_os_name()
local log = log
local iconv = iconv
local string_gsub = string.gsub
local string_format = string.format
local debug = debug
local type = type
local tostring = tostring

local function encoding(msg)
    if os_name == "Windows" then
        return iconv("utf-8", "gb2312", msg)
    end
    return msg
end

local function format_msg(fmt, ...)
    if type(fmt) ~= "string" then
        fmt = tostring(fmt)
        return fmt
    end
    local args = {...}
    if #args == 0 then
        return fmt
    end
    return string_format(fmt, ...)
end

local sync_table = 
{
    [log.LOG_LEVEL_DEBUG] = true,
    [log.LOG_LEVEL_INFO] = false,
    [log.LOG_LEVEL_WARN] = false,
    [log.LOG_LEVEL_ERROR] = true,
    [log.LOG_LEVEL_FATAL] = false
}

local function log_msg(level, fmt, ...)
    local info = debug.getinfo(2)
    local file, line = "", 0
    if info then
        file = info.short_src
        line = info.currentline
    end
    log.log(file, line, level, encoding(format_msg(fmt, ...)))
    if sync_table[level] then
        log.flush()
    end
end

---log debug message
---@param fmt string format string
---@param ... any args
function log.debug(fmt, ... )
    return log_msg(log.LOG_LEVEL_DEBUG, fmt, ...)
end

---log info message
---@param fmt string format string
---@param ... any args
function log.info(fmt, ... )
    return log_msg(log.LOG_LEVEL_INFO, fmt, ...)
end

---log warning message
---@param fmt string format string
---@param ... any args
function log.warning(fmt, ...)
    return log_msg(log.LOG_LEVEL_WARN, fmt, ...)
end

---log error message
---@param fmt string format string
---@param ... any args
function log.error(fmt, ...)
    return log_msg(log.LOG_LEVEL_ERROR, fmt, ...)
end

---log fatal message
---@param fmt string format string
---@param ... any args
function log.fatal(fmt, ...)
    return log_msg(log.LOG_LEVEL_FATAL, fmt, ...)
end

_G.old_print = print
_G.print = log.debug
