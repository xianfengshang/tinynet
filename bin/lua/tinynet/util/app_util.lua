local env = env
local AppUtil = {}

--- AppUtil.get_app Get the application instance
---@return Application
function AppUtil.get_app()
    return require(string.format("%s/app", env.meta.name))
end

local WILDCARD_MAP = {
    HOST = string.gsub(get_ip(), "%.", "-")
}

local function replace_wildcard(data)
    return string.gsub(data, "${(%w*)}", WILDCARD_MAP)
end

function AppUtil.make_app_id(id, app_name)
    id = replace_wildcard(id)
    if app_name == "naming" then
        return id
    end
	local id_parts = string.split(id, '/')
    if #id_parts == 2 and id_parts[1] == app_name then
        return id 
    end
    if #id_parts >= 2 then
        id = id_parts[2]
    end
    id_parts = {app_name, id}
    return table.concat(id_parts, "/")
end

-- Parse app type name from formatted app_id
function  AppUtil.parse_app_name(app_id)
	local list = string.split(app_id, '/')
    if #list >= 3 then
        return list[2]
    end
    return list[1]
end

function AppUtil.is_publish_mode()
    return string.find(env.meta.labels.env or "", "publish") ~= nil
end

function AppUtil.require_config(name)
	local environ =  env.meta.labels.env or ""
	local path = string.format("config/%s%s.yaml", name, environ)  
	if not fs.exists(path) then
		path = string.format("config/%s.yaml", name)
	end
	return require_yaml(path), path
end

function AppUtil.GetServerID()
    return AppUtil.get_app().clusterConfig.clusterId
end

function AppUtil.is_debug_mode()
    local app = AppUtil.get_app()
    if app.commonConfig and to_boolean(app.commonConfig.debugMode) == true then
        return true
    end
    return false
end

function AppUtil.enable_log_message()
    local app = AppUtil.get_app()
    if app.commonConfig and to_boolean(app.commonConfig.logMessage) == true then
        return true
    end
    return false
end

function AppUtil.GetMessageFilePath()
    local app = AppUtil.get_app()
    if app.commonConfig and app.commonConfig.messageFile then
        return app.commonConfig.messageFile
    end
    return "log/messsage"
end

return AppUtil
