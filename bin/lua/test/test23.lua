local citys = {}

local function load_geojson(adcode)
    local path = string.format("geojson/%s.json", adcode)
    if not fs.exists(path) then
        return
    end
    local geodata = rjson.decode(fs.readfile(path))
    return geodata
end

local adcode = 100000

local data = load_geojson(adcode)

--[[
local province_data
for k, v in ipairs(data.features) do
    if v.properties.adcode == 310000 then
        province_data = load_geojson(v.properties.adcode)
        if province_data then
            log.warning("adcode:%s, name:%s", v.properties.adcode, v.properties.name)
            for k1, v1 in ipairs(province_data.features) do
                local city = {}
                city.adcode = v1.properties.adcode
                city.name = v1.properties.name
                city.geometry = {}
                city.geometry.type = v1.geometry.type
                city.geometry.polygons = {}
                if v1.geometry.type == "MultiPolygon" then
                    city.geometry.polygons = v1.geometry.coordinates
                elseif v1.geometry.type == "Polygon" then
                    table.insert(city.geometry.polygons, v1.geometry.coordinates)
                end
                table.insert(citys, city)
            end
        end
        local v1 = v
        local city = {}
        city.adcode = v1.properties.adcode
        city.name = v1.properties.name
        city.geometry = {}
        city.geometry.type = v1.geometry.type
        city.geometry.polygons = {}
        if v1.geometry.type == "MultiPolygon" then
            city.geometry.polygons = v1.geometry.coordinates
        elseif v1.geometry.type == "Polygon" then
            table.insert(city.geometry.polygons, v1.geometry.coordinates)
        end
        table.insert(citys, city)
    end
end
--]]

local province_data
for k, v in ipairs(data.features) do
    --if v.properties.adcode == 310000 then
        local v1 = v
        local city = {}
        city.adcode = v1.properties.adcode
        city.name = v1.properties.name
        city.geometry = v1.geometry
        table.insert(citys, city)
    --end
end


log.warning("total citis =%s", #citys)

local function query(location)
    for k, v in ipairs(citys) do
        if geo.contains(v.geometry, location) then
            return v
        end
    end
end

local function test(location)
    local city = query(location)
    if city then
        log.warning("位置是:%s", city.name)
    else
        log.warning("没有查询到结果")
    end
end

local location = {121.404861, 31.392111}
test(location)

location = {117.190182, 39.125596}

test(location)

--[[
local db = mmdb.open("GeoLite2/GeoLite2-City.mmdb")
local data = db:lookup("183.193.95.34")
log.warning("location=%s", rjson.encode(data.location))
test({data.location.longitude, data.location.latitude})

local location = {119.84573364257812, 36.64638529597495}

local begin = high_resolution_time()
test(location)
local cost = high_resolution_time() - begin

log.warning("cost %.3f ms", cost)

--]]
local gs = geo.new()

gs:Add(1001, 119.84573364257812, 36.64638529597495)
gs:Add(1002, 119.84573364257812, 36.65638529597495)

local res = gs:QueryRadius({119.84573364257812, 36.64638529597495}, 1200)
log.warning("res=%s", rjson.encode(res))