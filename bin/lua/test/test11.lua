local filter = textfilter.new(textfilter.TRIE_FILTER)


local banned_words = {}
local word
for line in io.lines("config/banned_words.csv") do
    table.insert(banned_words, line)
end


local content = fs.readfile("test.txt")


local function contains(words)
    for k, v in ipairs(banned_words) do
        if string.find(words, v) then
            return true
        end
    end
    return false
end

local function replace(words)
    for k, v in ipairs(banned_words) do
        if string.find(words, v) then
            words = string.gsub(words, v, "*")
        end
    end
    return words
end

local begin = high_resolution_time()
filter:Init(banned_words)
local diff = high_resolution_time() - begin
log.warning("init cost %.3f ms", diff * 1000)

local begin = high_resolution_time()
local res = filter:Replace(content, "*")
local diff = high_resolution_time() - begin

log.warning("res=%s, elapse=%.3f us", tostring(res), diff * 1000 * 1000)
