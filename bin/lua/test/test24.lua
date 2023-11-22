local db = mmdb.open("GeoLite2/GeoLite2-City.mmdb")

local begin = high_resolution_time()
local data = db:lookup("183.193.95.34")

local cost = high_resolution_time() - begin
log.warning("cost:%.3f ms", cost * 1000)