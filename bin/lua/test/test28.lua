local tb = {1, 2, 3, 4, 6}
table.sort(tb, function (a, b)
    return a > b
end)
local pos = table.bisect(tb, 5, function (a, b)
    return a > b
end)
log.warning("tb=%s, pos= %s", rjson.encode(tb), pos)