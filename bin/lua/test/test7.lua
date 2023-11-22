local FsUtil = require("tinynet/util/fs_util")
local files, err = fs_readdir("config")

if not files then
    log.error(err)
    return
end

local handlers = FsUtil.list_file_names("lua/gate/handler", ".lua")
log.warning(rjson.encode(handlers))