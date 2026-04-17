
function test_float_logic(n)
    local res = 0.0
    local i = 0

    while i < n do
        res = res + 1.1
        if res >= 50.0 then
            res = res - 40.0
        end
        i = i + 1
    end
    return res
end

local n = 100000000
print(test_float_logic(n))
