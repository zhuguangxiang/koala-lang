
function test_bitwise(n)
    local x = 123456789
    local i = 0
    while i < n do
        x = x ~ (x << 13)
        x = x ~ (x >> 17)
        x = x ~ (x << 5)
        i = i + 1
    end
    return x
end

local n = 100000000
test_bitwise(n)
