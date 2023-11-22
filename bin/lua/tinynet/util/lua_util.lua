local loadstring = loadstring or load
local exports = {}

function exports.dofile(filename)
    return loadstring(fs.readfile(filename))()
end

return exports