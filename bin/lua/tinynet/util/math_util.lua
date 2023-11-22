if not math.maxinteger then
    math.maxinteger = 2147483647
end

function math.clamp(value, minValue, maxValue)
    if value < minValue then
        return minValue
    end
    if value > maxValue then
        return maxValue
    end
    return value
end