

local buf = bytes.new()

for i = 1, 100 do
    buf:append(tostring(i))
end

log.debug(#buf)