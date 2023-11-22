local PathUtil = {}

local BACKSLASH = string.byte('\\')
local SLASH = string.byte('/')

function PathUtil.basename(path)
    local index = string.find_last_of(path, "/\\")
    if not index then
        return path
    end
    return string.sub(path, index + 1)
end

function PathUtil.basedir(path)
    local index = string.find_first_of(path, "/\\")
    if not index then
        return ""
    end
    return string.sub(path, 1, index - 1)
end

function PathUtil.extname(path)
    local ext = string.match(path, ".+(%.%w+)$")
    return ext or ""
end

function PathUtil.splitext(path)
    local name, ext = string.match(path, "(.+)(%.%w+)$")
    if not name then
        return path, ""
    end
    return name, ext
end

function PathUtil.trimslash(path)
    if string.byte(path, #path) == BACKSLASH or string.byte(path, #path) == SLASH then
        return string.sub(path, 1, #path - 1)
    end
    return path
end

function PathUtil.join(path, ...)
    local paths, ext
    if type(path) == "table" then
        paths = path
        ext = select(1, ...)
    else
        paths = {path, ...}
    end
    for k, v in ipairs(paths) do
        paths[k] = PathUtil.trimslash(v) 
    end
    local res = table.concat(paths, '/')
    if type(ext) == "string" then
        res = res .. ext
    end
    return res
end

return PathUtil