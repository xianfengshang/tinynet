--Redis cluster implemention
local class = require("tinynet/core/class")
local exception = require("tinynet/core/exception")
local unpack = unpack or table.unpack
local sha1_sum = sha1_sum
local time = time
local table_insert = table.insert
local string_format = string.format
local json = rjson

local kRedisClusterSlots = 16384
local kMaxRedirection = 16

local ClusterNode = class("ClusterNode")

--@brief constructor
--@param {String} host host name
--@param {Number} port number
function ClusterNode:constructor(opts)
    self.hash = sha1_sum(rjson.encode(opts))
    self.name = string.format("%s:%s", opts.host, opts.port)
    self.driver = tinynet.redis.new(opts)
    self.opts = opts
    assert(self.driver, string.format("Cant not create redis cluster node: %s", self.name))
end

local function wrap_callback(begin, opts, cmd, callback)
    return function(res, err)
        if opts.logCommand then
            local now = time()
            local cmd_info = {}
            cmd_info.host = opts.host
            cmd_info.cmd = cmd
            cmd_info.status = err or "OK"
            cmd_info.res = res
            cmd_info.RT = string_format("%.3f ms", (now - begin) * 1000)
            cmd_info.time = now
            log.write_file_log(opts.logFile, string_format("%s\n", json.encode(cmd_info)))
        end
        return callback(res, err)
    end
end

local function default_callback(...)
end

function ClusterNode:command(cmd, callback)
    local yieldable
    if callback == nil then
        local co, b = coroutine.running()
        if co ~= nil and not b then
            yieldable = true
            callback = function(...)
                coroutine.resume(co, ...)
            end
        else
            callback = default_callback
        end
    end
    local err = self.driver:command(cmd, wrap_callback(time(), self.opts, cmd, callback))
    if yieldable then
        if err ~= nil then
            return throw(exception.RedisException, err)
        else
            local res, err1 = coroutine.yield()
            if err1 ~= nil then
                return throw(exception.RedisException, err1)
            else
                return res
            end
        end
    else
        if err ~= nil then
            return callback(nil, err)
        end
    end
end

function ClusterNode:has_slot(slot)
    if type(slot) ~= "number" then
        return false
    end
    return slot >= self.opts.slot_range[1] and slot <= self.opts.slot_range[2]
end

function ClusterNode:asking(callback)
    return self:command('ASKING', callback)
end

local RedisCluster = class("RedisCluster")

local function parse_slot(slot, nodes)
    nodes = nodes or {}
    for i, addr in ipairs(slot) do
        if type(addr) == "table" then
            local config = {
                host = addr[1],
                port = addr[2],
                slot_range = {slot[1], slot[2]},
                role = i == 3 and "master" or "slave"
            }
            table.insert(nodes, config)
        end
    end
    return nodes
end

local function parse_slots(slots)
    local nodes = {}
    for _, slot in pairs(slots) do
        parse_slot(slot, nodes)
    end
    return nodes
end

function RedisCluster:constructor(nodes)
    self.nodes = {}
    for _, node in pairs(nodes) do
        local config = {
            host = node.host,
            port = node.port,
            slot_range = {0, kRedisClusterSlots},
            role = "master"
        }
        local cluster_node= ClusterNode.new(config)
        self.nodes[cluster_node.name] = cluster_node
    end
    self:cluster_slots(function(res, err)
        if err then
            log.error(err)
            return
        end
        self:cache_slots(res)
    end)
end

function RedisCluster:cache_slots(slots)
    local nodes = parse_slots(slots)
    local name, hash, cluster_node
    for _, node in pairs(nodes) do
        name = string.format("%s:%s", node.host, node.port)
        hash = sha1_sum(rjson.encode(node))
        cluster_node = self.nodes[name]
        if not cluster_node or cluster_node.hash ~= hash then
            cluster_node = ClusterNode.new(node)
            self.nodes[name] = cluster_node
        end
    end
end

function RedisCluster:cluster_key_slot(key)
    return crc16(key) % kRedisClusterSlots
end

function RedisCluster:command_get_keys(cmds)
    local cmd, key
    local tp = type(cmds)
    if tp == 'table' then
        cmd, key = unpack(cmds)
    elseif tp == 'string' then
        local cmd_array = {}
        string.gsub(cmds, '[^%s]+', function(c) table.insert(cmd_array, c) end)
        cmd, key = unpack(cmd_array)
    else
        return nil
    end
    cmd = string.lower(cmd)
    if cmd == "info" or
        cmd == "multi" or
        cmd == "exec" or
        cmd == "slaveof" or
        cmd == "config" or
        cmd == "shutdown" then
        return nil
    end
    return key
end

--@brief Get redis cluster node specified by name
--@param {String} name node name or guid or slot
--@retval redis cluster node
function RedisCluster:get_node(name)
    for _, node in pairs(self.nodes) do
        if not name then
            return node
        end
        if node.name == name then
            return node
        end
        if node.guid == name then
            return node
        end
        if node.config.role == "master"
            and node:has_slot(name) then
            return node
        end
    end
end

function RedisCluster:command(...)
    local node, name
    local cmds = {...}
    local key = self:command_get_keys(cmds)
    if key then
        name = self:cluster_key_slot(key)
    end
    node = self:get_node(name)
    local msg
    local cmd, slot, host, port
    for i = 1, kMaxRedirection do
        msg = node:command(cmds)
        if type(msg) ~= "string" then
            return msg
        end
        cmd, slot, host, port = string.match(msg, "(%w+)%s+(%d+)%s+([%d,.]+):(%d+)")
        if not cmd then
            return msg
        end
        name = string.format("%s:%d", host, port)
        if cmd == "MOVED" then
            --Refresh cache slots info
            msg = self:cluster_slots()
            self:cache_slots(msg)
            node = self:get_node(name)
            if not node then
                return throw(exception.RedisException, "Redirect failed, node:%s", name)
            end
        end
        if cmd == "ASK" then
            node = self:get_node(name)
            if not node then
                local config = {
                    host = host,
                    port = port,
                    slot_range = {0, kRedisClusterSlots},
                    role = "master"
                }
                node = ClusterNode.new(config)
                self.nodes[name] = node
            end
            node:asking()
        end
    end
    return throw(exception.RedisException, "Max redirection")
end

local function parse_value(str)
    if tonumber(str) then
        return tonumber(str)
    end
    local it = string.gmatch(str, '([%w,_]+):([^\n]+)')
    if not it then
        return str
    end
    local value = {}
    for k, v in it do
        value[k] = parse_value(v)
    end
    return value
end

local function parse_info(str)
    local info = {}
    for k, v in string.gmatch(str, '([%w,_]+):([^\n]+)') do
        info[k] = parse_value(v)
    end
    return info
end

local function info_callback(callback)
    return function(res, err)
        if err == nil then
            res = parse_info(res)
        end
        return callback(res, err)
    end
end

function RedisCluster:info(callback)
    local node = self:get_node()
    local cmd = 'info'
    if callback then
        return node:command(cmd, info_callback(callback))
    end
    local res = node:command(cmd)
    return parse_info(res)
end

function RedisCluster:cluster_info(callback)
    local node = self:get_node()
    local cmd = 'info'
    if callback then
        return node:command(cmd, info_callback(callback))
    end
    local res = node:command(cmd)
    return parse_info(res)
end

function RedisCluster:cluster_slots(callback)
    local node = self:get_node()
    return node:command('CLUSTER SLOTS', callback)
end

function RedisCluster:get(key)
    return self:command("GET", key)
end

function RedisCluster:set(key, value)
    return self:command("SET", key, value)
end

function RedisCluster:del(key)
    return self:command("DEL", key)
end

return RedisCluster
