local MapUtil = require("common/util/map_util")

--2184150
--2184158
--2200534
local pos = MapUtil.GetPosition(2184150)
log.warning("2184150, pos=%s", rjson.encode(pos))
pos = MapUtil.GetPosition(2184158)
log.warning("2184158, pos=%s", rjson.encode(pos))
pos = MapUtil.GetPosition(2200534)
log.warning("2200534, pos=%s", rjson.encode(pos))

--[[
local pos = MapUtil.GetPosition(2184150)
pos.x = pos.x + 8

local posId = MapUtil.GetPositionId(pos)
log.warning("posId=%s", posId)

pos = MapUtil.GetPosition(2184150)
pos.y = pos.y + 8
posId = MapUtil.GetPositionId(pos)
log.warning("posId=%s", posId)--]]