local string_endswith = string.endswith
if not string_endswith then
    string_endswith = function(str, pattern)
        return string.match(str, string.format("%s$", pattern)) ~= nil
    end
end

local FsUtil = {}


local defaultOpts = 
{
    ignores = nil, -- string or string array
    exts = nil, -- string or string array
    full_path = false,
    remove_ext = false,
    recursive = false, 
    max_depth = 10
}


local function internal_make_opts(data)
    local opts
    local t = type(data)
    if t == 'string' then
        opts = {}
        opts.exts = data
    elseif t == 'boolean' then
        opts = {}
        opts.recursive = data
    elseif t == 'number' then
        opts = {}
        opts.max_depth = data
    elseif t == 'table' then
        if #data > 0 then
            opts = {}
            opts.exts = data
        else
            opts = data
        end
    else
        opts = {}
    end
    for k, v in pairs(defaultOpts) do
        if opts[k] == nil then
            opts[k] = v
        end
    end
    return opts
end

local function internal_match_extension(name, exts)
    if exts == nil or #exts == 0 then
        return true
    end
    if type(exts) == "string" then
        return string_endswith(name, exts)
    end
    for _, ext in ipairs(exts) do
        if string_endswith(name, ext) then
            return true
        end
    end
    return false
end

local function internal_math_ignore(name, ignores)
    if ignores == nil or #ignores == 0 then
        return false
    end
    if type(ignores) == "string" then
        return string.match(name, ignores) ~= nil
    end
    for _, ignore in ignores(ignores) do
        if string.match(name, ignore) then
            return true
        end
    end
    return false
end

local function internal_get_path(path, name, opts, result)
    if internal_math_ignore(name, opts.ignores) then
        return
    end
    if not internal_match_extension(name, opts.exts) then
        return
    end
    if opts.remove_ext then
        local PathUtil = require("tinynet/util/path_util")
        name, _ = PathUtil.splitext(name)
    end
    if opts.full_path then
        if string.endswith(path, '/') then
            table.insert(result, path .. name) 
        else
            table.insert(result, path .. '/' .. name) 
        end
    else
        table.insert(result, name) 
    end
end

local function internal_list_dir(path, opts, depth, result)
    if depth >= opts.max_depth then
        return nil, "recursive too deep"
    end
    local dirents, err = fs.readdir(path)
    if not dirents then
        return nil, err
    end
    result = result or {} 
    for _, entry in ipairs(dirents) do
        if entry.type == "dir" then
            internal_list_dir(path .. '/' .. entry.name, opts, depth + 1, result)
        elseif entry.type == "file" then
            internal_get_path(path, entry.name, opts, result)
        elseif entry.type == "unknown" then
            internal_get_path(path, entry.name, opts, result)
        end
    end
    return result
end

function FsUtil.list_dir(path, opts)
    return internal_list_dir(path, internal_make_opts(opts), 1, {})
end

function FsUtil.list_dirs(paths, opts)
    opts = internal_make_opts(opts)
    local result = {}
    for _, v in ipairs(paths) do
        internal_list_dir(v, opts, 1, result)
    end
    return result
end

return FsUtil
