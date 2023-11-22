local Codec = rjson
if jit then
    Codec = require("string.buffer")
end

local exports = {}

function exports.encode(msg)
    return Codec.encode(msg)
end

function exports.decode(msg_bin)
    return Codec.decode(msg_bin)
end

return exports
