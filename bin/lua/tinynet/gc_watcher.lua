local timer = require("tinynet/core/timer")
local GCWatcher = {}

local kGCWatchInterval =  10 * 60 * 1000
local kGCCollectThreshold = 1 * 1024 -- size in MB
local kGCCollectStep = 128 -- size in MB

function  GCWatcher.Init()
    GCWatcher.watch_timer =  timer.start_repeat(kGCWatchInterval, GCWatcher.Watch)
    if allocator and allocator.tcmalloc then
       allocator.tcmalloc.SetMemoryReleaseRate(10.0)
    end
end

function GCWatcher.Watch()
    local count = collectgarbage("count") / 1024 -- size in MB
    log.info(string.format("[LuaVM] Total memory size %.3f MB", count))
    if allocator and allocator.jemalloc then
        local stats = allocator.jemalloc.memory_stats()
        local allocated = stats.allocated / (1024 * 1024)
        local active = stats.active / (1024 * 1024)
        local resident = stats.resident / (1024 * 1024)
        log.info(string.format("[JEMALLOC] Stats info allocated:%.3f MB, active:%.3f MB, resident:%.3f MB", allocated, active, resident))
    end
    if count >= kGCCollectThreshold then
        local begin = high_resolution_time()
        collectgarbage("step", kGCCollectStep)
        local cost = (high_resolution_time() - begin) * 1000
        count = count - collectgarbage("count") / 1024
        log.info(string.format( "[LuaVM] Execute gc, collect memory %.3f MB, cost %.3f ms", count, cost))
    end
end

function GCWatcher.Stop()
    if GCWatcher.watch_timer then
        timer.stop_repeat(GCWatcher.watch_timer)
        GCWatcher.watch_timer = nil
    end
end

return GCWatcher
