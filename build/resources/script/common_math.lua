if not CMath then CMath = {} end

function CMath.IsNearlyEq(value1, value2, acc)
    local v1 = math.floor(value1 / acc + 0.5) * acc
    local v2 = math.floor(value2 / acc + 0.5) * acc
    return v1 == v2
end

function CMath.GammaToLin(color)
    return math.pow(color, 2.2)
end

function CMath.LinToGamma(color)
    return math.pow(color, 1.0 / 2.2)
end

function CMath.ColorNormalize(color)
    local m = math.max( math.max(color.x, color.y), color.z )
    local res = Vector4(0,0,0,0)
    res.x = color.x / m
    res.y = color.y / m
    res.z = color.z / m
    res.w = color.w
    return res, m
end

function CMath.ColorDenormalize(color, intensity)
    local res = Vector4(0,0,0,0)
    res.x = color.x * intensity
    res.y = color.y * intensity
    res.z = color.z * intensity
    res.w = color.w
    return res
end