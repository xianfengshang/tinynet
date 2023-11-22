--- require_json import json file from disk

---Import json file from disk
---@param path string An existing json file path
---@return string
function  require_json(path)
    local text = assert(fs.readfile(path), string.format("File %s not exist!", path))
    local content = rjson.decode(text)
    return content
end