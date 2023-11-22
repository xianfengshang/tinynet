local math_floor = math.floor
local time = time

local DateUtil = {}

---Game world epoch time
local epoch_time = time()
---Real world time point when epoch time has been setting
local epoch_begin_time = time()

--- Get current timestamp in seconds of the game world
function DateUtil.time()
    return math_floor(epoch_time + time() - epoch_begin_time)
end

--- Get current timestamp in millseconds of the game world
function DateUtil.time_ms()
    return math_floor((epoch_time + time() - epoch_begin_time) * 1000)
end

--- DateUtil.modifytime Reset then game world epoch time to the giving time point
-- @param tmval UTC timestamp
function DateUtil.modifytime(tmval)
    epoch_time = tmval
    epoch_begin_time = time()
end

--- DateUtil.totime Convert datetime string to timestamp
-- @param datestring datetime string
function DateUtil.totime(datestring)
    local date = {}
    date.year, date.month, date.day, date.hour, date.min, date.sec = string.match(datestring, "(%d+)[^%w](%d+)[^%w](%d+)%s+(%d+):(%d+):*(%d*)")
    for k, v in pairs(date) do
        date[k] = tonumber(v) or 0
    end
    return os.time(date)
end

--- DateUtil.nowdate Get datetime string of now
function DateUtil.nowdate()
    return os.date("%Y-%m-%d %H:%M:%S", DateUtil.time())
end

function DateUtil.datetime()
    local t1, t2 = math.modf(time())
    return string.format("%s.%03d", os.date("%Y-%m-%d %X", t1), t2 * 1000)
end

--- DateUtil.tomorrowbegin Get timestamp for the start of the next day
function DateUtil.tomorrowbegin(timestamp)
    timestamp = timestamp or DateUtil.time()
    local today = os.date("*t", timestamp)
    today.hour = 0
    today.min = 0
    today.sec = 0
    local result = os.time(today) + 86400
    return result
end

local RESET_OFFSET_TIME = 5 * 3600
--- DateUtil.nextDailyResetTime Get timestamp of next daily reset time
function DateUtil.nextDailyResetTime()
    return DateUtil.tomorrowbegin() + RESET_OFFSET_TIME
end

--- DateUtil.nextWeeklyResetTime Get timestamp of next weekly reset time
function DateUtil.nextWeeklyResetTime()
    return DateUtil.nextweekbegin() + RESET_OFFSET_TIME
end

--- DateUtil.nextMonthlyResetTime Get timestamp of next montyly reset time
function DateUtil.nextMonthlyResetTime()
    return DateUtil.nextmonthbegin() + RESET_OFFSET_TIME
end

function DateUtil.nextYearlyResetTime()
    return DateUtil.nextyearbegin() + RESET_OFFSET_TIME
end

--- DateUtil.todaybegin Get timestamp for the start of today
function DateUtil.todaybegin(timestamp)
    timestamp = timestamp or DateUtil.time()
    local today = os.date("*t", timestamp)
    today.hour = 0
    today.min = 0
    today.sec = 0
    return os.time(today)
end

--- DateUtil.dayFivebegin Get timestamp for the start of today+5h
function DateUtil.dayFivebegin()
    return DateUtil.todaybegin() + 5 * 3600
end

--- DateUtil.weekbegin Get timestamp for the start of this week
function DateUtil.weekbegin()
    local today = os.date("*t", DateUtil.time())
    local wday = today.wday - 1
    if wday == 0 then
        wday = 7
    end
    today.hour = 0
    today.min = 0
    today.sec = 0
    local result = os.time(today) - (wday - 1) * 86400
    return result
end

function DateUtil.nextweekbegin()
    return DateUtil.weekbegin() + 7 * 86400
end

--- DateUtil.monthbegin Get timestamp for the start of this month
function DateUtil.monthbegin()
    local today = os.date("*t", DateUtil.time())
    today.day = 1
    today.hour = 0
    today.min = 0
    today.sec = 0
    return os.time(today)
end

function DateUtil.nextmonthbegin()
    local today = os.date("*t", DateUtil.time())
    if today.month == 12 then
        today.year = today.year + 1
        today.month = 1
    else
        today.month = today.month + 1
    end
    today.day = 1
    today.hour = 0
    today.min = 0
    today.sec = 0
    return os.time(today)
end

function DateUtil.yearbegin()
    local today = os.date("*t", DateUtil.time())
    today.month = 1
    today.day = 1
    today.hour = 0
    today.min = 0
    today.sec = 0
    return os.time(today)
end

function DateUtil.nextyearbegin()
    local today = os.date("*t", DateUtil.time())
    today.year = today.year + 1
    today.month = 1
    today.day = 1
    today.hour = 0
    today.min = 0
    today.sec = 0
    return os.time(today)
end

--- DateUtil.istoday Check if the giving timestamp is today
-- @param timestamp UTC timestap
function DateUtil.istoday(timestamp)
    local today = os.date("*t", DateUtil.time())
    local other = os.date("*t", timestamp)
    return other.year == today.year and other.yday == today.yday
end

--- DateUtil.todatestring Covert the giving timestamp to datetime string
-- @param tm UTC timestamp
function DateUtil.todatestring(tm)
    return os.date("%Y-%m-%d %H:%M:%S", tm)
end

--- DateUtil.gettimezoneoffset Get the timezone offset
function DateUtil.gettimezoneoffset()
    local now = os.time()
    return os.difftime(os.time(os.date("!*t", now)), now) / 60
end

function DateUtil.minutebegin()
    local today = os.date("*t", DateUtil.time())
    today.sec = 0
    local result = os.time(today) 
    return result
end

function DateUtil.hourbegin()
    local today = os.date("*t", DateUtil.time())
    today.min = 0
    today.sec = 0
    local result = os.time(today) 
    return result
end

return DateUtil
