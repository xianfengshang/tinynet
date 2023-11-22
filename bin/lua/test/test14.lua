

local mysql = require("tinynet/core/mysql")
local redis = require("tinynet/core/redis")
local gevent = require("tinynet/core/gevent")

local device_infos = {}

local function create_device_info(dbinfo)
    local info = {}
    info.device_id = dbinfo.deviceId
    info.uid = dbinfo.uid
    info.record_time = os.date("*t", dbinfo.recordTime)
    device_infos[info.device_id] = info
end

local dates = 
{
    {year=2020, month=12, day=21},
    {year=2020, month=12, day=22},
    {year=2020, month=12, day=23},
    {year=2020, month=12, day=24},
    {year=2020, month=12, day=25},
    {year=2020, month=12, day=26},
    {year=2020, month=12, day=27},
    {year=2020, month=12, day=28},
    {year=2020, month=12, day=29},
    {year=2020, month=12, day=30},
    {year=2020, month=12, day=31},
    {year=2021, month=1, day=1},
    {year=2021, month=1, day=2},
    {year=2021, month=1, day=3},
}

local function LoadDeviceInfos()
    local opts = {}
    opts.host = "127.0.0.1"
    opts.port = 3306
    opts.user = "root"
    opts.password = "root"
    opts.database = "ice_cream_global"
    opts.encoding = "utf8mb4"
    opts.logSql = "true"
    opts.logConnect = "true"
    opts.connectionLimit = 10
    local conn = mysql.new(opts)
    local sql
    local data
    for i = 0, 9 do
        sql = string.format("select * from `device_info_%s`", i)
        data = conn:execute(sql)
        for k, v in pairs(data.rows) do
            create_device_info(v)
        end
    end
end

local stat_infos = {}

local function create_stat_info(date)
    local info = {}
    info.date = date
    info.total = 0
    info.bound_uid = 0
    return info
end

local function StateSingleDay(date)
    local stat = create_stat_info(date)
    for k, v in pairs(device_infos) do
        if v.record_time.year == date.year and v.record_time.month == date.month and v.record_time.day == date.day then
            stat.total = stat.total + 1
            if v.uid ~= 0 then
                stat.bound_uid = stat.bound_uid + 1
            end
        end
    end
    table.insert(stat_infos, stat)
end

local function StatData()
    for k, v in pairs(dates) do
        StateSingleDay(v)
    end
end

local function OutputStat()
    for k, v in pairs(stat_infos) do
        log.warning(string.format("%s-%s-%s %s %s", v.date.year, v.date.month, v.date.day, v.total, v.bound_uid))
    end
end

local function main()
    LoadDeviceInfos()
    StatData()
    OutputStat()
end

gevent.pspawn(main)