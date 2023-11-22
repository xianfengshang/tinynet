local redis = require("tinynet/core/redis")
local gevent = require("tinynet/core/gevent")
local class = require("tinynet/core/class")
local timer = require("tinynet/core/timer")

local SampleCluster = class("SampleCluster")

function SampleCluster:constructor()
    self.sid = ""
    ---@type Redis
    self.redis = nil
end

function SampleCluster:Init(sid)
    self.sid = sid

    local opts = {}
    opts.host = "localhost"
    opts.port = 6379
    self.redis = redis.new(opts)
    local key = string.format("Cluster:%s", self.sid)
    self.redis:subscribe(key, function (pkt)
        self:OnMessage(pkt)
    end)
end

function SampleCluster:OnMessage(pkt)
    log.warning("收到消息:%s", rjson.encode(pkt))
end

function SampleCluster:SendMessage(sid, msg)
    local pkt = {}
    pkt.source = self.sid    
    pkt.remove = sid
    pkt.data = msg    
    local key = string.format("Cluster:%s", sid)
    self.redis:command({"publish", key, rjson.encode(pkt)}, function (res)
        log.warning(tostring(res))
    end)
end


local world = SampleCluster.new()
world:Init("world")

local global = SampleCluster.new()
global:Init("global")

local function test_world()
    world:SendMessage("global", "Hello")
end

local function test_global()
    global:SendMessage("world", "Hello")
end

timer.start_repeat(10000, test_world)
timer.start_repeat(10000, test_global)