local MapUtil = require("common/util/map_util")

---@type AoiService
local aoi = aoi.new()


local beginTime = time()
log.warning("start time=%s", beginTime)

-- for i = 1, 100000 do
for i = 1, 10 do
    -- local x = math.random(1, 100)
    -- local y = math.random(1, 100)
    -- local z = math.random(1, 100)
    -- local pos = Vector3.New(x, y, z)

    local cellPos = Vector3.New(-20, 25, 0)
    -- local cellPos = Vector3.New(1, 2, 1)
    local worldPos = MapUtil.CellToWorld(cellPos)
    log.warning("worldPos x=%s y=%s z=%s", worldPos.x, worldPos.y, worldPos.z)

    -- local worldPos = Vector3.New(1, 2, 1)
    
    local cellPos = MapUtil.WorldToCell(worldPos)
    log.warning("cellPos x=%s y=%s z=%s", cellPos.x, cellPos.y, cellPos.z)

    -- aoi:Add(1, worldPos, worldPos)
    -- aoi.WorldToCell(worldPos.x, worldPos.y)
    -- log.warning("cellPos x=%s y=%s z=%s", cellPos.x, cellPos.y, cellPos.z)
end

local endTime = time()
log.warning("end time=%s interval=%s", endTime, (endTime - beginTime)*1000)