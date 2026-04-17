
function test_float_arith(n)
    local res = 0.0
    local step = 1.1
    local i = 0

    while i < n do
        res = res + step
        i = i + 1
    end
    return res
end

local n = 100000000
print(test_float_arith(n))
