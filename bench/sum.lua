-- test_sum.lua
local function sum(n, acc)
    if n == 0 then
        return acc
    end
    -- 这是一个标准的尾调用，Lua 会复用栈帧
    return sum(n - 1, acc + n)
end

local n = 100000000 -- 一亿次递归
local start = os.clock()
local result = sum(n, 0)
local elapsed = os.clock() - start

print(string.format("sum(%d, 0) = %d", n, result))
print(string.format("elapsed: %.6f sec", elapsed))
