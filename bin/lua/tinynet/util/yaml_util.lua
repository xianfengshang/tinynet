local string_gsub = string.gsub
local string_format = string.format
local assert = assert

--- require_yaml Parse yaml formatted file from disk
-- @param path An existing yaml file path
function  require_yaml(path)
    local text = assert(fs.readfile(path), string_format("Yaml file %s not exist!", path))
    local content = yaml.decode(text)
    return content
end
