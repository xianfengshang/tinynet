---@diagnostic disable: lowercase-global
---@meta
assert(false, "You can't not import a meta file")

---@class Object
---@field public super Object
---@field public class Object
---@field public __index Object
---@field public __name string the name of the class
---@field public constructor function
---@field public new function
local Object = {}

--- Constructor method for the class
---@vararg any Arguments for the constructor
function Object:constructor(...) end

--- Create a new instance of the class
---@vararg any Arguments for the constructor
function Object.new(...) end

---Get the time since the Epoch (00:00:00 UTC, January 1, 1970), measured in seconds.
---@return number time in seconds
function time() return 0.0 end

---Get the time since the Epoch (00:00:00 UTC, January 1, 1970), measured in microseconds.
---@return integer time in microseconds
function time_ms() return 0 end

---Get the high resolution  time since the Epoch (00:00:00 UTC, January 1, 1970), measured in seconds.
---@return number time in seconds
function high_resolution_time() return 0.0 end

---Return the CRC16 checksum of the given 'str'
---@param str string
---@return integer
function crc16(str) return 0 end

---Set the process title
---@param title string
function set_process_title(title) end

---Get the current executable file path
---@return string
function get_exe_path() return "" end

---Return the SHA1 checksum of the given 'str'
---@param str any
---@return string
function sha1_sum(str) return "" end

---Return the MD5 checksum of the given 'str'
---@param str string
---@return string
function md5_sum(str) return ""  end

---Return the utf-8 length of the 'str'
---@param str string
---@return integer
function utf8_len(str) return 0 end

----Convert a integer to string
function integer_to_string(value) return "" end

----Convert a string to integer
function string_to_integer(value) return 0 end

---Convert any value represented true or false to lua toolean
function to_boolean() return true or false end

---@class loglib log module
log = {}

---log debug message
---@param fmt string
---@param ... any args
function log.debug(fmt, ...) end

---log info message
---@param fmt string
---@param ... any args
function log.info(fmt, ...) end

---log warning message
---@param fmt string
---@param ... any args
function log.warning(fmt, ...) end

---log error message
---@param fmt string
---@param ... any args
function log.error(fmt, ...) end

---log fatal message
---@param fmt string
---@param ... any args
function log.fatal(fmt, ...) end

---@class cjsonlib cjson module
cjson = {}

--- Encode a plain object to json string
---@param value any plain object
function cjson.encode(value) return "" end

--- Decode a object from the json string
---@param str any
function cjson.decode(str) return {} end

---@class rjsonlib rjson module
rjson = {
    ---@type lightuserdata
    null = nil
}

--- Encode a plain object to json string
---@param value any plain object
---@param opts table|nil
function rjson.encode(value, opts) return "" end

--- Decode a object from the json string
---@param str any
---@param opts table|nil
function rjson.decode(str, opts) return {} end


---@class AoiService
local AoiService = {}

--- Add an entity to the aoi service
---@param id integer entity id, should be unique
---@param pos Vector3 entity position
---@param size Vector3 entity size
function AoiService:Add(id, pos, size)
end

--- Update the entity's position
---@param id integer entity id, should be unique
---@param pos Vector3 entity position
---@param size Vector3 entity size
function AoiService:Update(id, pos, size)
end

--- Remove an entity from the aoi service
---@param id integer entity id
---@return boolean
function AoiService:Remove(id)
    return false
end

--- Finds the entities within the circle area
---@param center Vector3 the bound box center pos
---@param size Vector3 the bound box size
---@return integer[]
function AoiService:QueryBound(center, size)
end

--- Finds the entities within the circle area
---@param center Vector3 the search center position
---@param distance number the search radius
---@return integer[]
function AoiService:QueryRadius(center, distance)
end

--- Finds the entities within the circle area
---@param center Vector3 the search center position
---@param distance number the search radius
---@return integer[]
function AoiService:QueryNearby(entryId)
end

--- Check the giving entity whether associated to the aoi service or not
---@param id integer
---@return boolean
function AoiService:Contains(id)
    return false
end

--- Finds the difference set of the two circle area
---@param pos_a Vector3
---@param pos_b Vector3
---@param distance number
---@return integer[] The entities within the area a but not within the area b
---@return integer[] The entities within the area b but not within the ara a
function AoiService:Diff(pos_a, pos_b, distance)
end

---@class GeoSearchResult
---@field entryId integer
---@field distance number


---@class GeoService
local GeoService = {}

--- Add an entry to the geo service
---@param entryId integer
---@param lon integer|integer[] longitude or (lon,lat) pair
---@param lat integer|nil latitude
function GeoService:Add(entryId, lon, lat)
end

--- Remove the entry from the geo service
---@param entryId integer
---@return boolean
function GeoService:Remove(entryId)
    return true
end

--- Query entries within the giving circle area
---@param pos any
---@param radius any
---@return GeoSearchResult[]
function GeoService:QueryRadius(pos, radius)
    return {}
end

--- Query a number of nearest entries around the giving entry
---@param pos any
---@param n any
---@return GeoSearchResult[]
function GeoService:QueryNearest(entryId, n)
    return {}
end

--- Returns the entries number
---@return integer
function GeoService:Size()
    return 0
end

--- Returns true if the geo service contains the entry, otherwise false will be returned
---@param entryId any
function GeoService:Contains(entryId)
end

--- Returns the coordinates of the entry if existing, otherwise returns nil
---@return table 
function GeoService:GetEntry(entryId)
end


---@class TileMap
local TileMap = {}

--- Add an entry to the TileMap
---@param entryId integer
---@param min_corner Vector3
---@param max_corner Vector3
function TileMap:Add(entryId, min_corner, max_corner)
end

--- Update bound box for the entry identified by the entryId
---@param entryId integer
---@param min_corner Vector3
---@param max_corner Vector3
function TileMap:Update(entryId, min_corner, max_corner)
end

--- Remove the entry from the geo service
---@param entryId integer
---@return boolean
function TileMap:Remove(entryId)
    return true
end

--- Query entries within the giving bound
---@param min_corner Vector3
---@param max_corner Vector3
---@return integer[]
function TileMap:QueryBound(min_corner, max_corner)
    return {}
end

--- Returns the entries number
---@return integer
function TileMap:Size()
    return 0
end

--- Returns true if the geo service contains the entry, otherwise false will be returned
---@param entryId any
function TileMap:Contains(entryId)
end

---@class aoilib aoi module
aoi = {}

--- New aoi service object
---@return AoiService
function aoi.new() end

---@class geolib geo module
geo = {}

function geo.contains() end


function geo.centroid() end

---@return GeoService
function geo.new() end

---@class tilemaplib tilemap module
tilemap = {}

--- New  tilemap object
---@return TileMap
function tilemap.new() end

---@class AppMeta
---@field name string
---@field labels table<string, string> 

---@class envlib
---@field meta AppMeta
---@field app any
env = {}

local tinynet = {}
return tinynet