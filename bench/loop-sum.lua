local function sum(n)
    local s = 0
    for i = 1, n do
        s = s + i
    end
    return s
end

local n = 100000000
local r = sum(n)
print("sum(" .. n .. ") = " .. r)
