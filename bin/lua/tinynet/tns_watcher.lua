local timer = require("tinynet/core/timer")
local cluster = require("tinynet/core/cluster")
local AppUtil = require("tinynet/util/app_util")

local TNSWatcher = {}

function  TNSWatcher.Init(opts)
    TNSWatcher.opts = opts
    TNSWatcher.rootPath = TNSWatcher.opts.tnsWatchKey .. '/'
    TNSWatcher.watch_timer = timer.start_timer(opts.tnsWatchInterval, opts.tnsWatchInterval, TNSWatcher.Watch)
    local initial_watch_count = opts.tnsWatchInterval / 1000
    for i = 1, initial_watch_count do
        timer.start_timeout(i * 1000, TNSWatcher.Watch)
    end
end

function TNSWatcher.Watch()
    local err = cluster.keys(TNSWatcher.opts.tnsWatchKey, function (res, err)
        if res ~= nil then
            for i = 1, #res do
                res[i] = string.gsub(res[i], TNSWatcher.rootPath, '')
            end
            AppUtil.get_app():LoadApps(res)
        end
    end)
    if err ~= nil then
        log.error("Cluster query keys error:%s", err)
    end
end

function TNSWatcher.Stop()
    if TNSWatcher.watch_timer then
        timer.stop_timer(TNSWatcher.watch_timer)
        TNSWatcher.watch_timer = nil
    end
end

return TNSWatcher
