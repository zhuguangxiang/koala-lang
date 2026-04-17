
function test_logic_chain(n)
    local i = 0
    local score = 0
    while i < n do
        if i % 2 == 0 then score = score + 1 end
        if i % 3 ~= 0 then score = score + 2 end
        if i < 5000   then score = score + 1 end
        if i >= 1000  then score = score - 1 end
        i = i + 1
    end
    return score
end

local n = 100000000
test_logic_chain(n)
