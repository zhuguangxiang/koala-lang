
function test_arith(n)
    local res = 0
    local i = 0
    while i < n do
        res = res + 10
        res = res - 5
        res = res * 2
        res = res / 2
        res = res | 255
        res = res & 255
        res = res << 1
        res = res >> 1
        i = i + 1
    end
    return res
end

local n = 100000000
test_arith(n)
