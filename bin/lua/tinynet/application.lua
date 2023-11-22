local AppUtil = require("tinynet/util/app_util")
local FsUtil = require("tinynet/util/fs_util")
local class = require("tinynet/core/class")
local table_insert = table.insert
local log = log
local env = env

---Base application
---@class Application:Object
---@field public commonConfig table
---@field public appsConfig table
---@field public mysqlConfig table
---@field public redisConfig table
local Application = class("Application")

--- Application.constructor constructor
--- call super.constructor(self) in subclass
function Application:constructor()
    self.app_id = ""
    self.app_name = ""
    self.app_env = ""
    self.app_list = {}
    self.app_map = {}
    self.commonConfig = nil
    ---@type table<string, ClusterService>
    self.services = {}
end

--- Application.InitEnv initialize runtime enviroment
function Application:InitEnv()
    -- log setting
    log.set_logtostderr(true)
    log.set_logbufsecs(10)
    log.set_minloglevel(0)
    log.set_max_log_size(128)

    log.info("OS:%s", get_os_name())

    local version = _VERSION
    if type(jit) == "table" then
        version = jit.version
    end
    log.info("Lua version: %s", version)
    log.info("Lua path: %s", package.path)

    log.info("init env")

    self.app_env = env.meta.labels.env or ""
    self.app_name = env.meta.name
    self.app_id = AppUtil.make_app_id(env.meta.labels.id, self.app_name)

    self.clusterConfig = AppUtil.require_config("cluster")
    -- progress title
    set_process_title(string.format("%s %s %s", self.clusterConfig.clusterName, self.app_id, get_exe_path()))

    -- random seed
    math.randomseed(time_ms())

    log.info("app id: %s, env: %s", self.app_id, self.app_env)
end

--- Application.InitConfig load configuration files
--- configuration files are yaml format
function Application:InitConfig()
    log.info("init config")
    local configs = {
        commonConfig = "config",
        appsConfig = "apps",
        mysqlConfig = "mysql",
        redisConfig = "redis"
    }
    local path
    for k, v in pairs(configs) do
        self[k], path = AppUtil.require_config(v)
        log.info("%s using %s", k, path)
    end
end

--- Application.InitCluster Initialize a regular node for cluster
--- Cluster node provies service such as message transfer service
function Application:InitCluster()
    log.info("init cluster")
    local ClusterUtil = require("tinynet/cluster/cluster_util")
    local ClusterService = require("tinynet/cluster/cluster_service")
    ---@type ClusterService
    local cluster_service = ClusterService.new()
    cluster_service:use(ClusterUtil.body_parser)
    cluster_service:use(ClusterUtil.routes)
    cluster_service:after(ClusterUtil.respond)
    cluster_service:panic(ClusterUtil.at_panic)
    self.clusterConfig.bytesAsString = true
    cluster_service:start(self.app_id, self.clusterConfig)
    self.services[self.app_id] = cluster_service
    log.info("Cluster node started!")
end

--- Application.InitHandler Initialize the message handlers to handle client requets
--- and remote handlers to handle rpc inside the cluster
function Application:InitHandler()
    log.info("init handler")
    local HandlerService = require("tinynet/service/handler_service")
    local handler_dir = string.format("lua/%s/handler", self.app_name)
    local remote_dirs = {}
    table_insert(remote_dirs, "lua/tinynet/cluster_remote")
    table_insert(remote_dirs, string.format("lua/%s/remote", self.app_name))
    local filter_dir = string.format("lua/%s/filter", self.app_name)
    local list_opts = {
        exts = ".lua",
        remove_ext = true,
        recursive = true,
        full_path = true
    }
    local opts = {
        handler = FsUtil.list_dir(handler_dir, list_opts),
        remote = FsUtil.list_dirs(remote_dirs, list_opts),
        filter = FsUtil.list_dir(filter_dir, list_opts)
    }
    HandlerService.Init(opts)
end

--- Application.InitMaster Intialize name node for cluster
--- The name node provides services such as naming service
function Application:InitNaming()
    log.info("init naming")
    local cluster = require("tinynet/core/cluster")
    cluster.start(self.app_id, self.clusterConfig)
    log.info("Name node started successfully!")
end

--- Application.InitGate Start a gate server to accept client connections
function Application:InitGate()
    log.info("init gate")
    local GateService = require("tinynet/gate/gate_service")
    local GateUtil = require("tinynet/gate/gate_util")
    local clientPort = env.meta.labels.clientPort or 11001
    local address = string.format("ws://*:%s", clientPort)

    local opts = {}
    opts.app_id = self.app_id
    opts.name = "gate"
    self.gate_service = GateService.new(opts)

    self.gate_service:use(GateUtil.body_parser)

    self.gate_service:use(GateUtil.routes)

    if self.commonConfig and to_boolean(self.commonConfig.logMessage) then
        self.gate_service:use(GateUtil.message_logger)
    end

    self.gate_service:use(GateUtil.access_logger)

    self.gate_service:after(GateUtil.after_each)

    self.gate_service:start(address)
    log.info("Gate service start at %s", address)
end

--- Application.InitRpc Initialize rpc stub
function Application:InitRpc()
    log.info("init rpc")
    local RpcStub = require("tinynet/rpc/rpc_stub")
    local opts = {}
    opts.app_id = self.app_id
    opts.routes = require(string.format("%s/route_util", env.meta.name))
    opts.services = {}
    for k, v in pairs(self.services) do
        table.insert(opts.services, k)
    end
    RpcStub.Init(opts)
end

--- Application.InitProtocol Initialize protobuf protocol files
function Application:InitProtocol()
    log.info("init protocol")
    local opts = {
        protoPath = { [""] = "lua/public/protocol/", ["google/protobuf"] = "lua/public/google/protobuf"},
        protoFiles = FsUtil.list_dir("lua/public/protocol/", ".proto"),
        decodeProtos = require("public/protocol/DecodeProto"),
        encodeProtos = require("public/protocol/EncodeProto"),
        namespace = "Game.Network.Proto"
    }
    local Protobuf = require("tinynet/protobuf")
    Protobuf.Init(opts)
end

--- Application.InitDatabase Initialize mysql client and redis client
function Application:InitDatabase()
    log.info("init database")
    local MysqlClient = require("common/db/mysql_client")
    MysqlClient.Init(self.mysqlConfig)

    local RedisClient = require("common/db/redis_client")
    RedisClient.Init(self.redisConfig)
end

--- Application.InitBaseModules Initialize base modules
function Application:InitBaseModules()
    log.info("init base modules")
    -- gc watcher
    local GCWatcher = require("tinynet/gc_watcher")
    GCWatcher.Init()
    -- tns watcher
    local opts = {}
    opts.tnsWatchInterval = self.clusterConfig.namingService.registrationInterval
    opts.tnsWatchKey = self.clusterConfig.namingService.nameSpace
    local TNSWatcher = require("tinynet/tns_watcher")
    TNSWatcher.Init(opts)
end

--- Application.UninitBaseModules Unintialize base modules
function Application:UninitBaseModules()
    local GCWatcher = require("tinynet/gc_watcher")
    GCWatcher.Stop()
    local TNSWatcher = require("tinynet/tns_watcher")
    TNSWatcher.Stop()
end

--- Application.InitModules initialize customized modules
function Application:InitModules()
end

--- Application.UninitModules uninitialize customized modules
function Application:UninitModules()
end

--- Application.Start Application entry
--- You can customize your application's initialization by override this function
function Application:Start()
    self:InitEnv()
    self:InitConfig()
    self:InitCluster()
    self:InitHandler()
    self:InitRpc()
    self:InitProtocol()
    self:InitDatabase()
    self:InitBaseModules()
    self:InitModules()
end

--- Get the apps by the giving app name
---@param app_name string
---@return string[] App instance id array
function Application:GetAppsByName(app_name)
    return self.app_map[app_name] or {}
end

--- Get all apps running belong the same cluster
---@return string[]
function Application:GetApps()
    local results = {}
    for k, v in ipairs(self.app_list) do
        table.insert(results, v)
    end
    return results
end

--- Load the registered services from cluster
---@param app_list string[]
function Application:LoadApps(app_list)
    self.app_list, self.app_map = {}, {}
    for k, v in ipairs(app_list) do
        local name = AppUtil.parse_app_name(v)
        if name then
            self.app_map[name] = self.app_map[name] or {}
            table.insert(self.app_list, v)
            table.insert(self.app_map[name], v)

        end
    end
end

--- Application.OnApplicationQuit
function Application:OnApplicationQuit()
    log.info("-----------------------------------------------------------------------------")
    log.info("Server %s shutdown!", self.app_id)
    log.info("-----------------------------------------------------------------------------")
    self:UninitBaseModules()
    self:UninitModules()
end

--- Application.OnApplicationReload
function Application:OnApplicationReload()
    log.info("Server %s exit!", self.app_id)
    exit()
end

return Application
