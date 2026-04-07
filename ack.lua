local function ack(m, n)
    if m == 0 then
        return n + 1
    elseif n == 0 then
        return ack(m - 1, 1)
    else
        return ack(m - 1, ack(m, n - 1))
    end
end

local m, n = 3, 11
local start = os.clock()
local result = ack(m, n)
local elapsed = os.clock() - start

print(string.format("Ack(%d, %d) = %d", m, n, result))
print(string.format("elapsed: %.6f sec", elapsed))
