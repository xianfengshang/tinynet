local Types = require("public/schema/Types")

local opts = {}
opts.TB = "all"

opts.bytesLoader= function(tbname)
    local buf = bytebuf.new()
    local designPath = "design/config/"
    local path = string.format("%s%s.bytes", designPath, tbname)
    buf:loadFromFile(path)
    return buf
end

local begin = high_resolution_time()
Types.Init(opts)
local cost = high_resolution_time() - begin

log.warning("load table cost:%.3f ms", cost *1000)
log.warning("%s", Types.TB.TbGlobal.data.stamina_get_time)
log.warning("%s", rjson.encode(Types.TB.TbGlobal.data.motion_initial_list))
log.warning("%s", Types.TB.TbGlobal.data.market_stock_min)

local questInfo = Types.TB.TbQuest:GetItem(810001)
log.warning(rjson.encode(questInfo.Rewards))