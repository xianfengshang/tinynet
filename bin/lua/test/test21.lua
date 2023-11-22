local redis = require("tinynet/core/redis")
local gevent = require("tinynet/core/gevent")

local function flushdb()
    local ns = "sxf"
    log.info("flushdb begin...")
    local opts = {}
    opts.host = "localhost"
    opts.port = 6379
    ---@type Redis
    local conn = redis.new(opts)
    local cmd = {"keys", ns..'*'}
    local res = conn:command(cmd)
    for k, v in ipairs(res) do
        log.info("del %s", v)
        conn:del(v)
    end
    log.info("flushdb done")
end

gevent.pspawn(flushdb)
