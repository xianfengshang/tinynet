local LocationService = require("common/location_service")

LocationService:Init()

local ad = LocationService:QueryByLocation(117.19018554688, 39.125595092773)

log.warning("ad= %s", rjson.encode(ad))