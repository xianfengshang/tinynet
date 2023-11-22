local Vector3 = require("UnityEngine/Vector3")

local aoi = aoi.new()

local id = 1

local first = Vector3.New(math.random(), math.random())

local size = Vector3.New(1, 1, 1)

aoi:Add(id, first, size)

local pos_map = {}
pos_map[id] = first

assert(aoi:Size() == 1)

for i = 1, 10000 do
    id = id + 1
    pos_map[id] = Vector3.New(math.random(0, 1000), math.random(0, 1000))
    aoi:Add(id, pos_map[id], size)
end

log.warning("Count=%s", aoi:Size())

--aoi:Remove(1)
log.warning("Count=%s", aoi:Size())
local begin = high_resolution_time()
local center = Vector3.New(math.random(1, 100), math.random(1, 100))
local res = aoi:QueryRadius(center, 10)
--local res = aoi:QueryNearby(1, 1)
--center = pos_map[1]
local cost = high_resolution_time() - begin
log.warning("res:%s cost %s ms", #res, cost * 1000)
for k, v in pairs(res) do
    log.warning("distance =%s", Vector3.Distance(center, pos_map[v]))
end
--[[
local pos_a = Vector3.New(50, 50)
local pos_b = Vector3.New(50, 55)
local diff_a, diff_b = aoi:Diff(pos_a, pos_b, 10)
local cost = high_resolution_time() - begin
log.warning("diff_a = %s, diff_b=%s, cost:%.3f ms", #diff_a, #diff_b, cost * 1000)
log.warning("%s", rjson.encode(diff_a))
log.warning("%s", rjson.encode(diff_b))
for k, v in pairs(diff_a) do
    for k1, v1 in pairs(diff_b) do
        if v == v1 then
            log.error("v == v1, v:%s, v1:%s", v, v1)
        end
    end
end--]]


